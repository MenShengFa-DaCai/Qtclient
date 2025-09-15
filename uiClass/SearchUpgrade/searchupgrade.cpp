#include "searchupgrade.h"
#include "ui_SearchUpgrade.h"


SearchUpgrade::SearchUpgrade(QWidget* parent) :
    QWidget(parent), ui(new Ui::SearchUpgrade) {
    ui->setupUi(this);
}

SearchUpgrade::~SearchUpgrade() {
    delete ui;
}