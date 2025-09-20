#include "searchupgrade.h"
#include "ui_searchupgrade.h"
#include <QNetworkInterface>  // 包含网络接口相关类，用于获取网络信息
#include <QMessageBox>  // 包含消息框类，用于显示提示/错误信息
#include <QTimer>
#include <QJsonDocument>  // 包含JSON文档类，用于JSON数据的序列化/反序列化
#include <QJsonObject>  // 包含JSON对象类，用于处理JSON键值对
#include "../LinkPush/linkpush.h"

/**
 * @brief SearchUpgrade类的构造函数
 * @param parent 父窗口指针
 * @details 初始化UI、设置窗口标题、创建UDP套接字、绑定信号槽、启动广播定时器
 */
SearchUpgrade::SearchUpgrade(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::SearchUpgrade),
    latestFirmwareVersion("v1.0") {  // 初始化最新固件版本为v1.0
    ui->setupUi(this);
    setWindowTitle("搜索网关");  // 设置窗口标题

    udpSocket = new QUdpSocket(this);  // 实例化UDP套接字，父对象为当前窗口（自动管理生命周期）

    /**
     * @brief QUdpSocket::bind()函数
     * @param address 绑定的地址（任意IPv4地址）
     * @param port 绑定的端口（0表示随机分配）
     * @return 绑定成功返回true，失败返回false
     * @details 绑定UDP套接字到本地任意IPv4地址的随机端口，用于接收设备响应
     */

    if (!udpSocket->bind(QHostAddress::AnyIPv4, 0)) {
        /**
         * @brief QMessageBox::warning()函数
         * @param parent 父窗口指针
         * @param title 消息框标题
         * @param text 消息内容
         * @details 当UDP端口绑定失败时，显示警告消息框
         */
        QMessageBox::warning(this, "错误", "无法绑定UDP端口");
    }

    /**
     * @brief QObject::connect()函数（绑定udpSocket的readyRead信号）
     * @param sender 信号发送者（udpSocket）
     * @param signal 发送的信号（QUdpSocket::readyRead）
     * @param receiver 信号接收者（this）
     * @param method 接收的槽函数（&SearchUpgrade::readPendingDatagrams）
     * @details 当UDP套接字有可读数据时，触发readPendingDatagrams槽函数
     */
    connect(udpSocket, &QUdpSocket::readyRead, this, &SearchUpgrade::readPendingDatagrams);
    connect(ui->pushButtonClose, &QPushButton::clicked, this, &SearchUpgrade::onCloseClicked);
    connect(ui->listWidgetClients, &QListWidget::itemDoubleClicked, this, &SearchUpgrade::onClientDoubleClicked);
    /**
     * @brief QTimer::singleShot()函数
     * @param msec 延迟时间（毫秒）
     * @param receiver 接收者（this）
     * @param member 要执行的槽函数（&SearchUpgrade::sendBroadcast）
     * @details 延迟1000毫秒后，执行sendBroadcast函数发送搜索广播
     */
    QTimer::singleShot(1000, this, &SearchUpgrade::sendBroadcast);
}

/**
 * @brief SearchUpgrade类的析构函数
 * @details 释放UI对象资源
 */
SearchUpgrade::~SearchUpgrade() {
    delete ui;  // 释放UI对象
}

/**
 * @brief 发送UDP广播搜索设备
 * @details 构建JSON格式的搜索请求，遍历所有活动网络接口的IPv4广播地址并发送
 */
void SearchUpgrade::sendBroadcast() {
    ui->labelStatus->setText("正在发送广播搜索设备...");  // 更新状态标签显示

    QJsonObject jsonObj;  // 创建JSON对象存储请求信息
    jsonObj["type"] = 0;  // 设置消息类型为0（搜索请求）
    jsonObj["request"] = "connect_info";  // 设置请求内容为获取连接信息

    QJsonDocument jsonDoc(jsonObj);  // 将JSON对象转换为JSON文档
    QByteArray datagram = jsonDoc.toJson(QJsonDocument::Compact);  // 将JSON文档转换为紧凑格式的字节数组

    bool broadcastSent = false;  // 标记是否成功发送广播
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();  // 获取所有网络接口

    // 遍历网络接口
    for (const QNetworkInterface& interface : interfaces) {
        // 筛选处于启动且运行中、非回环的接口
        if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
            interface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {

            QList<QNetworkAddressEntry> entries = interface.addressEntries();  // 获取接口的地址条目
            // 遍历地址条目
            for (const QNetworkAddressEntry& entry : entries) {
                // 筛选IPv4协议的地址
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    QHostAddress broadcastAddress = entry.broadcast();  // 获取广播地址
                    if (!broadcastAddress.isNull()) {
                        /**
                         * @brief 首次调用QUdpSocket::writeDatagram()函数
                         * @param data 要发送的数据（广播内容）
                         * @param address 目标广播地址
                         * @param port 目标端口（PORT=8888）
                         * @return 发送的字节数，失败返回-1
                         * @details 向广播地址发送UDP数据报
                         */
                        udpSocket->writeDatagram(datagram, broadcastAddress, PORT);
                        broadcastSent = true;  // 标记广播发送成功
                    }
                }
            }
        }
    }

    // 根据广播发送结果更新状态标签
    if (!broadcastSent) {
        ui->labelStatus->setText("无法发送广播，请检查网络连接");
    }
    else {
        ui->labelStatus->setText("广播已发送，等待设备响应...");
    }
}

