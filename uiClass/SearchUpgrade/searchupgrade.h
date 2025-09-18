// searchupgrade.h
#ifndef QTCLIENT_SEARCHUPGRADE_H  // 防止头文件重复包含的宏定义
#define QTCLIENT_SEARCHUPGRADE_H

#define PORT 8888

#include <QWidget>
#include <QUdpSocket>
#include <QListWidgetItem>  // 包含QListWidgetItem类，用于列表控件的项操作

QT_BEGIN_NAMESPACE
namespace Ui {
    class SearchUpgrade;
}
QT_END_NAMESPACE

/**
 * @brief 固件升级搜索窗口类，用于通过UDP广播搜索设备并处理升级相关交互
 * @details 继承自QWidget，实现了设备发现、列表展示及升级连接功能
 */
class SearchUpgrade : public QWidget {
    Q_OBJECT  // Qt元对象系统宏，启用信号与槽机制

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针，默认为nullptr
     */
    explicit SearchUpgrade(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     * @details 负责释放UI对象等动态分配的资源
     */
    ~SearchUpgrade() override;

private slots:
    /**
     * @brief 关闭按钮点击事件处理槽函数
     * @details 触发窗口关闭操作
     */
    void onCloseClicked();

    /**
     * @brief 列表项双击事件处理槽函数
     * @param item 被双击的列表项指针
     * @details 用于打开设备连接窗口，进行固件升级相关操作
     */
    void onClientDoubleClicked(QListWidgetItem* item);

    /**
     * @brief 读取等待的数据报槽函数
     * @details 当UDP套接字接收到数据时触发，解析设备响应信息
     */
    void readPendingDatagrams();

private:
    Ui::SearchUpgrade* ui;  // UI界面对象指针，用于访问界面控件
    QUdpSocket* udpSocket;  // UDP套接字指针，用于发送和接收广播数据
    QString latestFirmwareVersion;  // 最新固件版本号

    /**
     * @brief 发送UDP广播搜索设备
     * @details 遍历网络接口，向所有IPv4广播地址发送搜索请求
     */
    void sendBroadcast();
};

#endif //QTCLIENT_SEARCHUPGRADE_H  // 头文件宏定义结束