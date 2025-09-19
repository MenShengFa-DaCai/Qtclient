#include "FileSenderThread.h"
#include <QTcpSocket>

void FileSenderThread::run() {
    QTcpSocket socket;
    const qint64 blockSize = 1024; // 分块大小（4KB，可根据需求调整）
    qint64 totalSent = 0; // 累计已发送字节数
    const qint64 totalSize = m_fileContent.size(); // 总大小

    // 连接目标主机
    socket.connectToHost(m_ip, 8887);
    if (!socket.waitForConnected(30000)) {
        emit sendError(QString("连接失败: %1").arg(socket.errorString()));
        return;
    }

    // 分块发送数据
    while (totalSent < totalSize) {
        // 计算当前块的大小（最后一块可能小于blockSize）
        qint64 currentBlockSize = qMin(blockSize, totalSize - totalSent);

        // 发送当前块（从totalSent位置开始，发送currentBlockSize字节）
        qint64 bytesWritten = socket.write(
            m_fileContent.data() + totalSent, // 数据起始地址
            currentBlockSize // 发送字节数
        );

        if (bytesWritten <= 0) {
            emit sendError(QString("发送失败: %1").arg(socket.errorString()));
            socket.disconnectFromHost();
            return;
        }

        // 等待当前块发送完成
        if (!socket.waitForBytesWritten(30000)) {
            emit sendError(QString("发送超时: %1").arg(socket.errorString()));
            socket.disconnectFromHost();
            return;
        }

        // 更新已发送字节数（线程安全）
        totalSent += bytesWritten;
        {
            QMutexLocker locker(&m_mutex);
            m_bytesSent = totalSent;
        }

        // 发送进度信号（可选，供UI更新进度条等）
        emit sendProgress(totalSent, totalSize);
    }

    // 全部发送完成
    socket.disconnectFromHost();
    emit sendSuccess();
}
