#include <QDir>
#include <QRandomGenerator>
#include <qevent.h>

#include "homepage.h"
#include "ui_homepage.h"
#include "helper/qsshelper.h"


HomePage::HomePage(QWidget *parent, MediaController *externalMediaController)
    : QWidget(parent)
    , ui(new Ui::HomePage)
    , mediaController(externalMediaController)
    , addContentBtn(nullptr)
{
    ui->setupUi(this);

    ui->listOfTracks->setVisible(false);

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

HomePage::~HomePage()
{
    delete ui;
}

void HomePage::displayQueueTab(const QString& playlistName){
    QDir musicDir(playlistName);
    QStringList filters;
    filters << "*.mp3" << "*.wav" << "*.flac" << "*.ogg" << "*.opus";
    musicDir.setNameFilters(filters);

    // Get the list of file names
    qDebug() << "playlistName in displayQueueTab()" << playlistName;
    QStringList trackNames = LibraryManager::instance().getTracksFromPlaylist(playlistName);
    qDebug() << "trackNames in displayQueueTab():" << trackNames;

    // For each track, create an item with the track name and attach the full file path
    for (const QString &trackName : trackNames) {
        QListWidgetItem *item = new QListWidgetItem(trackName);
        // Save the absolute file path for later retrieval
        // item->setData(Qt::UserRole, musicDir.absoluteFilePath(trackName));
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
    // When a track is selected in HomePage:
    int selectedIndex = ui->currentTracklist->currentRow();
    if (!mediaController->loadAndPlayTrack(selectedIndex)) {
        qDebug() << "Failed to load selected track.";
    }
}

void HomePage:: on_listOfPlaylists_itemClicked(QListWidgetItem *item) {
    ui->listOfTracks->setVisible(true);
    ui->listOfTracks->clear();
    QStringList tracks = LibraryManager::instance().getTracksFromPlaylist(item->text());

    for (const QString& trackName : tracks) {
        QListWidgetItem *trackItem = new QListWidgetItem(trackName);
        qDebug() << "trackName in on_listOfPlaylists_itemClicked():" << trackName;
        trackItem->setData(Qt::UserRole, item->text());
        ui->listOfTracks->addItem(trackItem);
    }
}

void HomePage::on_listOfTracks_itemClicked(QListWidgetItem *item)
{
    ui->currentTracklist->clear();
    QString playlistName = item->data(Qt::UserRole).toString();
    displayQueueTab(playlistName);
    mediaController->initializePlaylist(playlistName);

    int selectedIndex = ui->listOfTracks->currentRow();
    if (!mediaController->loadAndPlayTrack(selectedIndex)) {
        qDebug() << "Failed to load selected track.";
    }
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
}
