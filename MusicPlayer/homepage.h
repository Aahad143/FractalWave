#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>
#include <qlistwidget.h>
#include <qpushbutton.h>

#include "mediacontroller.h"
#include "addcontentform.h"
#include "librarymanager.h"

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

    void handleFromCurrentTrackList(int row);
    void handleFromListOfTracks(int row);
    void handlePause(const QString& trackPath = nullptr);
    void handleResume(const QString& trackPath = nullptr);

private slots:
    void on_currentTracklist_itemClicked(QListWidgetItem *item);
    void on_listOfPlaylists_itemClicked(QListWidgetItem *item);
    void on_listOfTracks_itemClicked(QListWidgetItem *item);
    void changePage(int index);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::HomePage *ui;
    void displayQueueTab(const QString &playlistName);
    void displayPlaylistTab();
    void displayFavouritesTab();

    bool togglePlay;

    MediaController *mediaController;

    QPushButton *addContentBtn;
    QWidget* overlay_;
    AddContentForm *addContentForm;
};

#endif // HOMEPAGE_H
