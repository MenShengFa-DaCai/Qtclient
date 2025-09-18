#ifndef QTCLIENT_LOGIN_H
#define QTCLIENT_LOGIN_H

#define MQTT_PORT 1883  // 定义MQTT协议默认端口号

#include <QWidget>
#include <QMqttClient>  // MQTT客户端类，用于MQTT通信
#include <QJsonObject>  // JSON对象类，用于处理JSON数据
#include <QJsonDocument> // JSON文档类，用于JSON序列化和反序列化

QT_BEGIN_NAMESPACE

namespace Ui {
    class Login;
}

QT_END_NAMESPACE

/**
 * @brief 用户登录窗口类
 *
 * 负责处理用户登录逻辑，通过MQTT协议与设备进行认证通信
 */
class Login : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     * @param ip MQTT代理服务器IP地址
     * @param topic MQTT主题前缀
     */
    explicit Login(QWidget* parent = nullptr, QString ip = "0.0.0.0", QString topic = "0");

    /**
     * @brief 析构函数
     */
    ~Login() override;

private slots:
    /**
     * @brief 登录按钮点击槽函数
     */
    void onLoginButtonClicked();

    /**
     * @brief MQTT连接状态改变槽函数
     * @param state MQTT客户端状态
     */
    void onMqttStateChanged(QMqttClient::ClientState state);

    /**
     * @brief MQTT消息接收槽函数
     * @param message 接收到的消息内容
     * @param topic 消息来源的主题
     */
    void onMqttMessageReceived(const QByteArray &message, const QMqttTopicName &topic);

private:
    Ui::Login* ui;          // UI界面对象指针
    QString ip;             // MQTT代理服务器IP地址
    QString topic;          // MQTT主题前缀
    QMqttClient* mqttClient=nullptr; // MQTT客户端对象
    bool isConnected;       // MQTT连接状态标志

    /**
     * @brief 初始化MQTT客户端
     */
    void initMqttClient();

    /**
     * @brief 发送登录信息
     * @param username 用户名
     * @param password 密码
     */
    void sendLoginInfo(const QString& username, const QString& password);

    /**
     * @brief 处理登录响应
     * @param response JSON格式的响应数据
     */
    void handleLoginResponse(const QJsonObject& response);
};

#endif // QTCLIENT_LOGIN_H