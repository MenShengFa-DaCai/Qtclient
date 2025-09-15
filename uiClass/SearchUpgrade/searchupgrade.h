//
// Created by 1 on 15 9月 2025.
//

#ifndef QTCLIENT_SEARCHUPGRADE_H
#define QTCLIENT_SEARCHUPGRADE_H

#include <QWidget>


QT_BEGIN_NAMESPACE

namespace Ui {
    class SearchUpgrade;
}

QT_END_NAMESPACE

class SearchUpgrade : public QWidget {
    Q_OBJECT

public:
    explicit SearchUpgrade(QWidget* parent = nullptr);
    ~SearchUpgrade() override;

private:
    Ui::SearchUpgrade* ui;
};


#endif //QTCLIENT_SEARCHUPGRADE_H