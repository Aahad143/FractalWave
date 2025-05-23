#ifndef CLICKABLEWIDGET_H
#define CLICKABLEWIDGET_H

#include <QWidget>
#include <QMouseEvent>

class ClickableWidget : public QWidget {
    Q_OBJECT
public:
    explicit ClickableWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_NoSystemBackground, false);
        setAttribute(Qt::WA_StyledBackground, true);  // this allows QSS background to apply
    }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            emit clicked();
        }
        // still let base class handle any other logic
        QWidget::mousePressEvent(event);
    }
};

#endif // CLICKABLEWIDGET_H
