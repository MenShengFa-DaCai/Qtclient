#ifndef QTCLIENT_INFRARED_H
#define QTCLIENT_INFRARED_H

#include <QDialog>
#include <QUdpSocket>
#include <QImage>
#include <QPixmap>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
    class Infrared;
}
QT_END_NAMESPACE

class Infrared : public QDialog {
    Q_OBJECT

public:
    explicit Infrared(QWidget* parent = nullptr);
    ~Infrared() override;

private slots:
    void onUdpDataReceived();  // UDP数据接收槽函数
    void onClearRecords();     // 清空记录
    void onStartStream();      // 开始视频流
    void onStopStream();       // 停止视频流
    void updateFrameRate();    // 更新帧率显示

private:
    Ui::Infrared* ui;
    QUdpSocket* udpSocket;     // UDP套接字
    QTimer* frameRateTimer;    // 帧率计时器
    int frameCount;            // 帧计数器
    int udpPort;               // UDP端口号

    void initUdpSocket();      // 初始化UDP套接字
    void processImageData(const QByteArray& data);  // 处理图像数据
    void addDetectionRecord(const QString& timestamp, const QString& status);  // 添加检测记录
};

#endif //QTCLIENT_INFRARED_H