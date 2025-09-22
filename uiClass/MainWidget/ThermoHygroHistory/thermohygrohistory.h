//
// Created by 1 on 20 9月 2025.
//

#ifndef QTCLIENT_THERMOHYGROHISTORY_H
#define QTCLIENT_THERMOHYGROHISTORY_H

#include <QDialog>


QT_BEGIN_NAMESPACE

namespace Ui {
    class ThermoHygroHistory;
}

QT_END_NAMESPACE

class ThermoHygroHistory : public QDialog {
    Q_OBJECT

public:
    explicit ThermoHygroHistory(QWidget* parent = nullptr);
    ~ThermoHygroHistory() override;

private:
    Ui::ThermoHygroHistory* ui;
};


#endif //QTCLIENT_THERMOHYGROHISTORY_H