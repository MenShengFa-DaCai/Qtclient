#include "infrared.h"
#include "ui_Infrared.h"
#include <QMessageBox>
#include <QDateTime>
#include <QBuffer>
#include <QHeaderView>

Infrared::Infrared(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::Infrared),
    udpSocket(nullptr),
    frameRateTimer(new QTimer(this)),
    frameCount(0),
    udpPort(7777) {  // 默认UDP端口

    ui->setupUi(this);

    // 设置窗口标题
    setWindowTitle("红外检测监控");

    // 初始化表格
    QStringList headers;
    headers << "时间戳" << "检测状态" << "备注";
    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 连接信号槽
    connect(ui->btnClearAll, &QPushButton::clicked, this, &Infrared::onClearRecords);
    connect(ui->btnStartStream, &QPushButton::clicked, this, &Infrared::onStartStream);
    connect(ui->btnStopStream, &QPushButton::clicked, this, &Infrared::onStopStream);
    connect(frameRateTimer, &QTimer::timeout, this, &Infrared::updateFrameRate);

    // 初始化UDP
    initUdpSocket();

    // 启动帧率计时器（每秒更新一次）
    frameRateTimer->start(1000);

    // 添加一条初始记录
    addDetectionRecord(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),
                      "监控已启动");
}

Infrared::~Infrared() {
    if (udpSocket) {
        udpSocket->close();
        delete udpSocket;
    }
    delete ui;
}

void Infrared::initUdpSocket() {
    udpSocket = new QUdpSocket(this);

    // 绑定UDP端口
    if (!udpSocket->bind(udpPort, QUdpSocket::ShareAddress)) {
        QMessageBox::warning(this, "UDP错误",
                           QString("无法绑定端口 %1: %2").arg(udpPort).arg(udpSocket->errorString()));
        return;
    }

    connect(udpSocket, &QUdpSocket::readyRead, this, &Infrared::onUdpDataReceived);

    ui->statusLabel->setText(QString("UDP监听端口: %1 - 等待数据...").arg(udpPort));
}

void Infrared::onUdpDataReceived() {
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;

        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        // 处理接收到的数据
        processImageData(datagram);

        frameCount++;
    }
}

void Infrared::processImageData(const QByteArray& data) {
    // 尝试将数据解析为图像
    QImage image;
    if (image.loadFromData(data)) {
        // 图像数据有效，显示在标签上
        QPixmap pixmap = QPixmap::fromImage(image);

        // 缩放图像以适应显示区域，保持宽高比
        pixmap = pixmap.scaled(ui->videoLabel->width(),
                              ui->videoLabel->height(),
                              Qt::KeepAspectRatio,
                              Qt::SmoothTransformation);

        ui->videoLabel->setPixmap(pixmap);
        ui->videoLabel->setAlignment(Qt::AlignCenter);

        // 检测到运动（这里简单假设有数据就表示有运动）
        static int motionCount = 0;
        motionCount++;

        if (motionCount % 10 == 0) {  // 每10帧记录一次检测结果
            QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            QString status = "检测到运动";
            addDetectionRecord(timestamp, status);
        }
    } else {
        // 如果不是图像数据，尝试解析为文本信息
        QString textData = QString::fromUtf8(data);
        if (textData.contains("motion", Qt::CaseInsensitive) ||
            textData.contains("detect", Qt::CaseInsensitive)) {

            QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            QString status = "红外检测触发";
            addDetectionRecord(timestamp, status);
        }
    }
}

void Infrared::addDetectionRecord(const QString& timestamp, const QString& status) {
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);

    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(timestamp));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(status));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem("自动记录"));

    // 自动滚动到最后一行
    ui->tableWidget->scrollToBottom();
}

void Infrared::onClearRecords() {
    ui->tableWidget->setRowCount(0);
    addDetectionRecord(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),
                      "记录已清空");
}

void Infrared::onStartStream() {
    if (udpSocket && udpSocket->state() == QUdpSocket::BoundState) {
        ui->statusLabel->setText(QString("UDP端口 %1 - 正在接收数据...").arg(udpPort));
        ui->btnStartStream->setEnabled(false);
        ui->btnStopStream->setEnabled(true);
    }
}

void Infrared::onStopStream() {
    ui->videoLabel->clear();
    ui->statusLabel->setText(QString("UDP端口 %1 - 已停止接收").arg(udpPort));
    ui->btnStartStream->setEnabled(true);
    ui->btnStopStream->setEnabled(false);
}

void Infrared::updateFrameRate() {
    ui->frameRateLabel->setText(QString("帧率: %1 fps").arg(frameCount));
    frameCount = 0;  // 重置计数器
}