#ifndef QTCLIENT_MAINWIDGET_H
#define QTCLIENT_MAINWIDGET_H

#include <QMainWindow>       // 包含QMainWindow类，用于创建主窗口
#include <QMqttClient>       // 包含QMqttClient类，用于MQTT协议通信
#include <QJsonObject>       // 包含QJsonObject类，用于JSON数据处理

QT_BEGIN_NAMESPACE
namespace Ui { class MainWidget; }  // 声明UI命名空间中的MainWidget类（由.ui文件生成）
QT_END_NAMESPACE

// 主窗口类，继承自QMainWindow，负责智能家居控制中心的界面和逻辑
class MainWidget : public QMainWindow {
    Q_OBJECT  // Qt元对象系统宏，支持信号与槽机制

public:
    // 构造函数：parent为父窗口指针，默认为nullptr
    explicit MainWidget(QWidget* parent = nullptr,QString ip="",QString topic="");
    // 析构函数：重写父类析构函数
    ~MainWidget() override;

private slots:
    // MQTT消息接收槽函数：处理收到的MQTT消息和对应的主题
    void onMqttMessageReceived(const QByteArray &message, const QMqttTopicName &topic);
    // MQTT连接状态变化槽函数：处理MQTT客户端连接状态变化
    void onMqttStateChanged(QMqttClient::ClientState state) const;

    // 设备控制槽函数：对应各个设备的点击事件
    void onLedClicked();         // LED灯控制
    void onBuzzerClicked();      // 蜂鸣器控制
    void onFanClicked();         // 风扇控制
    void onDoorLockClicked();    // 门锁控制
    void onTvClicked();          // 电视控制
    void onThermoHygroClicked(); // 温湿度传感器控制（阈值设置）
    void onInfraredClicked();    // 红外传感器控制（详情查看）
    void onAirConditionerClicked(); // 空调开关控制
    void onRefreshClicked();     // 刷新按钮点击
    void onModeClicked();        // 模式切换按钮点击

    // 空调温度调节槽函数：处理空调温度滑块变化
    void onAirConditionerTempChanged(int value);
    //热水器温度调节
    void onWaterHeaterChanged(int value);

private:
    Ui::MainWidget* ui;          // UI界面指针，用于访问界面控件
    QMqttClient* mqttClient=nullptr;     // MQTT客户端指针，用于MQTT通信
    QString ip;             // MQTT代理服务器IP地址
    QString topic;          // MQTT主题前缀

    // 设备状态变量：记录各设备的开关状态
    bool ledState;               // LED灯状态（true为开，false为关）
    bool buzzerState;            // 蜂鸣器状态
    bool fanState;               // 风扇状态
    bool doorLockState;          // 门锁状态
    bool tvState;                // 电视状态
    bool infraredState;          // 红外传感器状态（检测到/未检测到人体）
    bool airConditionerState;    // 空调开关状态

    // 传感器数据：记录各传感器的实时数据
    float temperature;           // 温度值（℃）
    float humidity;              // 湿度值（%）
    float waterHeaterTemp;       // 热水器温度（℃）
    int airConditionerTemp;      // 空调设定温度（℃）

    // 阈值变量：记录各传感器的上下限阈值（用于自动控制逻辑）
    float tempUpperThreshold;    // 温度上限阈值
    float tempLowerThreshold;    // 温度下限阈值
    float humiUpperThreshold;    // 湿度上限阈值
    float humiLowerThreshold;    // 湿度下限阈值
    int waterHeaterLowerThreshold; // 热水器温度下限阈值
    int mod=2;                  //记录模式

    // 私有成员函数
    void initMqttClient();       // 初始化MQTT客户端（设置连接参数、连接信号槽）
    void updateDeviceUI();       // 更新UI界面（根据设备状态和传感器数据刷新控件显示）
    // 发布设备状态到MQTT服务器：device为设备名称，state为开关状态
    void publishDeviceState(const QString& device, bool state);
    // 发布传感器阈值到MQTT服务器：sensor为传感器名称，lower为下限，upper为上限
    void publishSensorThreshold(const QString& sensor, float lower, float upper);
};

#endif //QTCLIENT_MAINWIDGET_H