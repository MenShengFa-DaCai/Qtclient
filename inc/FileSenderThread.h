#ifndef QTCLIENT_FILESENDERTHREAD_H
#define QTCLIENT_FILESENDERTHREAD_H

#include <QThread>
#include <QTcpSocket>
#include <QByteArray>
#include <utility>
#include <QMutex>  // 新增：用于线程安全

class FileSenderThread : public QThread {
    Q_OBJECT

public:
    explicit FileSenderThread(QString ip, QByteArray fileContent, QObject* parent = nullptr)
        : QThread(parent), m_ip(std::move(ip)), m_fileContent(std::move(fileContent)),
          m_bytesSent(0) {
    } // 初始化已发送字节数为0

    // 新增：获取已发送的字节数（线程安全）
    qint64 getBytesSent() {
        QMutexLocker locker(&m_mutex); // 加锁防止读写冲突
        return m_bytesSent;
    }

protected:
    void run() override;

private:
    QString m_ip;
    QByteArray m_fileContent;
    qint64 m_bytesSent; // 新增：记录已发送字节数
    QMutex m_mutex; // 新增：保护m_bytesSent的互斥锁

signals:
    void sendSuccess();
    void sendError(const QString& errorMsg);
    // 新增：实时发送进度信号（可选，用于UI显示）
    void sendProgress(qint64 sent, qint64 total);
};

#endif
