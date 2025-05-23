#include <QDir>
#include <QRandomGenerator>
#include <qevent.h>
#include <QInputDialog>
#include <QMessageBox>

#include "homepage.h"
#include "ui_homepage.h"
#include "helper/qsshelper.h"
#include "helper/delegatehelper.h"
#include "centeredicondelegate.h".h"


HomePage::HomePage(QWidget *parent, MediaController *externalMediaController)
    : QWidget(parent)
    , ui(new Ui::HomePage)
    , mediaController(externalMediaController)
    , addContentBtn(nullptr)
    , playListPanel(nullptr)
    , overlay_(nullptr)
{
    ui->setupUi(this);

    ui->listOfTracks->setVisible(false);

    ui->headingFrame->setFixedHeight(150);

    ui->homepageTabs->setViewMode(QListView::IconMode);      // treat items like icons, not a strict list
    ui->homepageTabs->setFlow(QListView::LeftToRight);      // lay them out left→right, then wrap
    ui->homepageTabs->setIconSize({48,48});
    ui->homepageTabs->setFixedHeight(60);
    ui->homepageTabs->setContentsMargins(10,10,100,10);
    // 3) Make sure items really use the grid:
    ui->homepageTabs->setUniformItemSizes(true);
    ui->homepageTabs->setResizeMode(QListView::Adjust);     // adjust layout when items change
    ui->homepageTabs->setSpacing(0);

    // auto* centeredIconDelegate = new CenteredIcon2Delegate(this);
    // ui->homepageTabs->setItemDelegate(centeredIconDelegate);

    auto* queueDelegate = new TrackItemDelegate(this, Default, mediaController);
    auto* listOfTracksDelegate = new TrackItemDelegate(this, Default, mediaController);
    auto* listOfFavouritesDelegate = new TrackItemDelegate(this, Favourites, mediaController);

    auto* listOfPlaylistsDelegate = new PlaylistListDelegate(this);

    ui->listOfPlaylists->setItemDelegate(listOfPlaylistsDelegate);
    ui->listOfPlaylists->setMouseTracking(true);
    ui->listOfPlaylists->viewport()->setMouseTracking(true);

    connect(listOfPlaylistsDelegate, &PlaylistListDelegate::playlistItemClicked, this, &HomePage::playlistItemClicked);
    connect(listOfPlaylistsDelegate, &PlaylistListDelegate::overflowRequested, this, &HomePage::overflowActionForPlaylist);

    ui->currentTracklist->setItemDelegate(queueDelegate);
    ui->currentTracklist->setSelectionMode(QAbstractItemView::NoSelection);
    ui->currentTracklist->setMouseTracking(true);
    ui->currentTracklist->viewport()->setMouseTracking(true);

    ui->listOfTracks->setItemDelegate(listOfTracksDelegate);
    ui->listOfTracks->setSelectionMode(QAbstractItemView::NoSelection);
    ui->listOfTracks   ->setMouseTracking(true);
    ui->listOfTracks   ->viewport()->setMouseTracking(true);

    ui->listOfFavourites->setItemDelegate(listOfFavouritesDelegate);
    ui->listOfFavourites->setSelectionMode(QAbstractItemView::NoSelection);
    ui->listOfFavourites->setMouseTracking(true);
    ui->listOfFavourites->viewport()->setMouseTracking(true);

    connect(queueDelegate, &TrackItemDelegate::playIconClicked, this, &HomePage::handleFromCurrentTrackList);
    connect(listOfTracksDelegate, &TrackItemDelegate::playIconClicked, this, &HomePage::handleFromListOfTracks);
    connect(listOfFavouritesDelegate, &TrackItemDelegate::playIconClicked, this, &HomePage::handleFromFavourites);

    connect(queueDelegate, &TrackItemDelegate::favouritesIconClicked, this, &HomePage::onFavouritesIconClicked);
    connect(listOfTracksDelegate, &TrackItemDelegate::favouritesIconClicked, this, &HomePage::onFavouritesIconClicked);
    connect(listOfFavouritesDelegate, &TrackItemDelegate::favouritesIconClicked, this, &HomePage::onFavouritesIconClicked);

    connect(queueDelegate, &TrackItemDelegate::overflowActionRequested, this, &HomePage::overflowActionForTrack);
    connect(listOfTracksDelegate, &TrackItemDelegate::overflowActionRequested, this, &HomePage::overflowActionForTrack);
    connect(listOfFavouritesDelegate, &TrackItemDelegate::overflowActionRequested, this, &HomePage::overflowActionForTrack);

    connect(mediaController, &MediaController::paused, this, &HomePage::handlePause);
    connect(mediaController, &MediaController::playing, this, &HomePage::handleResume);

    // Define the path to your music folder
    if (mediaController) {
        mediaController->setTracklist(ui->currentTracklist);
        const QString& musicFolderPath = LibraryManager::instance().getCurrentMusicDirectory();
        const QString& lastPlaylistInQueue = LibraryManager::instance().getLastPlaylistPlayed();
        displayQueueTab(lastPlaylistInQueue);
        displayPlaylistTab();
        displayFavouritesTab();
        mediaController->initializePlaylist(lastPlaylistInQueue);
    }
    else {
        qDebug() << "MediaController is nullptr";
    }

    // Connect item selection to page change
    connect(ui->homepageTabs, &QListWidget::currentRowChanged, this, &HomePage::changePage);

    // code for initialilzing an "Add Playlists" button
    addContentBtn = new QPushButton("", this);
    addContentBtn->setObjectName("addContent");


    // Set the “plus” icon (adjust the path & size to taste)
    addContentBtn->setIcon(QIcon(":/resources/icons/plus-icon.png"));
    addContentBtn->setIconSize(QSize(32,32));

    addContentBtn->setFlat(true);
    addContentBtn->setFixedSize(60, 60);
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

    playListPanel = new DisplayPlaylists(this);
    playListPanel->setObjectName("playListPanel");
    playListPanel->hide();
    playListPanel->move(
        (width()  -  playListPanel->width())  / 2,
        (height() -  playListPanel->height()) / 2
    );

    // Initializing overlay (for when addContent frame becomes visible
    overlay_ = new ClickableWidget(this);
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
    connect(playListPanel, &DisplayPlaylists::closed, overlay_, &QWidget::hide);

    connect(overlay_, &ClickableWidget::clicked, this, [=](){
        if (addContentForm) {
            addContentForm->resetForm();
            addContentForm->hide();
        }

        if (playListPanel) {
            playListPanel->hide();
        }

        overlay_-> hide();
    });

    this->setStyleSheet(applyStyles(":/resources/styles/homepage.qss"));        // Apply the stylesheet to the homepage
}

