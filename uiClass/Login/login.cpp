#include "login.h"
#include "ui_Login.h"
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <utility>
#include "../MainWidget/mainwidget.h"

/**
 * @brief 构造函数
 * @param parent 父窗口指针
 * @param ip MQTT代理服务器IP地址
 * @param topic MQTT主题前缀
 */
Login::Login(QWidget* parent, QString ip, QString topic) :
    QWidget(parent), ui(new Ui::Login), ip(std::move(ip)), topic(std::move(topic)), isConnected(false) {
    ui->setupUi(this);

    // 设置窗口标题
    setWindowTitle("用户登录");

    // 初始化UI组件
    ui->lineEditUsername->setPlaceholderText("请输入用户名");
    ui->lineEditPassword->setPlaceholderText("请输入密码");
    ui->lineEditPassword->setEchoMode(QLineEdit::Password);  // 密码输入模式

    // 连接登录按钮信号槽
    connect(ui->pushButtonLogin, &QPushButton::clicked, this, &Login::onLoginButtonClicked);

    // 初始化MQTT客户端
    initMqttClient();
}

/**
 * @brief 析构函数
 */
Login::~Login() {
    if (mqttClient) {
        mqttClient->disconnectFromHost();
        delete mqttClient;
    }
    delete ui;
}

/**
 * @brief 初始化MQTT客户端
 */
void Login::initMqttClient() {
    mqttClient = new QMqttClient(this);
    mqttClient->setHostname(ip);
    mqttClient->setPort(MQTT_PORT);

    // 连接MQTT信号槽
    connect(mqttClient, &QMqttClient::stateChanged, this, &Login::onMqttStateChanged);
    connect(mqttClient, &QMqttClient::messageReceived, this, &Login::onMqttMessageReceived);

    // 连接到MQTT代理
    mqttClient->connectToHost();
}

/**
 * @brief MQTT连接状态改变槽函数
 * @param state MQTT客户端状态
 */
void Login::onMqttStateChanged(QMqttClient::ClientState state) {
    switch (state) {
    case QMqttClient::Connected:
        isConnected = true;
        ui->labelStatus->setText("已连接到MQTT代理");
        break;
    case QMqttClient::Connecting:
        ui->labelStatus->setText("正在连接MQTT代理...");
        break;
    case QMqttClient::Disconnected:
        isConnected = false;
        ui->labelStatus->setText("与MQTT代理断开连接");
        break;
    default:
        break;
    }
}

/**
 * @brief 登录按钮点击槽函数
 */
void Login::onLoginButtonClicked() {
    QString username = ui->lineEditUsername->text().trimmed();
    QString password = ui->lineEditPassword->text().trimmed();

    // 验证输入
    if (username.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "用户名不能为空");
        return;
    }
    if (password.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "密码不能为空");
        return;
    }
    if (!isConnected) {
        QMessageBox::warning(this, "连接错误", "未连接到MQTT代理，请检查网络连接");
        return;
    }

    // 发送登录信息
    sendLoginInfo(username, password);
    ui->labelStatus->setText("正在验证登录信息...");
}

/**
 * @brief 发送登录信息
 * @param username 用户名
 * @param password 密码
 */
void Login::sendLoginInfo(const QString& username, const QString& password) {
    // 创建JSON对象
    QJsonObject loginData;
    loginData["username"] = username;
    loginData["password"] = password;
    loginData["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // 转换为JSON文档
    QJsonDocument doc(loginData);
    QByteArray payload = doc.toJson(QJsonDocument::Compact);

    // 发布到MQTT主题
    QMqttTopicName publishTopic = QString("down");
    if (mqttClient->publish(publishTopic, payload) == -1) {
        QMessageBox::warning(this, "发送错误", "无法发送登录信息");
        return;
    }

    // 订阅响应主题
    QMqttTopicFilter responseTopic = topic;  // 使用QMqttTopicFilter
    auto subscription = mqttClient->subscribe(responseTopic);
    if (!subscription) {
        QMessageBox::warning(this, "订阅错误", "无法订阅响应主题");
        return;
    }
}

/**
 * @brief MQTT消息接收槽函数
 * @param message 接收到的消息内容
 * @param topic 消息来源的主题
 */
void Login::onMqttMessageReceived(const QByteArray &message, const QMqttTopicName &topic) {
    Q_UNUSED(topic)

    // 解析JSON响应
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(message, &error);

    if (error.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, "响应错误", "收到无效的响应格式");
        return;
    }

    if (!doc.isObject()) {
        QMessageBox::warning(this, "响应错误", "响应数据格式不正确");
        return;
    }

    // 处理登录响应
    handleLoginResponse(doc.object());
}

/**
 * @brief 处理登录响应
 * @param response JSON格式的响应数据
 */
void Login::handleLoginResponse(const QJsonObject& response) {
    // 检查响应结构
    if (!response.contains("status") || !response.contains("message")) {
        QMessageBox::warning(this, "响应错误", "响应数据不完整");
        return;
    }

    QString status = response["status"].toString();
    QString message = response["message"].toString();

    if (status == "success") {
        QMessageBox::information(this, "登录成功", message);
        // 跳转到主界面或执行其他操作
        auto* mainWidget = new MainWidget(nullptr,ip,topic);
        mainWidget->show();
        // 关闭登录窗口
        close();
    } else if (status == "error") {
        QMessageBox::warning(this, "登录失败", message);
        ui->lineEditPassword->clear();
    } else {
        QMessageBox::warning(this, "响应错误", "未知的响应状态");
    }

    ui->labelStatus->setText("就绪");
}