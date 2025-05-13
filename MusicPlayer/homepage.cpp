#include <QDir>
#include <QRandomGenerator>
#include <qevent.h>

#include "homepage.h"
#include "ui_homepage.h"
#include "helper/qsshelper.h"
#include "helper/delegatehelper.h"
#include "TrackItemDelegate.h"


HomePage::HomePage(QWidget *parent, MediaController *externalMediaController)
    : QWidget(parent)
    , ui(new Ui::HomePage)
    , mediaController(externalMediaController)
    , addContentBtn(nullptr)
{
    ui->setupUi(this);

    ui->listOfTracks->setVisible(false);

    ui->homepageTabs->setViewMode(QListView::IconMode);      // treat items like icons, not a strict list
    ui->homepageTabs->setFlow(QListView::LeftToRight);      // lay them out left→right, then wrap
    ui->homepageTabs->setFixedHeight(100);
    // 3) Make sure items really use the grid:
    ui->homepageTabs->setUniformItemSizes(true);
    // ui->homepageTabs->setWrapping(true);                    // allow wrapping to next row
    ui->homepageTabs->setResizeMode(QListView::Adjust);     // adjust layout when items change
    ui->homepageTabs->setSpacing(0);

    auto* queueDelegate = new TrackItemDelegate(this);
    auto* listOfTracksDelegate = new TrackItemDelegate(this);

    ui->currentTracklist->setItemDelegate(queueDelegate);
    ui->currentTracklist->setSelectionMode(QAbstractItemView::NoSelection);

    ui->listOfTracks->setItemDelegate(listOfTracksDelegate);
    ui->listOfTracks->setSelectionMode(QAbstractItemView::NoSelection);

    connect(queueDelegate, &TrackItemDelegate::playIconClicked, this, &HomePage::handleFromCurrentTrackList);
    connect(listOfTracksDelegate, &TrackItemDelegate::playIconClicked, this, &HomePage::handleFromListOfTracks);

    connect(mediaController, &MediaController::paused, this, &HomePage::handlePause);
    connect(mediaController, &MediaController::playing, this, &HomePage::handleResume);

    // Define the path to your music folder
    if (mediaController) {
        mediaController->setTracklist(ui->currentTracklist);
        const QString& musicFolderPath = LibraryManager::instance().getCorrentMusicDirectory();
        const QString& lastPlaylistInQueue = LibraryManager::instance().getLastPlaylistPlayed();
        displayQueueTab(lastPlaylistInQueue);
        displayPlaylistTab();
        mediaController->initializePlaylist(lastPlaylistInQueue);
    }
    else {
        qDebug() << "MediaController is nullptr";
    }

    // Connect item selection to page change
    connect(ui->homepageTabs, &QListWidget::currentRowChanged, this, &HomePage::changePage);

    // code for initialilzing an "Add Playlists" button
    addContentBtn = new QPushButton("Add Playlist", this);
    addContentBtn->setObjectName("addContent");

    addContentBtn->setFixedSize(80, 80);
    addContentBtn->raise();

    // code for initializing the frame for creating playlist/adding track
     addContentForm = new AddContentForm(this);
     addContentForm->setObjectName("addContentForm");
     addContentForm->hide();
     addContentForm->move(
        (width()  -  addContentForm->width())  / 2,
        (height() -  addContentForm->height()) / 2
        );

     addContentForm->setFixedSize(400, 200);



    // Initializing overlay (for when addContent frame becomes visible
    overlay_ = new QWidget(this);
    overlay_->setObjectName("overlay");
    overlay_->hide();
    overlay_->setGeometry(this->rect());

    // connecting the form and overlay visibility to add a focus effect
    connect(addContentBtn, &QPushButton::clicked, this, [=](){
        overlay_->show();
        overlay_->raise();
        addContentForm->show();
        addContentForm->raise();
    });

    connect(addContentForm, &AddContentForm::closed, overlay_, &QWidget::hide);

    this->setStyleSheet(applyStyles(":/resources/styles/homepage.qss"));        // Apply the stylesheet to the homepage
}

void HomePage::handlePause(const QString& trackPath)
{
    if (trackPath == nullptr) {
        return;
    }

    toggleDelegatePlayIcon(mediaController, ui->currentTracklist, false);
    toggleDelegatePlayIcon(mediaController, ui->listOfTracks, false);
}

void HomePage::handleResume(const QString& trackPath)
{
    if (trackPath == nullptr) {
        return;
    }

    toggleDelegatePlayIcon(mediaController, ui->currentTracklist, true);
    toggleDelegatePlayIcon(mediaController, ui->listOfTracks, true);
}

HomePage::~HomePage()
{
    delete ui;
}

