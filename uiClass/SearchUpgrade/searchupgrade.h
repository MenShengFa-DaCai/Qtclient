// searchupgrade.h
#ifndef QTCLIENT_SEARCHUPGRADE_H  // 防止头文件被重复包含的预处理器指令
#define QTCLIENT_SEARCHUPGRADE_H

#define PORT 8888  // 定义通信端口号为8888

#include <QWidget>          // 包含QWidget类，用于创建GUI窗口
#include <QUdpSocket>       // 包含QUdpSocket类，用于UDP通信
#include <QTcpSocket>       // 包含QTcpSocket类，用于TCP通信
#include <QListWidgetItem>  // 包含QListWidgetItem类，用于列表项操作

QT_BEGIN_NAMESPACE  // 开始Qt命名空间

// 前置声明Ui命名空间中的SearchUpgrade类
// 这个类是由Qt Designer自动生成的，对应UI文件中的界面元素
namespace Ui {
    class SearchUpgrade;
}

QT_END_NAMESPACE  // 结束Qt命名空间

// SearchUpgrade类定义，继承自QWidget
class SearchUpgrade : public QWidget {
    Q_OBJECT  // Qt宏，启用信号槽机制和元对象系统

public:
    // 构造函数
    // parent参数指向父窗口部件，默认为nullptr表示没有父窗口
    explicit SearchUpgrade(QWidget* parent = nullptr);

    // 析构函数，override关键字表示重写基类的虚函数
    ~SearchUpgrade() override;

private slots:  // 私有槽函数，用于响应各种事件
    // 关闭按钮点击事件的槽函数
    void onCloseClicked();

    /**
    * @brief 客户端列表项双击事件的槽函数
    * @param item 指向被双击的列表项
    */
    void onClientDoubleClicked(QListWidgetItem* item);

    // 读取UDP待处理数据报的槽函数
    void readPendingDatagrams();

    // TCP连接建立成功的槽函数
    void tcpConnected();

    // 处理TCP断开连接
    void tcpDisconnected();

    // TCP数据可读的槽函数
    void tcpReadyRead();

    /**
     * @brief TCP错误发生的槽函数
     * @param error 表示发生的具体错误类型
     */
    void tcpErrorOccurred(QAbstractSocket::SocketError error);

private:
    Ui::SearchUpgrade* ui;  // 指向UI界面对象的指针

    // UDP套接字指针，用于发送广播和接收响应
    QUdpSocket* udpSocket;

    // TCP套接字映射表，用于管理多个TCP连接
    // 键: QTcpSocket指针，值: QPair<IP地址, MQTT主题>
    QMap<QTcpSocket*, QPair<QString, QString>> tcpSockets;

    // 存储最新固件版本号的字符串
    QString latestFirmwareVersion;

    // 私有成员函数
    // 发送UDP广播数据包
    void sendBroadcast();

    // 连接到指定IP地址的客户端
    // ipAddress参数表示客户端的IP地址
    void connectToClient(const QString& ipAddress);
};

#endif //QTCLIENT_SEARCHUPGRADE_H  // 结束头文件保护条件