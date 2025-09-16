#include "searchupgrade.h"
#include "ui_searchupgrade.h"
#include <QNetworkInterface>         // 用于获取网络接口信息
#include <QMessageBox>               // 用于显示消息框
#include <QTimer>                    // 用于定时器功能
#include "../LinkPush/linkpush.h"

// 构造函数
SearchUpgrade::SearchUpgrade(QWidget* parent) :
    // 调用基类构造函数
    QWidget(parent),
    // 初始化UI对象
    ui(new Ui::SearchUpgrade),
    // 初始化最新固件版本号为v1.0
    latestFirmwareVersion("v1.0") {
    // 设置UI界面
    ui->setupUi(this);

    // 设置窗口属性
    setWindowTitle("固件升级搜索"); // 设置窗口标题

    // 初始化UDP socket
    udpSocket = new QUdpSocket(this); // 创建UDP套接字对象，并设置父对象为当前窗口
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
    // 延迟1000毫秒发送广播，确保UI已完全加载
    QTimer::singleShot(1000, this, &SearchUpgrade::sendBroadcast);
}

// 析构函数
SearchUpgrade::~SearchUpgrade() {
    // 断开所有TCP连接
    // 遍历tcpSockets映射表，断开每个TCP连接
    for (auto it = tcpSockets.begin(); it != tcpSockets.end(); ++it) {
        it.key()->disconnectFromHost(); // 断开TCP连接
        it.key()->deleteLater(); // 延迟删除TCP套接字对象
    }
    delete ui; // 删除UI对象，释放内存
}

