#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>
#include <qdir.h>
#include <qlistwidget.h>
#include <qpushbutton.h>

#include "TrackItemDelegate.h"
#include "displayplaylists.h"
#include "mediacontroller.h"
#include "addcontentform.h"
#include "ClickableWIdget.h"
#include "librarymanager.h"
#include "playlistlistdelegate.h"

namespace Ui {
class HomePage;
}

class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr, MediaController *externalMediaController = nullptr);
    ~HomePage();

    void refreshUI();

    void setMediaController(MediaController *mediaController){
        this->mediaController = mediaController;
        // mediaController->initializePlaylist("C:\\Users\\aahad\\Music");
    }

    void handleFromCurrentTrackList(int row);
    void handleFromListOfTracks(int row);
    void handleFromFavourites(int row);
    void handlePause(const QString& trackPath = nullptr);
    void handleResume(const QString& trackPath = nullptr);

private slots:
    void on_currentTracklist_itemClicked(QListWidgetItem *item);
    // void on_listOfPlaylists_itemClicked(QListWidgetItem *item);
    void on_listOfTracks_itemClicked(QListWidgetItem *item);
    void changePage(int index);

    void overflowActionForTrack(const QModelIndex& index, const OverflowCommand action, const QString& trackName);
    void overflowActionForPlaylist(const QModelIndex& index, const OverflowAction action, const QString& playlistName);


protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::HomePage *ui;
    void displayQueueTab(const QString &playlistName);
    void displayPlaylistTab();
    void displayFavouritesTab();
    void displayListOfTracks(QDir musicDir, QString playlistName);

    bool addToPlaylist(const QStringList &trackPathsList);
    void onPlaylistSelected(const QString& playlistName);

    void playlistItemClicked(const QModelIndex& idx);

    bool togglePlay;

    MediaController *mediaController;

    ClickableWidget *overlay_;

    QPushButton *addContentBtn;
    AddContentForm *addContentForm;

    DisplayPlaylists *playListPanel;

    void syncPlayingHighlight(const QString& playingPath);
    void syncPlayIcons(bool isPlaying);
    void onFavouritesIconClicked(const QModelIndex& index);
};

#endif // HOMEPAGE_H
