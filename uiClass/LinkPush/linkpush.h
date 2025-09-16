//
// Created by 1 on 15 9月 2025.
//

#ifndef QTCLIENT_LINKPUSH_H
#define QTCLIENT_LINKPUSH_H

#include <QWidget>


QT_BEGIN_NAMESPACE

namespace Ui {
    class LinkPush;
}

QT_END_NAMESPACE

class LinkPush : public QWidget {
    Q_OBJECT

public:
    explicit LinkPush(QWidget* parent = nullptr);
    ~LinkPush() override;

private:
    Ui::LinkPush* ui;
};


#endif //QTCLIENT_LINKPUSH_H