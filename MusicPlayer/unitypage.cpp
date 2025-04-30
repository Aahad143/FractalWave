#include "unitypage.h"
#include "ui_unitypage.h"
#include "unitypage.h"
#include "worker.h"
#include <QResizeEvent>
#include <QThread>
#include <QGraphicsOpacityEffect>
#include <qpropertyanimation.h>
#include <qstackedlayout.h>
#include <qtimer.h>


UnityPage::UnityPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::UnityPage)
    , unityEmbedder(nullptr)
{
    ui->setupUi(this);
    // Create the UnityEmbedder instance

    // ui->activateVisualizer->setVisible(false);

    ui->loadingFrame->setFrameShape(QFrame::StyledPanel);

    embeddedFrame = new QFrame( this );    // 1) same parent
    embeddedFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    embeddedFrame->setFrameShape(QFrame::StyledPanel);
    // embeddedFrame->setVisible(false);

    // Ensure the loading frame is on top
    ui->loadingFrame->raise();

    unityEmbedder = new UnityEmbedder(embeddedFrame, "Visualizer", "Visualizer.exe");

    connect(ui->activateVisualizer, &QPushButton::pressed, this, &UnityPage::embedUnity);
}

// In UnityPage.cpp, outside of any function
bool UnityPage::m_unityEmbedded = false;

void setUpframe() {

}

void UnityPage::embedUnity(){
    qDebug() << "m_unityEmbedded:" << m_unityEmbedded;
    if (!m_unityEmbedded)
    {
        m_unityEmbedded = true;
        // Create a worker and a thread for the blocking operations
        QThread *workerThread = new QThread;

        Worker *worker = new Worker;
        worker->moveToThread(workerThread);

        if (unityEmbedder) {
            worker->scheduleFunctions(
                // wrap calls:
                [ue = unityEmbedder]() { return ue->launchUnity(); },
                [ue = unityEmbedder]() { return ue->findUnityWindow(); },
                [ue = unityEmbedder]() { return ue->waitForUnityReadySignal(); },
                [ue = unityEmbedder]() { return ue->embedUnity(); }
                );

            workerThread->start();
        }
        \
            // When finished, signal back to the main thread
            connect(worker, &Worker::finished, this, [this]() {
                // Perform any post-embedding UI updates
                unityEmbedder->debugWindowHierarchy();

                QFrame *lf = ui->loadingFrame;
                QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(lf);
                lf->setGraphicsEffect(effect);

                QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity", lf);
                anim->setDuration(500);
                anim->setStartValue(1.0);
                anim->setEndValue(0.0);

                // When the animation finishes, hide & clean up *lf*
                connect(anim, &QPropertyAnimation::finished, lf, [lf]() {
                    lf->setVisible(false);
                    lf->setGraphicsEffect(nullptr);
                });

                // and start it
                anim->start(QAbstractAnimation::DeleteWhenStopped);

                qDebug() << "Unity embedded successfully.";
            });

        // If there is an error, log it
        connect(worker, &Worker::error, this, [this](const QString &err){
            qDebug() << "Error in Worker for UnityEmbedder:" << err;
            ui->label->setText("Error loading visualizer");
            m_unityEmbedded = false;
        });


        // // Clean up the worker and thread after processing
        connect(worker, &Worker::finished, workerThread, &QThread::quit);
        connect(worker, &Worker::finished, worker, &Worker::deleteLater);
        connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);

        workerThread->start();
    }
}

UnityPage::~UnityPage(){
    delete unityEmbedder;
    m_unityEmbedded = false;
}

void UnityPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // make the unity embedded frame have the same position and size of the loadng frame for smooth transition
    embeddedFrame->setGeometry( ui->loadingFrame->geometry() );
    if (unityEmbedder) {
        // Resize the Unity window to match this widget's new size.
        unityEmbedder->resizeToHost();
    }

    if (ui->loadingFrame) {
        QSize newSize = event->size();
        ui->loadingFrame->setGeometry(0, 0, newSize.width(), newSize.height());
    }
}

void UnityPage::resizeWindow()
{
    unityEmbedder->resizeToHost();
}
