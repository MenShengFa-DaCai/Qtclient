#include "linkpush.h"
#include "ui_LinkPush.h"
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>

LinkPush::LinkPush(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::LinkPush),
    m_tcpSocket(nullptr) {
    ui->setupUi(this);
    m_tcpSocket = new QTcpSocket(this);

    // 连接TCP信号槽
    connect(m_tcpSocket, &QTcpSocket::connected, this, &LinkPush::onTcpConnected);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &LinkPush::onTcpReadyRead);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &LinkPush::onTcpDisconnected);
    connect(m_tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &LinkPush::onTcpErrorOccurred);
}

LinkPush::~LinkPush() {
    if (m_tcpSocket) {
        m_tcpSocket->disconnectFromHost();
        m_tcpSocket->deleteLater();
    }
    delete ui;
}

void LinkPush::connectToDevice(const QString& ip, const QString& topic) {
    m_ipAddress = ip;
    m_topic = topic;

    ui->label_info->setText(QString("正在连接到设备 %1...").arg(ip));

    // 连接到设备
    m_tcpSocket->connectToHost(ip, 8888);
}

void LinkPush::onTcpConnected() {
    ui->label_info->setText(QString("已连接到设备 %1").arg(m_ipAddress));

    // 发送版本信息
    QJsonObject versionInfo;
    versionInfo["type"] = 1;

    QJsonObject dataObj;
    dataObj["ver"] = "v1.0.1";
    dataObj["file_name"] = "update.zip";
    dataObj["file_len"] = 560000;
    dataObj["md5"] = "AABBCCDDEEFF";

    versionInfo["data"] = dataObj;

    QJsonDocument doc(versionInfo);
    m_tcpSocket->write(doc.toJson(QJsonDocument::Compact));

    ui->label_info->setText("已发送升级问询信息，等待设备回复...");
}

void LinkPush::onTcpDisconnected() {
    ui->label_info->setText(QString("与设备 %1 断开连接").arg(m_ipAddress));
}

void LinkPush::onTcpErrorOccurred(QAbstractSocket::SocketError error) {
    Q_UNUSED(error)
    ui->label_info->setText(QString("连接错误: %1").arg(m_tcpSocket->errorString()));
}

void LinkPush::onTcpReadyRead() {
    if (!m_tcpSocket) return;

    QByteArray data = m_tcpSocket->readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        ui->label_info->setText("收到无效的JSON数据");
        return;
    }

    QJsonObject jsonObj = doc.object();
    if (!jsonObj.contains("type")) {
        ui->label_info->setText("收到的数据包缺少type字段");
        return;
    }

    int type = jsonObj["type"].toInt();

    switch (type) {
    case 1: {
        // 处理升级问询回复
        if (jsonObj.contains("data") && jsonObj["data"].isObject()) {
            QJsonObject dataObj = jsonObj["data"].toObject();
            bool needUpdate = dataObj["update"].toBool();
            if (needUpdate) {
                ui->label_info->setText(QString("设备 %1 需要升级").arg(m_ipAddress));
                // 这里可以添加升级逻辑
            } else {
                ui->label_info->setText(QString("设备 %1 已是最新版本").arg(m_ipAddress));
            }
        }
        break;
    }
    case 3: {
        // 处理升级结果回复
        if (jsonObj.contains("data") && jsonObj["data"].isObject()) {
            QJsonObject dataObj = jsonObj["data"].toObject();
            bool updateSuccess = dataObj["update_success"].toBool();
            if (updateSuccess) {
                ui->label_info->setText(QString("设备 %1 升级成功").arg(m_ipAddress));
            } else {
                ui->label_info->setText(QString("设备 %1 升级失败").arg(m_ipAddress));
            }
        }
        break;
    }
    default:
        ui->label_info->setText(QString("收到未知类型的数据包: %1").arg(type));
        break;
    }
}