void HomePage::displayQueueTab(const QString& playlistName){
    QDir musicDir(LibraryManager::instance().libraryManager().getCorrentMusicDirectory());
    qDebug() << "musicDir in displayQueueTab:" << musicDir;

    // Get the list of file names
    qDebug() << "playlistName in displayQueueTab()" << playlistName;
    QStringList trackNames = LibraryManager::instance().getTracksFromPlaylist(playlistName);
    qDebug() << "trackNames in displayQueueTab():" << trackNames;

    // For each track, create an item with the track name and attach the full file path
    for (const QString &trackName : trackNames) {
        QListWidgetItem *item = new QListWidgetItem(trackName);
        // Save the absolute file path for later retrieval
        item->setData(Qt::UserRole, musicDir.absoluteFilePath(trackName));
        item->setData(TitleRole, trackName);
        item->setData(ArtistRole, "Juno Miles");
        item->setData(IsPlayingRole, false);
        ui->currentTracklist->addItem(item);
    }
}

void HomePage::displayPlaylistTab(){
    QStringList playlists = LibraryManager::instance().getPlaylistNames();

    for (const QString& playlistName : playlists) {
        QListWidgetItem *item = new QListWidgetItem(playlistName);
        qDebug() << "playlist in displayPlaylistTab():" << playlistName;
        ui->listOfPlaylists->addItem(item);
    }
}

void HomePage::on_currentTracklist_itemClicked(QListWidgetItem *item)
{
    // // When a track is selected in HomePage:
    // int selectedIndex = ui->currentTracklist->currentRow();
    // if (!mediaController->loadAndPlayTrack(selectedIndex)) {
    //     qDebug() << "Failed to load selected track.";
    // }
}

void HomePage::handleFromCurrentTrackList(int row)
{
    if (!mediaController->loadAndPlayTrack(row)) {
        qDebug() << "Failed to load/play track at" << row;
    }

    toggleDelegatePlayIcon(mediaController, ui->currentTracklist, mediaController->isPlaying());
}

void HomePage::handleFromListOfTracks(int row)
{
    ui->currentTracklist->clear();
    QString playlistName = ui->listOfTracks->item(row)->data(Qt::UserRole).toString();
    displayQueueTab(playlistName);
    mediaController->initializePlaylist(playlistName);

    int selectedIndex = ui->listOfTracks->currentRow();
    if (!mediaController->loadAndPlayTrack(selectedIndex)) {
        qDebug() << "Failed to load selected track.";
    }

    toggleDelegatePlayIcon(mediaController, ui->listOfTracks, mediaController->isPlaying());
}

void HomePage:: on_listOfPlaylists_itemClicked(QListWidgetItem *item) {
    QDir musicDir(LibraryManager::instance().libraryManager().getCorrentMusicDirectory());
    qDebug() << "musicDir in displayQueueTab:" << musicDir;

    ui->listOfTracks->setVisible(true);
    ui->listOfTracks->clear();
    QStringList tracks = LibraryManager::instance().getTracksFromPlaylist(item->text());

    for (const QString& trackName : tracks) {
        QListWidgetItem *trackItem = new QListWidgetItem(trackName);
        qDebug() << "trackName in on_listOfPlaylists_itemClicked():" << trackName;
        trackItem->setData(Qt::UserRole, item->text());
        // trackItem->setData(Qt::UserRole, musicDir.absoluteFilePath(trackName));
        trackItem->setData(TitleRole, trackName);
        trackItem->setData(ArtistRole, "Juno Miles");
        trackItem->setData(IsPlayingRole, false);
        ui->listOfTracks->addItem(trackItem);
    }
}

void HomePage::on_listOfTracks_itemClicked(QListWidgetItem *item)
{
    // ui->currentTracklist->clear();
    // QString playlistName = item->data(Qt::UserRole).toString();
    // displayQueueTab(playlistName);
    // mediaController->initializePlaylist(playlistName);

    // int selectedIndex = ui->listOfTracks->currentRow();
    // if (!mediaController->loadAndPlayTrack(selectedIndex)) {
    //     qDebug() << "Failed to load selected track.";
    // }
}

void HomePage::changePage(int index)
{
    ui->stackedWidget->setCurrentIndex(index);
}

void HomePage::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (addContentBtn) {
        QSize newSize = event->size();

        // Set the button's geometry while resizing
        addContentBtn->setGeometry(this->width() - addContentBtn->width() - 20,
                                   newSize.height() - addContentBtn->height() - 11,
                                   addContentBtn->width(),
                                   addContentBtn->height());
    }

    if ( addContentForm) {
         addContentForm->move(
            (width()  -  addContentForm->width())  / 2,
            (height() -  addContentForm->height()) / 2
            );
    }

    if (overlay_) {
        overlay_->setGeometry(this->rect());
    }

    if (ui->homepageTabs) {
        int count = ui->homepageTabs->count();
        if (count == 0) return;


        // How much width each tab gets
        int totalW  = ui->homepageTabs->viewport()->width();
        int tabW    = totalW / count - 3;
        int tabH    = ui->homepageTabs->viewport()->height();

        // Set each item’s hint so the view lays them out equally
        for (int i = 0; i < count; ++i) {
            auto* it = ui->homepageTabs->item(i);
            it->setSizeHint(QSize(tabW, tabH));
        }

        // Force a re‐layout
        ui->homepageTabs->update();
    }
}