/**
 * @brief 读取等待的UDP数据报并解析
 * @details 循环读取所有待处理的数据报，解析JSON格式的设备响应，提取设备信息并添加到列表
 */
void SearchUpgrade::readPendingDatagrams() {
    // 循环读取所有待处理的数据报
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram="";  // 存储接收的数据
        datagram.resize(udpSocket->pendingDatagramSize());  // 调整数据大小为待接收数据报的大小
        QHostAddress sender;  // 存储发送者地址
        quint16 senderPort;  // 存储发送者端口

        /**
         * @brief 首次调用QUdpSocket::readDatagram()函数
         * @param data 接收数据的缓冲区
         * @param maxSize 缓冲区最大大小
         * @param sender 发送者地址输出参数
         * @param senderPort 发送者端口输出参数
         * @return 实际读取的字节数，失败返回-1
         * @details 从UDP套接字读取数据报内容及发送者信息
         */
        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        QJsonParseError error;  // 存储JSON解析错误信息
        /**
         * @brief 首次调用QJsonDocument::fromJson()函数
         * @param json 待解析的JSON字节数组
         * @param error 解析错误信息输出参数
         * @return 解析后的QJsonDocument对象
         * @details 将接收到的字节数组解析为JSON文档
         */
        QJsonDocument jsonDoc = QJsonDocument::fromJson(datagram, &error);

        // 检查解析是否成功且为JSON对象
        if (error.error == QJsonParseError::NoError && jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();  // 获取JSON对象

            // 检查消息类型是否为0（设备响应）且发送者为IPv4
            if (jsonObj.contains("type") && jsonObj["type"].toInt() == 0 &&
                sender.protocol() == QAbstractSocket::IPv4Protocol) {

                // 检查是否包含data字段且为对象
                if (jsonObj.contains("data") && jsonObj["data"].isObject()) {
                    QJsonObject dataObj = jsonObj["data"].toObject();  // 获取data对象
                    QString mqttTopic = dataObj["mqtt_topic_report"].toString();  // 提取MQTT主题
                    QString ipAddress = sender.toString();  // 获取发送者IP地址

                    ui->labelStatus->setText(QString("发现设备: %1").arg(ipAddress));  // 更新状态标签

                    // 构建列表项显示文本（IP地址 + MQTT主题）
                    QString displayText = QString("%1 %2").arg(ipAddress, mqttTopic);
                    /**
                     * @brief 首次调用QListWidget::addItem()函数
                     * @param text 列表项显示文本
                     * @details 向列表控件添加设备信息项
                     */
                    ui->listWidgetClients->addItem(displayText);
                }
            }
        }
    }
}

/**
 * @brief 关闭按钮点击事件处理
 * @details 关闭当前窗口
 */
void SearchUpgrade::onCloseClicked() {
    close();  // 调用QWidget的close()方法关闭窗口
}

/**
 * @brief 列表项双击事件处理
 * @param item 被双击的列表项
 * @details 解析列表项中的IP和MQTT主题，创建LinkPush窗口并连接设备
 */
void SearchUpgrade::onClientDoubleClicked(QListWidgetItem* item) {
    QString clientInfo = item->text();  // 获取列表项文本
    QStringList parts = clientInfo.split(" ");  // 按空格分割文本（IP和MQTT主题）
    if (parts.size() >= 2) {  // 确保分割结果有效
        const QString& ip = parts[0];  // 提取IP地址
        const QString& topic = parts[1];  // 提取MQTT主题

        auto *linkPush = new LinkPush(this);  // 实例化LinkPush对象（父对象为当前窗口）
        linkPush->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);  // 设置窗口标志（对话框样式，显示标题）
        linkPush->connectToDevice(ip, topic);  // 调用LinkPush的方法连接设备
        linkPush->show();  // 显示LinkPush窗口
    }
}