// 发送UDP广播函数
void SearchUpgrade::sendBroadcast() {
    ui->labelStatus->setText("正在发送广播搜索设备..."); // 更新状态标签文本

    QByteArray datagram = "SearchUpgrade"; // 创建要发送的数据报内容
    bool broadcastSent = false; // 标记是否成功发送广播
    /*
     *获取所有网络接口
     *Qt 提供的静态函数，用于获取当前设备上所有的网络接口（如以太网、Wi-Fi、虚拟机网卡等），
     *返回值是QList<QNetworkInterface>类型的列表。
     */
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    /*遍历所有网络接口
     *QNetworkInterface::flags()：返回网络接口的状态标志（QFlags<QNetworkInterface::InterfaceFlag>类型），
     *用于描述接口的当前状态。
     *testFlag(flag)：检查标志中是否包含某个特定状态（如IsUp、IsRunning等）。
     *QNetworkInterface::IsUp：接口已启用（未被禁用）。
     *QNetworkInterface::IsRunning：接口正在运行（可以传输数据）。
     *QNetworkInterface::IsLoopBack：回环接口（如127.0.0.1，仅用于本地通信，无需发送广播）。
     *筛选逻辑：只处理 “已启用、正在运行且非回环” 的接口，确保广播能通过有效网络发送到外部设备。
     */
    for (const QNetworkInterface& interface : interfaces) {
        if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
            interface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            // 获取接口的所有IP地址条目
            /*
             *QNetworkInterface::addressEntries()：
             *返回当前网络接口关联的所有 IP 地址条目（QNetworkAddressEntry类型），
             *每个条目包含 IP 地址、子网掩码、广播地址等信息。
             */
            QList<QNetworkAddressEntry> entries = interface.addressEntries();
            // 遍历所有IP地址条目
            for (const QNetworkAddressEntry& entry : entries) {
                // 只处理IPv4地址
                //QHostAddress::protocol()：返回 IP 地址的协议类型
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    //QNetworkAddressEntry::broadcast()：返回该 IP 地址对应的广播地址
                    QHostAddress broadcastAddress = entry.broadcast();
                    if (!broadcastAddress.isNull()) {
                        // 如果广播地址有效
                        // 发送UDP数据报到广播地址和指定端口
                        udpSocket->writeDatagram(datagram, broadcastAddress, PORT);
                        broadcastSent = true; // 标记已发送广播
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

// 读取UDP待处理数据报函数
void SearchUpgrade::readPendingDatagrams() {
    //hasPendingDatagrams()判断 UDP 套接字中是否有未处理的数据报（返回true表示有数据可读取）。
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram; // 存储接收到的数据
        // 调整数据报大小为待处理数据报的大小
        //pendingDatagramSize()：返回下一个待处理数据报的字节数（大小）。
        /**
         * QByteArray::resize(int size) 的核心作用是 将字节数组的长度调整为 size 字节。
         * 如果新大小 size 大于当前数组长度：数组会被扩展，新增的字节会被初始化为 '\0'（空字节）。
         * 如果新大小 size 小于当前数组长度：数组会被截断，超出部分的字节会被丢弃。
         */
        datagram.resize(udpSocket->pendingDatagramSize());
        QHostAddress sender; // 存储发送方地址
        quint16 senderPort; // 存储发送方端口

        // 读取数据报到datagram，并获取发送方地址和端口
        /*readDatagram()函数 UDP 读取数据报的核心函数
        * datagram.data()：接收数据的缓冲区指针（QByteArray的底层字节数据）。
        * datagram.size()：缓冲区大小（即前面通过pendingDatagramSize()获取的大小）。
        * &sender：输出参数，接收发送方的 IP 地址。
        * &senderPort：输出参数，接收发送方的端口号。
         */
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
    // 创建新的TCP套接字
    //这里注意内存泄露风险
    //这里不绑定父对象，这些链接要传递到下一个窗口
    auto* tcpSocket = new QTcpSocket();
    // 将TCP套接字和IP地址添加到映射表，主题初始为空字符串
    tcpSockets.insert(tcpSocket, QPair<QString, QString>(ipAddress, ""));

    // 连接TCP套接字的信号到相应的槽函数
    connect(tcpSocket, &QTcpSocket::connected, this, &SearchUpgrade::tcpConnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &SearchUpgrade::tcpReadyRead);
    //链接异常槽函数
    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &SearchUpgrade::tcpErrorOccurred);
    //断开连接槽函数
    connect(tcpSocket,&QTcpSocket::disconnected,this,&SearchUpgrade::tcpDisconnected);
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

//主动断开连接的槽函数
void SearchUpgrade::tcpDisconnected() {
    //和读一样获取链接，sender返回触发槽函数的tcp套接字
    auto* tcpSocket = qobject_cast<QTcpSocket*>(sender());
    //获取到且在表中，contains检查映射表中是否包含指定键，返回布尔
    if (tcpSocket && tcpSockets.contains(tcpSocket)) {
        //arg(...)：QString 的字符串格式化函数
        //value(key) 是映射容器的成员函数，用于根据指定的 key（键）获取对应的 value（值）。
        //first是容器函数，获取第一个值
        ui->labelStatus->setText(QString("与 %1 断开连接").arg(tcpSockets.value(tcpSocket).first));
        //删除
        tcpSockets.remove(tcpSocket);
        /**
         * deleteLater();
         * 不会立即销毁对象，而是向对象所属线程的事件队列中发送一个 “删除事件”，
         * 当事件循环处理到该事件时，才会真正调用 delete 销毁对象。
         */
        tcpSocket->deleteLater();
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
        clientInfo.second = data; // 保存主题信息
        // 发送最新固件版本号给客户端
        tcpSocket->write(latestFirmwareVersion.toUtf8());
    }
    else {
        // 第二次读取，应该是客户端固件版本号
        QString ip = clientInfo.first; // 获取IP地址
        QString topic = clientInfo.second; // 获取主题
        const QString& clientVersion = data; // 获取客户端版本号

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
        ui->labelStatus->setText(QString("连接错误: %1:%2").arg(tcpSocket->errorString()).arg(error));
        // 从映射表中移除TCP套接字
        tcpSockets.remove(tcpSocket);
        // 延迟删除TCP套接字对象
        tcpSocket->deleteLater();
    }
}

// 关闭按钮点击槽函数
void SearchUpgrade::onCloseClicked() {
    close(); // 关闭窗口
}

// 客户端列表项双击槽函数
void SearchUpgrade::onClientDoubleClicked(QListWidgetItem* item) {
    // 获取双击的项目文本
    QString clientInfo = item->text();
    // 按空格分割文本为部分
    QStringList parts = clientInfo.split(" ");
    // 检查是否有足够的部分(至少3部分: IP, 主题, 版本号)
    if (parts.size() >= 3) {
        QString ip = parts[0]; // 第一部分是IP地址
        QString topic = parts[1]; // 第二部分是主题
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
