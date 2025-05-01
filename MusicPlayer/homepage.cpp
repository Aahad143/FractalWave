#include "homepage.h"
#include "ui_homepage.h"
#include "helper/qsshelper.h"
#include <QDir>
#include <QRandomGenerator>

HomePage::HomePage(QWidget *parent, MediaController *externalMediaController)
    : QWidget(parent)
    , ui(new Ui::HomePage)
    , mediaController(externalMediaController)
{
    ui->setupUi(this);


    this->setStyleSheet(applyStyles(":/resources/styles/homepage.qss"));        // Apply the stylesheet to the homepage

    // Define the path to your music folder
    if (mediaController) {
        mediaController->setTracklist(ui->currentTracklist);
        const QString musicFolderPath = "C:\\Users\\aahad\\Music";
        displayTrackList(musicFolderPath);
        mediaController->initializePlaylist(musicFolderPath);
    }
    else {
        qDebug() << "MediaController is nullptr";
    }

    // Connect item selection to page change
    connect(ui->homepageTabs, &QListWidget::currentRowChanged, this, &HomePage::changePage);
}

HomePage::~HomePage()
{
    delete ui;
}

void HomePage::displayTrackList(const QString& musicFolderPath){
    QDir musicDir(musicFolderPath);
    QStringList filters;
    filters << "*.mp3" << "*.wav" << "*.flac" << "*.ogg";
    musicDir.setNameFilters(filters);

    // Get the list of file names
    QStringList trackNames = musicDir.entryList(QDir::Files);
    qDebug() << "trackNames:" << trackNames;

    // For each track, create an item with the track name and attach the full file path
    for (const QString &trackName : trackNames) {
        QListWidgetItem *item = new QListWidgetItem(trackName);
        // Save the absolute file path for later retrieval
        item->setData(Qt::UserRole, musicDir.absoluteFilePath(trackName));
        ui->currentTracklist->addItem(item);
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

void HomePage::changePage(int index)
{
    ui->stackedWidget->setCurrentIndex(index);
}
