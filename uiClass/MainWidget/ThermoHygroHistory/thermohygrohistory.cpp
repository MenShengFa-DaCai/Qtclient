#include "thermohygrohistory.h"
#include "ui_ThermoHygroHistory.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

ThermoHygroHistory::ThermoHygroHistory(QMqttClient* client, QWidget* parent) :
    QDialog(parent),
    ui(new Ui::ThermoHygroHistory),
    mqttClient(client),
    receivedCount(0), m_startTime(0),
    m_endTime(0)
{
    ui->setupUi(this);

    // 设置默认时间范围为过去24小时
    ui->startDateTime->setDateTime(QDateTime::currentDateTime().addDays(-1));
    ui->endDateTime->setDateTime(QDateTime::currentDateTime());

    // 连接查询按钮
    connect(ui->queryButton, &QPushButton::clicked, this, &ThermoHygroHistory::onQueryButtonClicked);

    // 连接MQTT消息接收
    if (mqttClient) {
        connect(mqttClient, &QMqttClient::messageReceived, this, &ThermoHygroHistory::onMqttMessageReceived);
    }

    // 设置图表
    setupChart();
}

ThermoHygroHistory::~ThermoHygroHistory() {
    delete ui;
}

void ThermoHygroHistory::setupChart() {
    // 设置图表标题
    ui->customPlot->plotLayout()->insertRow(0);
    ui->customPlot->plotLayout()->addElement(
        0, 0, new QCPTextElement(ui->customPlot, "温湿度历史数据", QFont("sans", 12, QFont::Bold)));

    // 设置温度图表（左侧Y轴）
    ui->customPlot->addGraph();
    ui->customPlot->graph(0)->setPen(QPen(Qt::red));
    ui->customPlot->graph(0)->setName("温度");

    // 设置湿度图表（右侧Y轴）
    ui->customPlot->addGraph(ui->customPlot->xAxis, ui->customPlot->yAxis2);
    ui->customPlot->graph(1)->setPen(QPen(Qt::blue));
    ui->customPlot->graph(1)->setName("湿度");

    // 设置X轴为时间轴
    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setDateTimeFormat("MM-dd hh:mm");
    ui->customPlot->xAxis->setTicker(dateTicker);

    // 设置Y轴
    ui->customPlot->yAxis->setLabel("温度 (°C)");
    ui->customPlot->yAxis->setLabelColor(Qt::red);
    ui->customPlot->yAxis->setTickLabelColor(Qt::red);

    // 设置右侧Y轴
    ui->customPlot->yAxis2->setVisible(true);
    ui->customPlot->yAxis2->setLabel("湿度 (%)");
    ui->customPlot->yAxis2->setLabelColor(Qt::blue);
    ui->customPlot->yAxis2->setTickLabelColor(Qt::blue);

    // 添加图例
    ui->customPlot->legend->setVisible(true);

    // 设置交互
    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
}

void ThermoHygroHistory::onQueryButtonClicked() {
    if (!mqttClient || mqttClient->state() != QMqttClient::Connected) {
        QMessageBox::warning(this, "错误", "MQTT客户端未连接");
        return;
    }

    // 获取时间范围并保存到成员变量
    m_startTime = ui->startDateTime->dateTime().toSecsSinceEpoch();
    m_endTime = ui->endDateTime->dateTime().toSecsSinceEpoch();

    if (m_startTime >= m_endTime) {
        QMessageBox::warning(this, "错误", "开始时间必须早于结束时间");
        return;
    }

    // 清空现有数据
    temperatureTime.clear();
    temperatureValues.clear();
    humidityTime.clear();
    humidityValues.clear();
    receivedCount = 0;

    // 只先发送温度历史数据请求（第一个请求）
    QJsonObject tempRequest;
    tempRequest["type"] = 4;

    QJsonObject tempData;
    tempData["key"] = 307;
    QJsonArray tempLimit;
    tempLimit.append(static_cast<double>(m_startTime));
    tempLimit.append(static_cast<double>(m_endTime));
    tempData["limit"] = tempLimit;

    tempRequest["data"] = tempData;

    QJsonDocument tempDoc(tempRequest);
    mqttClient->publish(QString("up"), tempDoc.toJson());
}

void ThermoHygroHistory::onMqttMessageReceived(const QByteArray& message, const QMqttTopicName& topic) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(message, &error);

    if (error.error != QJsonParseError::NoError) {
        return;
    }

    QJsonObject rootObj = doc.object();

    // 只处理历史数据回复（type=4）且成功返回（result=0）的消息
    if (rootObj["type"].toInt() == 4 && rootObj["result"].toInt() == 0) {
        int key = rootObj["key"].toInt();
        QJsonArray dataArray = rootObj["data"].toArray();

        processHistoryData(key, dataArray);
        receivedCount++;

        // 收到温度回复后，再发送湿度请求（串行处理）
        if (key == 307 && receivedCount == 1) {
            // 发送湿度历史数据请求（第二个请求）
            QJsonObject humiRequest;
            humiRequest["type"] = 4;

            QJsonObject humiData;
            humiData["key"] = 304;
            QJsonArray humiLimit;
            humiLimit.append(static_cast<double>(m_startTime));
            humiLimit.append(static_cast<double>(m_endTime));
            humiData["limit"] = humiLimit;

            humiRequest["data"] = humiData;

            QJsonDocument humiDoc(humiRequest);
            mqttClient->publish(QString("up"), humiDoc.toJson());
        }

        // 当收到两个响应后（温度+湿度），更新图表
        if (receivedCount == 2) {
            // 更新温度图表
            ui->customPlot->graph(0)->setData(temperatureTime, temperatureValues);

            // 更新湿度图表
            ui->customPlot->graph(1)->setData(humidityTime, humidityValues);

            // 自动调整范围
            ui->customPlot->rescaleAxes();

            // 刷新图表
            ui->customPlot->replot();
        }
    }
}

void ThermoHygroHistory::processHistoryData(int key, const QJsonArray& data) {
    for (const auto& item : data) {
        QJsonObject dataObj = item.toObject();
        double time = dataObj["time"].toDouble();
        double value = dataObj["val"].toString().toDouble();

        if (key == 307) {
            // 温度
            temperatureTime.append(time);
            temperatureValues.append(value);
        }
        else if (key == 304) {
            // 湿度
            humidityTime.append(time);
            humidityValues.append(value);
        }
    }
}
