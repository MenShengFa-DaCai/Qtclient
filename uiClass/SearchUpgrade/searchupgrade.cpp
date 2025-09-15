// searchupgrade.cpp
#include "searchupgrade.h"           // 包含自定义头文件
#include "ui_searchupgrade.h"        // 包含UI自动生成的头文件
#include <QNetworkInterface>         // 用于获取网络接口信息
#include <QMessageBox>               // 用于显示消息框
#include <QTimer>                    // 用于定时器功能

// 构造函数
SearchUpgrade::SearchUpgrade(QWidget* parent) :
    QWidget(parent),                 // 调用基类构造函数
    ui(new Ui::SearchUpgrade),       // 初始化UI对象
    latestFirmwareVersion("v2.1.0")  // 初始化最新固件版本号为v2.1.0
{
    ui->setupUi(this);               // 设置UI界面

    // 设置窗口属性
    setWindowTitle("固件升级搜索");   // 设置窗口标题

    // 初始化UDP socket
    udpSocket = new QUdpSocket(this);  // 创建UDP套接字对象，并设置父对象为当前窗口
    // 尝试绑定到任意IPv4地址和随机端口
    if (!udpSocket->bind(QHostAddress::AnyIPv4, 0)) {
        // 如果绑定失败，显示警告消息框
        QMessageBox::warning(this, "错误", "无法绑定UDP端口");
    }

    // 连接信号和槽函数
    // 当UDP套接字有数据可读时，调用readPendingDatagrams函数
    connect(udpSocket, &QUdpSocket::readyRead, this, &SearchUpgrade::readPendingDatagrams);
    // 当关闭按钮被点击时，调用onCloseClicked函数
    connect(ui->pushButtonClose, &QPushButton::clicked, this, &SearchUpgrade::onCloseClicked);
    // 当列表控件中的项被双击时，调用onClientDoubleClicked函数
    connect(ui->listWidgetClients, &QListWidget::itemDoubleClicked, this, &SearchUpgrade::onClientDoubleClicked);

    // 延迟100毫秒发送广播，确保UI已完全加载
    QTimer::singleShot(100, this, &SearchUpgrade::sendBroadcast);
}

// 析构函数
SearchUpgrade::~SearchUpgrade() {
    // 断开所有TCP连接
    // 遍历tcpSockets映射表，断开每个TCP连接
    for (auto it = tcpSockets.begin(); it != tcpSockets.end(); ++it) {
        it.key()->disconnectFromHost();  // 断开TCP连接
        it.key()->deleteLater();         // 延迟删除TCP套接字对象
    }

    delete ui;  // 删除UI对象，释放内存
}

// 发送UDP广播函数
void SearchUpgrade::sendBroadcast() {
    ui->labelStatus->setText("正在发送广播搜索设备...");  // 更新状态标签文本

    QByteArray datagram = "SearchUpgrade";  // 创建要发送的数据报内容
    bool broadcastSent = false;             // 标记是否成功发送广播

    // 获取所有网络接口
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    // 遍历所有网络接口
    for (const QNetworkInterface &interface : interfaces) {
        // 检查接口是否启用、正在运行且不是回环接口
        if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
            interface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {

            // 获取接口的所有IP地址条目
            QList<QNetworkAddressEntry> entries = interface.addressEntries();
            // 遍历所有IP地址条目
            for (const QNetworkAddressEntry &entry : entries) {
                // 只处理IPv4地址
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    QHostAddress broadcastAddress = entry.broadcast();  // 获取广播地址
                    if (!broadcastAddress.isNull()) {  // 如果广播地址有效
                        // 发送UDP数据报到广播地址和指定端口
                        udpSocket->writeDatagram(datagram, broadcastAddress, PORT);
                        broadcastSent = true;  // 标记已发送广播
                    }
                }
            }
        }
    }

    // 根据广播发送结果更新状态标签
    if (!broadcastSent) {
        ui->labelStatus->setText("无法发送广播，请检查网络连接");
    } else {
        ui->labelStatus->setText("广播已发送，等待设备响应...");
    }
}

// 读取UDP待处理数据报函数
void SearchUpgrade::readPendingDatagrams() {
    // 循环处理所有待处理的UDP数据报
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;  // 存储接收到的数据
        // 调整数据报大小为待处理数据报的大小
        datagram.resize(udpSocket->pendingDatagramSize());
        QHostAddress sender;  // 存储发送方地址
        quint16 senderPort;   // 存储发送方端口

        // 读取数据报到datagram，并获取发送方地址和端口
        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        // 检查数据报内容是否为"Client"且发送方使用IPv4协议
        if (datagram == "Client" && sender.protocol() == QAbstractSocket::IPv4Protocol) {
            // 更新状态标签显示发现的设备
            ui->labelStatus->setText(QString("发现设备: %1").arg(sender.toString()));
            // 连接到发现的客户端
            connectToClient(sender.toString());
        }
    }
}

