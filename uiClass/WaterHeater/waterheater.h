//
// Created by 1 on 20 9月 2025.
//

#ifndef QTCLIENT_WATERHEATER_H
#define QTCLIENT_WATERHEATER_H

#include <QDialog>


QT_BEGIN_NAMESPACE

namespace Ui {
    class WaterHeater;
}

QT_END_NAMESPACE

class WaterHeater : public QDialog {
    Q_OBJECT

public:
    explicit WaterHeater(QWidget* parent = nullptr);
    ~WaterHeater() override;

private:
    Ui::WaterHeater* ui;
};


#endif //QTCLIENT_WATERHEATER_H