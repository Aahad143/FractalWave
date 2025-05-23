#include "CurrentTracklistManager.h"
#include <QDir>
#include <QDebug>

CurrentTracklistManager::CurrentTracklistManager() :
    currentIndex(-1),
    currentPlaylist("")
{
}

CurrentTracklistManager::~CurrentTracklistManager()
{
}

void CurrentTracklistManager::scanDirectory(const QString &directoryPath)
{
    QDir dir(directoryPath);
    QStringList filters;
    filters << "*.mp3" << "*.wav" << "*.flac" << "*.ogg" << "*.opus";
    dir.setNameFilters(filters);

    tracks.clear();

    // For each file, create a Track.
    QStringList fileNames = dir.entryList(QDir::Files);
    for (const QString &fileName : fileNames) {
        Track track;
        track.filePath = dir.absoluteFilePath(fileName);
        track.title = fileName; // For simplicity, use fileName as title.
        tracks.append(track);
    }

    // Set currentIndex to 0 if there are any tracks.
    if (!tracks.isEmpty())
        currentIndex = 0;
    else
        currentIndex = -1;
}

void CurrentTracklistManager::initializePlaylist(const QString& playlistName)
{
    QDir musicDir(LibraryManager::instance().getCurrentMusicDirectory());
    // 1) fetch the file-paths from LibraryManager
    QStringList trackNames =
        LibraryManager::instance().getTracksFromPlaylist(playlistName);

    // 2) reset our internal Playlist
    currentPlaylist.clear();
    currentPlaylist.setName(playlistName);

    // 3) build Track objects and add them
    for (const QString& trackName : trackNames)
    {
        Track t;
        t.filePath = musicDir.filePath(trackName);
        t.title    = QFileInfo(trackName).baseName();
        currentPlaylist.addTrack(t);
    }

    // emit playlistChanged(playlistName);
}


QList<Track> CurrentTracklistManager::getTracks() const
{
    return tracks;
}

int CurrentTracklistManager::getCurrentIndex() const
{
    return currentIndex;
}

bool CurrentTracklistManager::setCurrentIndex(int index)
{
    if (index >= 0 && index < currentPlaylist.size()) {
        currentIndex = index;
        return true;
    }
    qDebug() << "currentPlaylist.size():" << currentPlaylist.size();
    return false;
}

Track CurrentTracklistManager::getCurrentTrack() const
{
    if (currentIndex >= 0 && currentIndex < currentPlaylist.size())
        return currentPlaylist.at(currentIndex);
    return Track();
}

bool CurrentTracklistManager::nextTrack()
{
    if (currentIndex + 1 < currentPlaylist.size()) {
        ++currentIndex;
    }
    else {
        currentIndex = 0;
    }
    return true;
}

bool CurrentTracklistManager::previousTrack()
{
    if (currentIndex - 1 >= 0) {
        --currentIndex;
    }
    else {
        currentIndex = currentPlaylist.size()-1;
    }
    return true;
}
