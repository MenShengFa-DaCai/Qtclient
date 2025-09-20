#include "infrared.h"
#include "ui_Infrared.h"


Infrared::Infrared(QWidget* parent) :
    QDialog(parent), ui(new Ui::Infrared) {
    ui->setupUi(this);
}

Infrared::~Infrared() {
    delete ui;
}