// 连接到客户端函数
void SearchUpgrade::connectToClient(const QString& ipAddress) {
    auto* tcpSocket = new QTcpSocket(this);  // 创建新的TCP套接字
    // 将TCP套接字和IP地址添加到映射表，主题初始为空字符串
    tcpSockets.insert(tcpSocket, QPair<QString, QString>(ipAddress, ""));

    // 连接TCP套接字的信号到相应的槽函数
    connect(tcpSocket, &QTcpSocket::connected, this, &SearchUpgrade::tcpConnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &SearchUpgrade::tcpReadyRead);
    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &SearchUpgrade::tcpErrorOccurred);

    // 尝试连接到指定IP地址和端口的客户端
    tcpSocket->connectToHost(ipAddress, PORT);
}

// TCP连接建立成功槽函数
void SearchUpgrade::tcpConnected() {
    // 获取发送信号的TCP套接字对象
    auto* tcpSocket = qobject_cast<QTcpSocket*>(sender());
    // 检查TCP套接字是否有效且在映射表中
    if (tcpSocket && tcpSockets.contains(tcpSocket)) {
        // 更新状态标签显示连接成功
        ui->labelStatus->setText(QString("已连接到: %1").arg(tcpSockets.value(tcpSocket).first));
    }
}

// TCP数据可读槽函数
void SearchUpgrade::tcpReadyRead() {
    // 获取发送信号的TCP套接字对象
    auto* tcpSocket = qobject_cast<QTcpSocket*>(sender());
    // 检查TCP套接字是否有效且在映射表中，如果无效则返回
    if (!tcpSocket || !tcpSockets.contains(tcpSocket)) return;

    // 获取客户端信息引用
    QPair<QString, QString>& clientInfo = tcpSockets[tcpSocket];
    // 读取所有可用数据并转换为字符串，去除首尾空白字符
    QString data = QString::fromUtf8(tcpSocket->readAll()).trimmed();

    // 检查是否已收到主题信息
    if (clientInfo.second.isEmpty()) {
        // 第一次读取，应该是MQTT主题
        clientInfo.second = data;  // 保存主题信息
        // 发送最新固件版本号给客户端
        tcpSocket->write(latestFirmwareVersion.toUtf8());
    } else {
        // 第二次读取，应该是客户端固件版本号
        QString ip = clientInfo.first;        // 获取IP地址
        QString topic = clientInfo.second;    // 获取主题
        const QString& clientVersion = data;  // 获取客户端版本号

        // 创建显示文本，格式为"IP地址 主题 客户端版本号"
        QString displayText = QString("%1 %2 %3").arg(ip, topic, clientVersion);
        // 将显示文本添加到列表控件
        ui->listWidgetClients->addItem(displayText);

        // 更新状态标签显示获取到的设备信息
        ui->labelStatus->setText(QString("获取到设备信息: %1").arg(displayText));

        // 断开TCP连接
        tcpSocket->disconnectFromHost();
        // 从映射表中移除TCP套接字
        tcpSockets.remove(tcpSocket);
        // 延迟删除TCP套接字对象
        tcpSocket->deleteLater();
    }
}

// TCP错误发生槽函数
void SearchUpgrade::tcpErrorOccurred(QAbstractSocket::SocketError error) {
    // 获取发送信号的TCP套接字对象
    auto* tcpSocket = qobject_cast<QTcpSocket*>(sender());
    // 检查TCP套接字是否有效且在映射表中
    if (tcpSocket && tcpSockets.contains(tcpSocket)) {
        // 更新状态标签显示错误信息
        ui->labelStatus->setText(QString("连接错误: %1").arg(tcpSocket->errorString()));
        // 从映射表中移除TCP套接字
        tcpSockets.remove(tcpSocket);
        // 延迟删除TCP套接字对象
        tcpSocket->deleteLater();
    }
}

// 关闭按钮点击槽函数
void SearchUpgrade::onCloseClicked() {
    close();  // 关闭窗口
}

// 客户端列表项双击槽函数
void SearchUpgrade::onClientDoubleClicked(QListWidgetItem* item) {
    // 获取双击的项目文本
    QString clientInfo = item->text();
    // 按空格分割文本为部分
    QStringList parts = clientInfo.split(" ");
    // 检查是否有足够的部分(至少3部分: IP, 主题, 版本号)
    if (parts.size() >= 3) {
        QString ip = parts[0];      // 第一部分是IP地址
        QString topic = parts[1];   // 第二部分是主题
        QString version = parts[2]; // 第三部分是版本号

        // 这里可以添加代码来打开连接和推送升级固件的窗口
        // 暂时注释掉的代码示例:
        // QMessageBox::information(this, "设备详情",
        //     QString("IP: %1\n主题: %2\n版本: %3").arg(ip).arg(topic).arg(version));

        // 暂时显示消息框提示功能待实现
        QMessageBox::information(this, "功能待实现",
            "双击设备功能待实现，这里将打开连接和推送升级固件的窗口");
    }
}