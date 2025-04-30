#ifndef UNITYPAGE_H
#define UNITYPAGE_H

#include <QWidget>
#include <qlabel.h>
#include "UnityEmbedder.h"

namespace Ui {
class UnityPage;
}

class UnityPage : public QWidget
{
    Q_OBJECT

public:
    explicit UnityPage(QWidget *parent = nullptr);
    ~UnityPage();

    void embedUnity();

    static bool isUnityEmbedded() {
        return m_unityEmbedded;
    }

    void resizeWindow();

public slots:

protected:
    // Override resizeEvent to update Unity's size when the widget resizes
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::UnityPage *ui;
    static bool m_unityEmbedded;

    UnityEmbedder *unityEmbedder;
    QLabel  *notification;

    QFrame* embeddedFrame = nullptr;
};

#endif // UNITYPAGE_H
