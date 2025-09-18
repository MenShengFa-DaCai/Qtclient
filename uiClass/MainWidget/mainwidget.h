
#ifndef QTCLIENT_MAINWIDGET_H
#define QTCLIENT_MAINWIDGET_H

#include <QMainWindow>
/************************************************************
 *
 *该界面主要实现显示如下数据：
 *
 *  一个LED灯的状态（开关需要控制，状态布尔）
 *
 *  一个蜂鸣器的状态（开关需要控制，状态布尔）
 *
 *  一个风扇的状态（开关需要控制，状态布尔）
 *
 *  一个门锁的状态（开关需要控制，状态布尔）
 *
 *  一个电视的状态（开关需要控制，状态布尔）
 *
 *  环境湿度数据 （浮点，00.00 - 99.99）
 *
 *  湿度上下限阈值（阈值需要控制）
 *
 *  环境温度（浮点，-50.00 - 50.00）
 *
 *  温度上下限阈值（阈值需要控制）
 *
 *  人体红外（布尔）
 *
 *  热水器水温数据（浮点，00.00 - 99.99）
 *
 *  热水器温度下限（阈值需要控制）
 *
 *  空调：
 *      空调的状态（开关需要控制，状态布尔）
 *      空调的温度（整形，需要控制，16-35）
 ************************************************************/

QT_BEGIN_NAMESPACE

namespace Ui {
    class MainWidget;
}

QT_END_NAMESPACE

class MainWidget : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWidget(QWidget* parent = nullptr);
    ~MainWidget() override;

private:
    Ui::MainWidget* ui;
};


#endif //QTCLIENT_MAINWIDGET_H