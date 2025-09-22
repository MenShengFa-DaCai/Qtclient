#ifndef QTCLIENT_LINKPUSH_H
#define QTCLIENT_LINKPUSH_H

#include <QWidget>
#include <QTcpSocket>
#include <QMqttClient>

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

    // 连接到设备并建立TCP连接
    void connectToDevice(const QString& ip, const QString& topic);

private slots:
    void onTcpReadyRead();
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpErrorOccurred(QAbstractSocket::SocketError error);
    // 处理文件发送结果的槽函数
    void onFileSendSuccess();
    void onFileSendError(const QString& errorMsg);
    // 新增：处理文件发送进度更新
    void onFileSendProgress(qint64 sent, qint64 total);

private:
    Ui::LinkPush* ui;
    QString m_ipAddress;
    QTcpSocket* m_tcpSocket;
    QString m_topic;
    QByteArray m_jsonData=""; //版本文件内容
    QWidget* parent=nullptr;
};

#endif