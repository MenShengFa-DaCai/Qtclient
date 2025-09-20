//
// Created by 1 on 20 9月 2025.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ThermoHygroHistory.h" resolved

#include "thermohygrohistory.h"
#include "ui_ThermoHygroHistory.h"


ThermoHygroHistory::ThermoHygroHistory(QWidget* parent) :
    QDialog(parent), ui(new Ui::ThermoHygroHistory) {
    ui->setupUi(this);
}

ThermoHygroHistory::~ThermoHygroHistory() {
    delete ui;
}