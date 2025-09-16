#include "linkpush.h"
#include "ui_LinkPush.h"


LinkPush::LinkPush(QWidget* parent) :
    QWidget(parent), ui(new Ui::LinkPush) {
    ui->setupUi(this);
}

LinkPush::~LinkPush() {
    delete ui;
}