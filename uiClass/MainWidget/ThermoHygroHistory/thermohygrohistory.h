#ifndef QTCLIENT_THERMOHYGROHISTORY_H
#define QTCLIENT_THERMOHYGROHISTORY_H

#include <QDialog>
#include <QMqttClient>
#include <QJsonArray>
#include "qcustomplot.h"

QT_BEGIN_NAMESPACE
namespace Ui {
    class ThermoHygroHistory;
}
QT_END_NAMESPACE

class ThermoHygroHistory : public QDialog {
    Q_OBJECT

public:
    explicit ThermoHygroHistory(QMqttClient* mqttClient, QWidget* parent = nullptr);
    ~ThermoHygroHistory() override;

private slots:
    void onQueryButtonClicked();
    void onMqttMessageReceived(const QByteArray& message, const QMqttTopicName& topic);

private:
    Ui::ThermoHygroHistory* ui;
    QMqttClient* mqttClient;
    QVector<double> temperatureTime;
    QVector<double> temperatureValues;
    QVector<double> humidityTime;
    QVector<double> humidityValues;
    qint64 m_startTime;  // 保存查询开始时间
    qint64 m_endTime;    // 保存查询结束时间
    int receivedCount; // 计数器，用于跟踪收到的响应数量

    void setupChart();
    void processHistoryData(int key, const QJsonArray& data);
};

#endif // QTCLIENT_THERMOHYGROHISTORY_H