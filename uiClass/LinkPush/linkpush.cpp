#include "linkpush.h"
#include "ui_LinkPush.h"
#include "../Login/login.h"
#include <QMessageBox>


LinkPush::LinkPush(QWidget* parent) :
    QWidget(parent), ui(new Ui::LinkPush), m_tcpSocket(nullptr) {
    ui->setupUi(this);
}

LinkPush::~LinkPush() {
    if (m_tcpSocket) {
        m_tcpSocket->disconnectFromHost();
        m_tcpSocket->deleteLater();
    }
    delete ui;
}

void LinkPush::setLinkInfo(const QString& ip, QTcpSocket* socket, const QString& topic,const QString& version) {
    m_ipAddress = ip;
    m_tcpSocket = socket;
    m_topic = topic;

    // 显示基础信息
    ui->label_info->setText(QString("设备IP: %1\n主题: %2\n\n正在发送最新版本信息...").arg(ip, topic));

    // 绑定TCP接收信号
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &LinkPush::onTcpReadyRead);

    // 发送版本号（实际项目中版本号建议从配置/全局变量获取）
    if (m_tcpSocket->state() == QTcpSocket::ConnectedState) {
        m_tcpSocket->write(version.toUtf8());
        ui->label_info->setText("已发送版本号，等待设备回复...");
    }
    else {
        QMessageBox::critical(this, "错误", "TCP连接未建立，无法通信！");
    }
}

void LinkPush::onTcpReadyRead() {
    if (!m_tcpSocket) return;

    QString response = m_tcpSocket->readAll().trimmed();
    ui->label_info->setText(QString("\n设备回复: %1").arg(response));

    // 1. 设备同意升级（Y）：显示进度条+模拟升级+完成弹窗
    if (response == "Y") {
        ui->progressBar_upgrade->setVisible(true); // 显示进度条
        ui->label_info->setText("设备同意升级，开始执行升级流程...");
        QMessageBox::information(this, "升级结果", "升级完成！", QMessageBox::Ok);

        // --------------------------
        // 预留：升级完成后的后续逻辑
        // TODO: 1. 记录设备升级成功日志（如写入本地文件/数据库）
        // TODO: 2. 向服务器上报升级结果（若有服务端）
        // TODO: 3. 重启设备（若需）或刷新设备状态
        // TODO: 4. 关闭当前窗口或返回设备列表
        // --------------------------
    }
    // 2. 设备拒绝升级（N）：直接弹窗提示
    else if (response == "N") {
        // 无需升级弹窗
        QMessageBox::information(this, "升级结果", "设备无需升级！", QMessageBox::Ok);

        // --------------------------
        // 预留：无需升级后的后续逻辑
        // TODO: 1. 记录设备拒绝升级原因（若设备返回原因）
        // TODO: 2. 维持TCP连接以进行其他操作（如推送配置）
        // TODO: 3. 关闭窗口或返回设备列表
        // --------------------------

        // 3. 无效回复处理
    }
    else {
        QMessageBox::warning(this, "错误", "收到无效回复，请重试！", QMessageBox::Ok);

        // --------------------------
        // 预留：无效回复后的后续逻辑
        // TODO: 1. 重新发送版本号（限3次，避免死循环）
        // TODO: 2. 断开TCP连接并提示用户检查设备
        // --------------------------
    }
    //打开登陆界面，关闭前面的界面，将网关IP地址存下，放在下面的登陆界面对象里
    Login login(nullptr,m_ipAddress,m_topic);
    login.show();
}
