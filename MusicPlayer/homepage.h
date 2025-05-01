#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>
#include <qlistwidget.h>
#include "mediacontroller.h"

namespace Ui {
class HomePage;
}

class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr, MediaController *externalMediaController = nullptr);
    ~HomePage();

    void setMediaController(MediaController *mediaController){
        this->mediaController = mediaController;
        // mediaController->initializePlaylist("C:\\Users\\aahad\\Music");
    }

private slots:
    void on_currentTracklist_itemClicked(QListWidgetItem *item);
    void changePage(int index);

private:
    Ui::HomePage *ui;
    void displayTrackList(const QString &musicFolderPath);

    MediaController *mediaController;
};

#endif // HOMEPAGE_H
