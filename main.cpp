#include <QApplication>
#include <QPushButton>
#include "uiClass/SearchUpgrade/searchupgrade.h"

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    SearchUpgrade searchUpgrade;
    searchUpgrade.show();
    return QApplication::exec();
}