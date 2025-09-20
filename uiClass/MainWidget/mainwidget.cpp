#include "mainwidget.h"
#include "ui_MainWidget.h"       // 包含UI生成的头文件
#include <QMessageBox>          // 包含QMessageBox类，用于弹出提示框
#include <QJsonObject>          // JSON对象处理
#include <QJsonDocument>        // JSON文档处理（序列化/反序列化）
#include <utility>
#include <QJsonArray>  // 添加QJsonArray头文件

// 构造函数：初始化成员变量、UI和MQTT客户端
MainWidget::MainWidget(QWidget* parent,QString ip,QString topic) :
    QMainWindow(parent),
    ui(new Ui::MainWidget),
    // 初始化设备状态（默认均为关闭）
    ledState(false), buzzerState(false), fanState(false),
    doorLockState(false), tvState(false), infraredState(false),
    airConditionerState(false),
    // 初始化传感器数据（默认值）
    temperature(0.0), humidity(0.0),
    waterHeaterTemp(0.0), airConditionerTemp(25),  // 空调默认25℃
    // 初始化阈值（默认值）
    tempUpperThreshold(30.0), tempLowerThreshold(10.0),  // 温度阈值10-30℃
    humiUpperThreshold(70.0), humiLowerThreshold(30.0),  // 湿度阈值30-70%
    waterHeaterLowerThreshold(40.0),                     // 热水器最低40℃
    ip(std::move(ip)),topic(std::move(topic)){

    ui->setupUi(this);  // 初始化UI界面（加载.ui文件定义的控件）
    setWindowTitle("智能家居控制中心");  // 设置窗口标题

    // 连接信号与槽：将按钮点击事件绑定到对应的控制函数
    connect(ui->btnLed, &QPushButton::clicked, this, &MainWidget::onLedClicked);
    connect(ui->btnBuzzer, &QPushButton::clicked, this, &MainWidget::onBuzzerClicked);
    connect(ui->btnFan, &QPushButton::clicked, this, &MainWidget::onFanClicked);
    connect(ui->btnDoorLock, &QPushButton::clicked, this, &MainWidget::onDoorLockClicked);
    connect(ui->btnTv, &QPushButton::clicked, this, &MainWidget::onTvClicked);
    connect(ui->btnThermoHygro, &QPushButton::clicked, this, &MainWidget::onThermoHygroClicked);
    connect(ui->btnInfrared, &QPushButton::clicked, this, &MainWidget::onInfraredClicked);
    connect(ui->btnWaterHeater, &QPushButton::clicked, this, &MainWidget::onWaterHeaterClicked);
    connect(ui->btnAirConditioner, &QPushButton::clicked, this, &MainWidget::onAirConditionerClicked);
    // 空调温度滑块变化绑定到温度调节函数
    connect(ui->sliderAirConditionerTemp, &QSlider::valueChanged, this, &MainWidget::onAirConditionerTempChanged);

    updateDeviceUI();  // 初始化UI显示（根据默认状态刷新控件）
    initMqttClient();  // 初始化MQTT客户端
}

// 析构函数：释放资源
MainWidget::~MainWidget() {
    if (mqttClient) {
        mqttClient->disconnectFromHost();  // 断开MQTT连接
        delete mqttClient;                 // 释放MQTT客户端
    }
    delete ui;  // 释放UI指针
}

// 初始化MQTT客户端：设置连接参数并连接到服务器
void MainWidget::initMqttClient() {
    mqttClient = new QMqttClient(this);
    // 使用传入的ip作为MQTT服务器地址（替代原"localhost"）
    mqttClient->setHostname(this->ip);
    mqttClient->setPort(1883);          // 保持默认端口

    connect(mqttClient, &QMqttClient::stateChanged, this, &MainWidget::onMqttStateChanged);
    connect(mqttClient, &QMqttClient::messageReceived, this, &MainWidget::onMqttMessageReceived);

    mqttClient->connectToHost();
}

