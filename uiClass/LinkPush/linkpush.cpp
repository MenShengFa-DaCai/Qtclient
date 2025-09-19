#include "linkpush.h"
#include "ui_LinkPush.h"
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>          // QFile需要的头文件
#include <QCryptographicHash>  // 用于计算MD5
#include "../Login/login.h"
#include "FileSenderThread.h"


LinkPush::LinkPush(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::LinkPush),
    m_tcpSocket(nullptr) {
    ui->setupUi(this);
    m_tcpSocket = new QTcpSocket(this);
    this->parent=parent;

    // 初始化进度条
    ui->progressBar_upgrade->setValue(0);
    ui->progressBar_upgrade->setVisible(false); // 初始隐藏进度条

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
//链接并发送版本信息
void LinkPush::onTcpConnected() {
    ui->label_info->setText(QString("已连接到设备 %1").arg(m_ipAddress));
    m_jsonData.clear();

    // 发送版本信息
    QJsonObject versionInfo;
    versionInfo["type"] = 1;

    // 读取update.json文件获取版本信息并计算文件信息
    QJsonObject dataObj;
    QFile file("update.json");

    if (!file.exists()) {
        ui->label_info->setText("错误: update.json文件不存在");
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->label_info->setText(QString("错误: 无法打开update.json文件 - %1").arg(file.errorString()));
        return;
    }

    // 先读取所有文件内容
    m_jsonData  = file.readAll();
    file.close(); // 读取完成后即可关闭文件

    // 检查是否读取到数据
    if (m_jsonData.isEmpty()) {
        ui->label_info->setText("错误: update.json文件内容为空");
        return;
    }

    // 计算文件长度（使用已读取的字节数组长度）
    qint64 fileLen = m_jsonData.size();
    dataObj["file_len"] = fileLen;

    // 计算MD5（使用已读取的jsonData）
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(m_jsonData); // 直接用读取到的字节数组计算MD5
    QByteArray md5 = hash.result();
    dataObj["md5"] = QString(md5.toHex());

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(m_jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        ui->label_info->setText(QString("错误: 解析update.json失败 - %1").arg(parseError.errorString()));
        return;
    }

    if (!jsonDoc.isObject()) {
        ui->label_info->setText("错误: update.json格式不正确，不是JSON对象");
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();

    // 从JSON获取version字段
    if (jsonObj.contains("version") && jsonObj["version"].isString()) {
        dataObj["ver"] = jsonObj["version"].toString();
    } else {
        ui->label_info->setText("错误: update.json缺少或无效的version字段");
        return;
    }

    dataObj["file_name"] = "update.json";  // 文件名固定为update.json

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

//有数据可读即调用此函数
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
                // 升级逻辑:使用FileSenderThread通过8887端口发送文件
                if (m_jsonData.isEmpty()) {
                    ui->label_info->setText("错误: 升级文件内容为空，无法发送");
                    return;
                }

                // 显示并重置进度条
                ui->progressBar_upgrade->setVisible(true);
                ui->progressBar_upgrade->setValue(0);

                // 创建文件发送线程（连接 8887 端口）
                //注意内存泄露
                auto* senderThread = new FileSenderThread(m_ipAddress, m_jsonData, this);
                // 连接线程信号与槽函数
                connect(senderThread, &FileSenderThread::sendSuccess, this, &LinkPush::onFileSendSuccess);
                connect(senderThread, &FileSenderThread::sendError, this, &LinkPush::onFileSendError);
                // 设置线程结束后自动删除
                connect(senderThread, &QThread::finished, senderThread, &QObject::deleteLater);
                // 新增：连接进度更新信号
                connect(senderThread, &FileSenderThread::sendProgress, this, &LinkPush::onFileSendProgress);
                // 启动线程
                senderThread->start();
            } else {
                ui->label_info->setText(QString("设备 %1 已是最新版本").arg(m_ipAddress));
                // 弹窗确认后打开登录窗口，关闭当前窗口
                QMessageBox::information(this, "提示", "设备已是最新版本，即将进入登录界面", QMessageBox::Ok);
                // 打开登录窗口，传入设备IP和topic
                auto* loginWindow = new Login(nullptr, m_ipAddress, m_topic);
                loginWindow->show();
                // 关闭当前LinkPush窗口
                parent->close();
                this->close();
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
                // 弹窗确认后打开登录窗口，关闭当前窗口
                QMessageBox::information(this, "成功", "设备升级成功，即将进入登录界面", QMessageBox::Ok);
                auto* loginWindow = new Login(nullptr, m_ipAddress, m_topic);
                loginWindow->show();
                parent->close();
                this->close();
            } else {
                ui->label_info->setText(QString("设备 %1 升级失败").arg(m_ipAddress));
                // 弹窗选择：确认打开登录窗口，取消则退出
                int ret = QMessageBox::question(this, "失败", "设备升级失败，是否进入登录界面？",
                                                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
                if (ret == QMessageBox::Yes) {
                    auto* loginWindow = new Login(nullptr, m_ipAddress, m_topic);
                    loginWindow->show();
                }
                parent->close();
                this->close(); // 无论是否打开登录窗口，都关闭当前窗口
            }
        }
        break;
    }
    default:
        ui->label_info->setText(QString("收到未知类型的数据包: %1").arg(type));
        // 弹窗确认后关闭当前窗口退出
        QMessageBox::warning(this, "错误", "收到未知类型数据包，即将退出", QMessageBox::Ok);
        parent->close();
        this->close();
        break;
    }
}

// 新增槽函数：处理文件发送进度更新
void LinkPush::onFileSendProgress(qint64 sent, qint64 total) {
    if (total > 0) {
        int progress = static_cast<int>(sent * 100 / total);
        ui->progressBar_upgrade->setValue(progress);
        ui->label_info->setText(QString("正在发送升级文件: %1%").arg(progress));
    }
}

// 修改文件发送成功回调，更新进度条
void LinkPush::onFileSendSuccess() {
    ui->progressBar_upgrade->setValue(100);
    ui->label_info->setText(QString("设备 %1 的升级文件发送成功").arg(m_ipAddress));
}

// 修改文件发送失败回调，重置进度条
void LinkPush::onFileSendError(const QString& errorMsg) {
    ui->progressBar_upgrade->setValue(0);
    ui->progressBar_upgrade->setVisible(false);
    ui->label_info->setText(QString("设备 %1 的升级文件发送失败: %2").arg(m_ipAddress, errorMsg));
}