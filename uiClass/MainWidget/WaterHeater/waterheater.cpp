#include "waterheater.h"
#include "ui_WaterHeater.h"


WaterHeater::WaterHeater(QWidget* parent) :
    QDialog(parent), ui(new Ui::WaterHeater) {
    ui->setupUi(this);
}

WaterHeater::~WaterHeater() {
    delete ui;
}