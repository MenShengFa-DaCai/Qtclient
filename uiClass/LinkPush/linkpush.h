//
// Created by 1 on 15 9月 2025.
//

#ifndef QTCLIENT_LINKPUSH_H
#define QTCLIENT_LINKPUSH_H

#include <QWidget>
#include <QTcpSocket>  // 新增：用于TCP通信

QT_BEGIN_NAMESPACE

namespace Ui {
    class LinkPush;
}

QT_END_NAMESPACE

class LinkPush : public QWidget {
    Q_OBJECT

public:
    explicit LinkPush(QWidget* parent = nullptr);
    ~LinkPush() override;

    // 新增：设置IP地址、TCP套接字和主题信息,版本号
    void setLinkInfo(const QString& ip, QTcpSocket* socket, const QString& topic,const QString& version);

private slots:
    // 新增：处理TCP数据接收
    void onTcpReadyRead();

private:
    Ui::LinkPush* ui;
    QString m_ipAddress;       // 新增：存储设备IP地址
    QTcpSocket* m_tcpSocket;   // 新增：TCP通信套接字
    QString m_topic;           // 新增：存储MQTT主题
};

#endif //QTCLIENT_LINKPUSH_H