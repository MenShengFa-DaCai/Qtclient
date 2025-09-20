
#ifndef QTCLIENT_INFRARED_H
#define QTCLIENT_INFRARED_H

#include <QDialog>


QT_BEGIN_NAMESPACE

namespace Ui {
    class Infrared;
}

QT_END_NAMESPACE

class Infrared : public QDialog {
    Q_OBJECT

public:
    explicit Infrared(QWidget* parent = nullptr);
    ~Infrared() override;

private:
    Ui::Infrared* ui;
};


#endif //QTCLIENT_INFRARED_H