// In HomePage.cpp

void HomePage::refreshUI()
{
    // 1) Rescan disk & rebuild master playlists
    LibraryManager& lm = LibraryManager::instance().libraryManager();
    lm.scanDirectory();

    // 2) Hide any open panels/forms
    if (addContentForm)    addContentForm->hide();
    if (playListPanel)     playListPanel->hide();
    if (overlay_)          overlay_->hide();

    // 3) Clear all lists
    ui->currentTracklist->clear();
    ui->listOfPlaylists->clear();
    ui->listOfTracks->clear();

    // 4) Repopulate the “Queue” (current tracks) tab
    const QString last = lm.getLastPlaylistPlayed();
    displayQueueTab(last);
    if (mediaController) {
        mediaController->initializePlaylist(last);
    }

    // 5) Repopulate the “Playlists” tab
    displayPlaylistTab();

    QDir musicDir(LibraryManager::instance().libraryManager().getCurrentMusicDirectory());
    displayListOfTracks(musicDir, "All Songs");

    // 6) Repopulate “Favorites” if you have that page
    displayFavouritesTab();
}

void HomePage::overflowActionForTrack(const QModelIndex& index, const OverflowCommand action, const QString& trackName)
{
    switch (action) {
    case OverflowCommand::AddToPlaylist:
        qDebug() << "Adding to playlist";
        if (!addToPlaylist( {trackName} )) {
            qDebug() << "Failed to add track at:" << trackName << "to playlist";
        }
        break;
    case OverflowCommand::Delete: {
        // 1) Ask user to confirm
        auto reply = QMessageBox::question(
            this,
            tr("Delete Track"),
            tr("Are you sure you want to delete \"%1\"?").arg(trackName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );

        // 2) If they clicked Yes, perform deletion
        if (reply == QMessageBox::Yes) {
            if (!LibraryManager::instance()
                     .libraryManager()
                     .delTrackFromDir(trackName))
            {
                QMessageBox::warning(
                    this,
                    tr("Delete Failed"),
                    tr("Could not delete the track \"%1\".").arg(trackName)
                    );
            } else {
                // 3) Refresh UI so the track list updates
                refreshUI();
            }
        }
        // else: user canceled, do nothing
        break;
    }

    default:
        break;
    }
}

void HomePage::overflowActionForPlaylist(const QModelIndex& index, const OverflowAction action, const QString& playlistName)
{
    switch (action) {
    case PlaylistRename: {
        bool ok = false;
        // Ask the user for a new name
        QString newName = QInputDialog::getText(
            this,
            tr("Rename Playlist"),
            tr("New name for “%1”:").arg(playlistName),
            QLineEdit::Normal,
            playlistName,
            &ok
            );

        // If they clicked OK and typed something different:
        if (ok && !newName.isEmpty() && newName != playlistName) {
            if (!LibraryManager::instance().renamePlaylist(playlistName, newName)) {
                QMessageBox::warning(this,
                                     tr("Rename Failed"),
                                     tr("Could not rename “%1” to “%2”.").arg(playlistName, newName)
                                     );
            }
            // Refresh the UI so the sidebar shows the new name
            refreshUI();
        }
        break;
    }
    case PlaylistDelete: {
        // Ask the user to confirm
        auto reply = QMessageBox::question(
            this,
            tr("Delete Playlist"),
            tr("Are you sure you want to delete the playlist “%1”?").arg(playlistName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );

        if (reply == QMessageBox::Yes) {
            // Attempt the delete
            if (!LibraryManager::instance().deletePlaylist(playlistName)) {
                QMessageBox::warning(
                    this,
                    tr("Delete Failed"),
                    tr("Could not delete playlist “%1.”").arg(playlistName)
                    );
            } else {
                // Refresh the UI so the sidebar & current view update
                refreshUI();
            }
        }
        // else: user cancelled — do nothing
        break;
    }

    default:
        break;
    }
}

bool HomePage::addToPlaylist(const QStringList &trackNamesList)
{
    connect(playListPanel, &DisplayPlaylists::playlistSelected,
            this, &HomePage::onPlaylistSelected);

    overlay_->show();
    overlay_->raise();

    playListPanel->show();
    playListPanel->raise();

    QStringList names = LibraryManager::instance().libraryManager().getPlaylistNames();
    playListPanel->setListOfTrackNames(trackNamesList);
    playListPanel->setPlaylists(names);
    return true;
}

void HomePage::onPlaylistSelected(const QString& playlistName)
{
    qDebug() << "Selected playlist:" << playlistName;
    // TODO: Load and display the tracks for 'playlistName'
    // e.g., auto tracks = m_library.loadPlaylist(playlistName);
    //       m_trackView->setTracks(tracks);
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
    LibraryManager::instance().libraryManager().setLastPlaylistPlayed(playlistName);

    QDir musicDir(LibraryManager::instance().libraryManager().getCurrentMusicDirectory());

    // Get the list of file names
    qDebug() << "playlistName in displayQueueTab()" << playlistName;
    QStringList trackNames = LibraryManager::instance().getTracksFromPlaylist(playlistName);
    qDebug() << "trackNames in displayQueueTab():" << trackNames;

    // For each track, create an item with the track name and attach the full file path
    for (const QString &trackName : trackNames) {
        QListWidgetItem *item = new QListWidgetItem(trackName);
        // Save the absolute file path for later retrieval
        QString filePath = musicDir.absoluteFilePath(trackName);
        Track *track = LibraryManager::instance().libraryManager().getTrackFromMasterPlaylist(filePath);
        item->setData(Qt::UserRole, QVariant::fromValue(track));
        item->setData(IsPlayingRole, false);

        bool isFavourite = LibraryManager::instance().libraryManager().isTrackFavourite(track);
        item->setData(IsFavouriteRole, isFavourite);

        ui->currentTracklist->addItem(item);
    }
}

void HomePage::displayPlaylistTab(){
    QStringList playlists = LibraryManager::instance().getPlaylistNames();

    for (const QString& playlistName : playlists) {
        if (playlistName == "Favourites")
            continue;

        QListWidgetItem *item = new QListWidgetItem(playlistName);
        qDebug() << "playlist in displayPlaylistTab():" << playlistName;
        ui->listOfPlaylists->addItem(item);
    }
}

void HomePage::displayFavouritesTab()
{
    ui->listOfFavourites->clear();
    QStringList favouritesList = LibraryManager::instance().libraryManager().getTracksFromPlaylist("Favourites");
    QDir musicDir(LibraryManager::instance().libraryManager().getCurrentMusicDirectory());

    for (const QString& favouriteTrack : favouritesList) {
        QListWidgetItem *item = new QListWidgetItem(favouriteTrack);
        qDebug() << "favouriteTrack in displayPlaylistTab():" << favouriteTrack;

        QString filePath = musicDir.absoluteFilePath(favouriteTrack);
        Track *track = LibraryManager::instance().libraryManager().getTrackFromMasterPlaylist(filePath);
        item->setData(Qt::UserRole, QVariant::fromValue(track));
        item->setData(IsPlayingRole, false);
        ui->listOfFavourites->addItem(item);
    }
}

// void HomePage:: on_listOfPlaylists_itemClicked(QListWidgetItem *item) {
//     QDir musicDir(LibraryManager::instance().libraryManager().getCorrentMusicDirectory());

//     ui->listOfTracks->setVisible(true);
//     ui->listOfTracks->clear();
//     QStringList tracks = LibraryManager::instance().getTracksFromPlaylist(item->text());
//     for (const QString& trackName : tracks) {
//         QListWidgetItem *trackItem = new QListWidgetItem(trackName);
//         QString filePath = musicDir.absoluteFilePath(trackName);
//         Track *track = LibraryManager::instance().libraryManager().getTrackFromMasterPlaylist(filePath);
//         trackItem->setData(Qt::UserRole, QVariant::fromValue(track));
//         trackItem->setData(PlaylistRole, item->text());
//         trackItem->setData(IsPlayingRole, false);
//         ui->listOfTracks->addItem(trackItem);
//     }
// }

void HomePage::on_currentTracklist_itemClicked(QListWidgetItem *item)
{
    // // When a track is selected in HomePage:
    // int selectedIndex = ui->currentTracklist->currentRow();
    // if (!mediaController->loadAndPlayTrack(selectedIndex)) {
    //     qDebug() << "Failed to load selected track.";
    // }
}

void HomePage::syncPlayingHighlight(const QString& playingPath)
{
    auto canonical = [](const QString& p){
        return QFileInfo(p).canonicalFilePath();
    };
    const QString target = canonical(playingPath);

    auto syncList = [&](QListWidget* list){
        auto* mdl = list->model();
        for(int i = 0; i < list->count(); ++i) {
            QListWidgetItem* item = list->item(i);
            // --- pull Track* out of the QVariant ---
            Track* track = qvariant_cast<Track*>( item->data(Qt::UserRole) );
            bool isPlay = false;
            if (track) {
                QString path = canonical(track->filePath);
                isPlay = (path == target);
            }
            QModelIndex ix = mdl->index(i, 0);
            mdl->setData(ix, isPlay, IsPlayingRole);
        }
        list->viewport()->update();
    };

    syncList(ui->currentTracklist);
    syncList(ui->listOfTracks);
    syncList(ui->listOfFavourites);
}

void HomePage::syncPlayIcons(bool isPlaying)
{
    QString playingPath = mediaController->getAudioPlayback()->getCurrentTrackPath();
    if (playingPath.isEmpty())
        return;

    auto syncList = [&](QListWidget* list) {
        for (int i = 0; i < list->count(); ++i) {
            QListWidgetItem* item = list->item(i);
            Track* track = item->data(Qt::UserRole).value<Track*>();
            if (!track) continue;

            bool match = QFileInfo(track->filePath).canonicalFilePath() ==
                         QFileInfo(playingPath).canonicalFilePath();
            item->setData(IsPlayingRole, match && isPlaying);
        }
        list->viewport()->update(); // repaint
    };

    syncList(ui->currentTracklist);
    syncList(ui->listOfTracks);
    syncList(ui->listOfFavourites);
}

void HomePage::handleFromCurrentTrackList(int row)
{
    auto* item = ui->currentTracklist->item(row);
    Track* track = qvariant_cast<Track*>( item->data(Qt::UserRole) );
    if (!track) return;

    if (!mediaController->loadAndPlayTrack(row)) {
        qDebug() << "Failed to load/play track at" << row;
    }

    // now sync the highlight across all lists
    syncPlayingHighlight(track->filePath);
    syncPlayIcons(mediaController->isPlaying());

    toggleDelegatePlayIcon(mediaController, ui->currentTracklist, mediaController->isPlaying());

    if (addContentBtn) {
        // baseline Y offset from bottom
        int yOffset = 11;
        // if a track is loaded, lift it up by extra 50px:
        if (mediaController && mediaController->getAudioPlayback()->isTrackLoaded()) {
            qDebug() << "isTrackLoaded";
            yOffset += 90;
        }

        int x = width() - addContentBtn->width() - 20;
        int y = height() - addContentBtn->height() - yOffset;
        addContentBtn->setGeometry(x, y,
                                   addContentBtn->width(),
                                   addContentBtn->height());
    }
}

void HomePage::handleFromListOfTracks(int row)
{
    auto* item = ui->listOfTracks->item(row);
    Track* track = qvariant_cast<Track*>( item->data(Qt::UserRole) );
    if (!track) return;

    ui->currentTracklist->clear();
    QString playlistName = ui->listOfTracks->item(row)->data(PlaylistRole).toString();
    displayQueueTab(playlistName);
    mediaController->initializePlaylist(playlistName);

    int selectedIndex = ui->listOfTracks->currentRow();
    if (!mediaController->loadAndPlayTrack(selectedIndex)) {
        qDebug() << "Failed to load selected track.";
    }

    syncPlayingHighlight(track->filePath);
    syncPlayIcons(mediaController->isPlaying());

    toggleDelegatePlayIcon(mediaController, ui->listOfTracks, mediaController->isPlaying());

    if (addContentBtn) {
        // baseline Y offset from bottom
        int yOffset = 11;
        // if a track is loaded, lift it up by extra 50px:
        if (mediaController && mediaController->getAudioPlayback()->isTrackLoaded()) {
            qDebug() << "isTrackLoaded";
            yOffset += 90;
        }

        int x = width() - addContentBtn->width() - 20;
        int y = height() - addContentBtn->height() - yOffset;
        addContentBtn->setGeometry(x, y,
                                   addContentBtn->width(),
                                   addContentBtn->height());
    }
}

void HomePage::handleFromFavourites(int row)
{
    auto* item = ui->listOfFavourites->item(row);
    Track* track = qvariant_cast<Track*>( item->data(Qt::UserRole) );
    if (!track) return;

    QString playlistName = "Favourites";
    mediaController->initializePlaylist(playlistName);

    int selectedIndex = ui->listOfFavourites->currentRow();
    if (!mediaController->loadAndPlayTrack(selectedIndex)) {
        qDebug() << "Failed to load selected track.";
    }

    syncPlayingHighlight(track->filePath);
    syncPlayIcons(mediaController->isPlaying());

    toggleDelegatePlayIcon(mediaController, ui->listOfFavourites, mediaController->isPlaying());

    if (addContentBtn) {
        // baseline Y offset from bottom
        int yOffset = 11;
        // if a track is loaded, lift it up by extra 50px:
        if (mediaController && mediaController->getAudioPlayback()->isTrackLoaded()) {
            qDebug() << "isTrackLoaded";
            yOffset += 90;
        }

        int x = width() - addContentBtn->width() - 20;
        int y = height() - addContentBtn->height() - yOffset;
        addContentBtn->setGeometry(x, y,
                                   addContentBtn->width(),
                                   addContentBtn->height());
    }
}

void HomePage::playlistItemClicked(const QModelIndex& idx)
{
    QListWidgetItem *item = ui->listOfPlaylists->item(idx.row());
    QDir musicDir(LibraryManager::instance().libraryManager().getCurrentMusicDirectory());

    ui->listOfTracks->setVisible(true);
    ui->listOfTracks->clear();

    displayListOfTracks(musicDir, item->text());
}

void HomePage::displayListOfTracks(QDir musicDir, QString playlistName)
{
    QStringList tracks = LibraryManager::instance().getTracksFromPlaylist(playlistName);
    for (const QString& trackName : tracks) {
        QListWidgetItem *trackItem = new QListWidgetItem(trackName);
        QString filePath = musicDir.absoluteFilePath(trackName);
        Track *track = LibraryManager::instance().libraryManager().getTrackFromMasterPlaylist(filePath);
        trackItem->setData(Qt::UserRole, QVariant::fromValue(track));
        trackItem->setData(PlaylistRole, playlistName);
        trackItem->setData(IsPlayingRole, false);

        bool isFavourite = LibraryManager::instance().libraryManager().isTrackFavourite(track);
        trackItem->setData(IsFavouriteRole, isFavourite);
        ui->listOfTracks->addItem(trackItem);
    }

    // —————————————
    // New: highlight current track if it exists in this list
    QString playing = mediaController
                          ? mediaController->getAudioPlayback()->getCurrentTrackPath()
                          : QString();
    if (!playing.isEmpty()) {
        syncPlayingHighlight(playing);
        syncPlayIcons(mediaController->isPlaying());
    }
}

void HomePage::onFavouritesIconClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;

    Track* track = qvariant_cast<Track*>(index.data(Qt::UserRole));
    if (!track) return;

    bool isCurrentlyFavourite = LibraryManager::instance().isTrackFavourite(track);

    QString trackFileName = QFileInfo(track->filePath).fileName();
    // Toggle logic
    if (isCurrentlyFavourite) {
        LibraryManager::instance().libraryManager().delTrackFromPlaylist("Favourites", trackFileName);
    } else {
        LibraryManager::instance().addTrackToPlaylist("Favourites", trackFileName);
    }

    // Update model data at the index
    QAbstractItemModel* model = const_cast<QAbstractItemModel*>(index.model());
    model->setData(index, !isCurrentlyFavourite, IsFavouriteRole);

    // Refresh all visible lists that may contain this track
    refreshUI();
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

        // baseline Y offset from bottom
        int yOffset = 11;
        // if a track is loaded, lift it up by extra 50px:
        if (mediaController && mediaController->getAudioPlayback()->isTrackLoaded()) {
            qDebug() << "isTrackLoaded";
            yOffset += 90;
        }

        int x = width() - addContentBtn->width() - 20;
        int y = newSize.height() - addContentBtn->height() - yOffset;
        addContentBtn->setGeometry(x, y,
                                   addContentBtn->width(),
                                   addContentBtn->height());
    }

    if ( addContentForm) {
         addContentForm->move(
            (width()  -  addContentForm->width())  / 2,
            (height() -  addContentForm->height()) / 2
            );
    }

    if (playListPanel)
    {
        playListPanel->move(
            (width()  -  playListPanel->width())  / 2,
            (height() -  playListPanel->height()) / 2
            );
    }

    if (overlay_) {
        overlay_->setGeometry(this->rect());
    }

    if (ui->homepageTabs) {
        int count = ui->homepageTabs->count();
        if (count == 0) return;


        // How much width each tab gets
        int tabH    = ui->homepageTabs->viewport()->height();
        int totalW  = ui->homepageTabs->viewport()->width();
        int tabW    = totalW / count - 3.2;

        // Set each item’s hint so the view lays them out equally
        for (int i = 0; i < count; ++i) {
            auto* it = ui->homepageTabs->item(i);
            it->setSizeHint(QSize(tabW, tabH));
        }

        // Force a re‐layout
        ui->homepageTabs->update();
    }
}

