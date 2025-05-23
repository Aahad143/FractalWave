#include "displayplaylists.h"
#include "ui_displayplaylists.h"
#include <qboxlayout.h>
#include <qdir.h>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include "PlaylistItemDelegate.h"
#include "librarymanager.h"

DisplayPlaylists::DisplayPlaylists(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::DisplayPlaylists)
{
    ui->setupUi(this);

    // frame styling (optional)
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Raised);

    // create list widget and add it to the frame
    m_listWidget = new QListWidget(ui->frame);
    auto layout = new QVBoxLayout(ui->frame);
    layout->addWidget(m_listWidget);
    ui->frame->setLayout(layout);

    connect(ui->addBtn, &QPushButton::clicked, this, &DisplayPlaylists::syncTrackWithSelectedPlaylists);
    connect(ui->cancelBtn, &QPushButton::clicked, this, [=](){
        hide();
        emit closed();
    });
}

void DisplayPlaylists::setPlaylists(const QStringList& names)
{
    qDebug() << "listOfTrackNames in setPlaylists:" << listOfTrackNames;
    m_listWidget->clear();

    // Call findPlaylistsContainingTrack only if one track is selected
    if (listOfTrackNames.size() == 1) {
        findPlaylistsContainingTrack(listOfTrackNames.first());
    }


    // 1) Install your custom delegate
    m_listWidget->setItemDelegate(new PlaylistItemDelegate(m_listWidget));

    // 2) Populate the model
    for (const QString& name : names) {
        if (name == "All Songs" || name == "Favourites")
        {
            continue;
        }
        if (alreadyInPlaylists.contains(name)) {
            selectedPlaylists.append(name);
        }

        auto* item = new QListWidgetItem(name, m_listWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

        // Automatically check the item if it's in alreadyInPlaylists
        Qt::CheckState state = alreadyInPlaylists.contains(name) ? Qt::Checked : Qt::Unchecked;
        item->setData(Qt::CheckStateRole, state);

        m_listWidget->addItem(item);
    }

    qDebug() << "Current alreadyInPlaylists:" << alreadyInPlaylists;
    qDebug() << "Current selectedPlaylists:" << selectedPlaylists;

    // 3) Connect to dataChanged for check tracking
    connect(m_listWidget->model(), &QAbstractItemModel::dataChanged,
            this, [&](const QModelIndex& topLeft, const QModelIndex&, const QVector<int>& roles) {
                if (roles.contains(Qt::CheckStateRole)) {
                    QString playlistName = topLeft.data(Qt::DisplayRole).toString();
                    bool checked = topLeft.data(Qt::CheckStateRole).toInt() == Qt::Checked;

                    if (checked) {
                        if (!selectedPlaylists.contains(playlistName))
                            selectedPlaylists.append(playlistName);
                    } else {
                        selectedPlaylists.removeAll(playlistName);
                    }

                    qDebug() << (checked ? "Added to" : "Removed from") << "selectedPlaylists:" << playlistName;
                    qDebug() << "Current selectedPlaylists:" << selectedPlaylists;
                }
            });
}


void DisplayPlaylists::handleSelect()
{
    // Identify which button was clicked
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    // Retrieve the playlist name stored in the button
    QVariant var = btn->property("playlistName");
    if (!var.isValid()) return;

    QString playlistName = var.toString();
    emit playlistSelected(playlistName);
}

void DisplayPlaylists::findPlaylistsContainingTrack(const QString& trackName)
{
    alreadyInPlaylists.clear();

    QJsonObject root;
    if (!LibraryManager::instance().loadJson(root)) {
        qWarning() << "Failed to load playlists JSON.";
        return;
    }

    if (!root.contains("playlists") || !root["playlists"].isObject()) {
        qWarning() << "Invalid JSON format: missing 'playlists' object.";
        return;
    }

    QJsonObject playlistsObj = root["playlists"].toObject();

    for (auto it = playlistsObj.begin(); it != playlistsObj.end(); ++it) {
        const QString playlistName = it.key();
        const QJsonArray trackNamesInPlaylist = it.value().toArray();

        for (const QJsonValue& rawTrackNameInPlaylist : trackNamesInPlaylist) {
            QString trackNameInPlaylist = rawTrackNameInPlaylist.toString();

            if (trackNameInPlaylist == trackName) {
                alreadyInPlaylists.append(playlistName);
                break; // found in this playlist, skip to next
            }
        }
    }
}

void DisplayPlaylists::syncTrackWithSelectedPlaylists()
{
    for (const QString& trackPath : listOfTrackNames) {
        QString fileName = QFileInfo(trackPath).fileName();

        // Remove track from deselected playlists
        for (const QString& playlist : alreadyInPlaylists) {
            if (!selectedPlaylists.contains(playlist)) {
                if (LibraryManager::instance().delTrackFromPlaylist(playlist, fileName)) {
                    qDebug() << "Removed" << fileName << "from playlist:" << playlist;
                } else {
                    qDebug() << "Failed to remove" << fileName << "from playlist:" << playlist;
                }
            }
        }

        // Add track to newly selected playlists
        for (const QString& playlist : selectedPlaylists) {
            if (!alreadyInPlaylists.contains(playlist)) {
                if (LibraryManager::instance().addTrackToPlaylist(playlist, fileName)) {
                    qDebug() << "Added" << fileName << "to playlist:" << playlist;
                } else {
                    qDebug() << "Failed to add" << fileName << "to playlist (might already exist):" << playlist;
                }
            }
        }
    }

    alreadyInPlaylists.clear();
    alreadyInPlaylists = selectedPlaylists;
    qDebug() << "Playlist synchronization complete.";
}

DisplayPlaylists::~DisplayPlaylists()
{
    delete ui;
}