// 处理收到的MQTT消息：解析JSON并更新传感器数据
void MainWidget::onMqttMessageReceived(const QByteArray &message, const QMqttTopicName &topic) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(message, &error);

    if (error.error != QJsonParseError::NoError) {  // 解析失败则返回
        return;
    }

    QJsonObject rootObj = doc.object();

    // 只处理采集回复指令（type=1）且成功返回（result=0）的消息
    if (rootObj["type"].toInt() != 1 || rootObj["result"].toInt() != 0) {
        return;
    }

    // 提取数据点数组
    QJsonArray dataArray = rootObj["data"].toArray();
    if (dataArray.isEmpty()) {
        return;
    }

    // 遍历所有数据点，根据key更新对应设备/传感器状态
    for (const auto &val : dataArray) {
        QJsonObject dataObj = val.toObject();
        int key = dataObj["key"].toInt();       // 数据点key
        QString valStr = dataObj["val"].toString();  // 数据点值（字符串）

        // 根据点表映射key与设备/传感器
        switch (key) {
            // stm32模块 - 灯（key=301，type=1："1"=开，"0"=关）
            case 301:
                ledState = (valStr == "true");
                break;
            // stm32模块 - 蜂鸣器（key=302，type=1）
            case 302:
                buzzerState = (valStr == "true");
                break;
            // stm32模块 - 风扇（key=303，type=1）
            case 303:
                fanState = (valStr == "true");
                break;
            // stm32模块 - 湿度（key=304）
            case 304:
                humidity = valStr.toDouble();
                break;

            //305和306是湿度上下限阈值，无需采集

            // stm32模块 - 温度（key=1）
            case 307:
                temperature = valStr.toDouble();
                break;

            //308和309是温度上下限阈值无需采集

            // stm32模块 - 人体红外（key=310，type=1）
            case 310:
                infraredState = (valStr == "true");
                break;
            // stm32模块 - 门锁（key=311，type=1）
            case 311:
                doorLockState = (valStr == "true");
                break;
            // modbus模块 - 电视（key=101，type=1）
            case 101:
                tvState = (valStr == "true");
                break;
            // modbus模块 - 热水器温度（key=103，type=3）
            case 103:
                waterHeaterTemp = valStr.toDouble();
                break;
            // modbus模块 - 空调开关（key=104，type=1）
            case 104:
                airConditionerState = (valStr == "true");
                break;
            // modbus模块 - 空调温度（key=105，type=3）
            case 105:
                airConditionerTemp = valStr.toInt();
                break;
            // 其他未用到的key可在此扩展
            default:
                break;
        }
    }

    updateDeviceUI();  // 刷新UI显示
}

// 处理MQTT连接状态变化：连接成功时订阅主题
void MainWidget::onMqttStateChanged(const QMqttClient::ClientState state) const {
    if (state == QMqttClient::Connected) {  // 当客户端连接成功时
        // 可扩展：订阅需要的主题（如控制指令反馈、传感器数据等）
        mqttClient->subscribe(topic);
    }
}

// 更新UI界面：根据设备状态和传感器数据刷新控件显示
void MainWidget::updateDeviceUI() {
    // 更新LED灯按钮显示（文字和样式）
    ui->btnLed->setText(ledState ? "开" : "关");
    ui->btnLed->setStyleSheet(ledState ?
        "QPushButton { background-color: #4CAF50; color: white; }" :  // 开：绿色背景
        "QPushButton { background-color: #f8f9fa; color: #3c4043; }"); // 关：默认样式

    // 更新蜂鸣器按钮显示
    ui->btnBuzzer->setText(buzzerState ? "开" : "关");
    ui->btnBuzzer->setStyleSheet(buzzerState ?
        "QPushButton { background-color: #4CAF50; color: white; }" :
        "QPushButton { background-color: #f8f9fa; color: #3c4043; }");

    // 更新风扇按钮显示
    ui->btnFan->setText(fanState ? "开" : "关");
    ui->btnFan->setStyleSheet(fanState ?
        "QPushButton { background-color: #4CAF50; color: white; }" :
        "QPushButton { background-color: #f8f9fa; color: #3c4043; }");

    // 更新门锁按钮显示
    ui->btnDoorLock->setText(doorLockState ? "开" : "关");
    ui->btnDoorLock->setStyleSheet(doorLockState ?
        "QPushButton { background-color: #4CAF50; color: white; }" :
        "QPushButton { background-color: #f8f9fa; color: #3c4043; }");

    // 更新电视按钮显示
    ui->btnTv->setText(tvState ? "开" : "关");
    ui->btnTv->setStyleSheet(tvState ?
        "QPushButton { background-color: #4CAF50; color: white; }" :
        "QPushButton { background-color: #f8f9fa; color: #3c4043; }");

    // 更新空调按钮显示
    ui->btnAirConditioner->setText(airConditionerState ? "开" : "关");
    ui->btnAirConditioner->setStyleSheet(airConditionerState ?
        "QPushButton { background-color: #4CAF50; color: white; }" :
        "QPushButton { background-color: #f8f9fa; color: #3c4043; }");

    // 更新温度显示标签
    ui->lblTemperature->setText(QString("温度: %1 °C").arg(temperature, 0, 'f', 2));
    // 更新湿度显示标签
    ui->lblHumidity->setText(QString("湿度: %1 %").arg(humidity, 0, 'f', 2));
    // 更新热水器温度显示标签
    ui->lblWaterHeater->setText(QString("水温: %1 °C").arg(waterHeaterTemp, 0, 'f', 2));
    // 更新红外传感器显示标签（文字和颜色）
    ui->lblInfrared->setText(infraredState ? "检测到人体" : "未检测到");
    ui->lblInfrared->setStyleSheet(infraredState ? "color: #F44336;" : "color: #3c4043;");  // 检测到：红色

    // 更新空调温度显示标签
    ui->lblAirConditionerTemp->setText(QString("温度: %1°C").arg(airConditionerTemp));
}

// 发布设备状态到MQTT服务器：将设备开关状态以JSON格式发送
void MainWidget::publishDeviceState(const QString& device, bool state) {
    // 检查MQTT客户端是否存在且已连接
    if (!mqttClient || mqttClient->state() != QMqttClient::Connected) {
        return;  // 未连接则不发送
    }

    // 仅保留需要控制的五个外设映射（根据点表）
    QMap<QString, int> deviceKeyMap = {
        {"led", 301},                  // LED灯（stm32的light）
        {"buzzer", 302},               // 蜂鸣器（stm32的fengmingqi）
        {"fan", 303},                  // 风扇（stm32的fan）
        {"door_lock", 311},            // 门锁（stm32的lock）
        {"air_conditioner", 104},       // 空调（modbus的air-switch）
        {"tv",101}                      //电视
    };

    // 检查设备是否在需要控制的列表中
    if (!deviceKeyMap.contains(device)) {
        return;  // 非目标设备不发送
    }

    // 构造JSON消息体（符合指定格式）
    QJsonObject rootJson;
    rootJson["type"] = 2;  // 指令类型：2-控制指令

    QJsonObject dataJson;
    dataJson["key"] = deviceKeyMap[device];  // 数据点唯一标识
    dataJson["val"] = state ? "true" : "false";  // 开关状态（字符串类型）

    rootJson["data"] = dataJson;

    QJsonDocument doc(rootJson);
    // 发布消息到指定主题
    mqttClient->publish(QString("up"), doc.toJson());
}


// 发布传感器阈值到MQTT服务器：将传感器上下限阈值以JSON格式发送
void MainWidget::publishSensorThreshold(const QString& sensor, float lower, float upper) {
    // 检查MQTT客户端是否存在且已连接
    if (!mqttClient || mqttClient->state() != QMqttClient::Connected) {
        return;  // 未连接则不发送
    }

    // 构造JSON消息体
    QJsonObject json;
    json["sensor"] = sensor;                  // 传感器名称
    json["lower"] = lower;                    // 下限阈值
    json["upper"] = upper;                    // 上限阈值
    json["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);  // 时间戳

    QJsonDocument doc(json);  // 序列化JSON对象为字节数组
    // 发布消息到指定主题（threshold/传感器名）
    mqttClient->publish(QString("threshold/%1").arg(sensor), doc.toJson());
}

// LED灯控制：切换状态并发布到MQTT
void MainWidget::onLedClicked() {
    ledState = !ledState;  // 切换LED状态（取反）
    updateDeviceUI();      // 刷新UI
    publishDeviceState("led", ledState);  // 发布状态到MQTT
}

// 蜂鸣器控制：切换状态并发布到MQTT
void MainWidget::onBuzzerClicked() {
    buzzerState = !buzzerState;
    updateDeviceUI();
    publishDeviceState("buzzer", buzzerState);
}

// 风扇控制：切换状态并发布到MQTT
void MainWidget::onFanClicked() {
    fanState = !fanState;
    updateDeviceUI();
    publishDeviceState("fan", fanState);
}

// 门锁控制：切换状态并发布到MQTT
void MainWidget::onDoorLockClicked() {
    doorLockState = !doorLockState;
    updateDeviceUI();
    publishDeviceState("door_lock", doorLockState);
}

// 电视控制：切换状态并发布到MQTT
void MainWidget::onTvClicked() {
    tvState = !tvState;
    updateDeviceUI();
    publishDeviceState("tv", tvState);
}

// 温湿度计控制：弹出阈值设置提示（待实现）
void MainWidget::onThermoHygroClicked() {
    QMessageBox::information(this, "温湿度计", "阈值设置功能待实现");
}

// 红外传感器控制：弹出详情提示（待实现）
void MainWidget::onInfraredClicked() {
    QMessageBox::information(this, "人体红外传感器", "详情功能待实现");
}

// 热水器控制：弹出温度设置提示（待实现）
void MainWidget::onWaterHeaterClicked() {
    QMessageBox::information(this, "热水器", "温度设置功能待实现");
}

// 空调开关控制：切换状态并发布到MQTT
void MainWidget::onAirConditionerClicked() {
    airConditionerState = !airConditionerState;
    updateDeviceUI();
    publishDeviceState("air_conditioner", airConditionerState);
}

// 空调温度调节：更新温度并发布到MQTT
void MainWidget::onAirConditionerTempChanged(int value) {
    airConditionerTemp = value;  // 更新空调设定温度
    updateDeviceUI();            // 刷新UI显示

    // 检查MQTT客户端是否连接
    if (!mqttClient || mqttClient->state() != QMqttClient::Connected) {
        return;
    }

    // 构造符合要求的控制指令JSON格式
    QJsonObject rootJson;
    rootJson["type"] = 2;  // 指令类型：2-控制指令

    QJsonObject dataJson;
    dataJson["key"] = 105; // 空调温度对应的数据点key
    dataJson["val"] = QString::number(value); // 温度值转为字符串类型

    rootJson["data"] = dataJson;

    QJsonDocument doc(rootJson);
    // 发布到控制指令主题（与其他设备控制保持一致）
    mqttClient->publish(QString("up"), doc.toJson());
}