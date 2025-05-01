#include "CurrentTracklistManager.h"
#include <QDir>
#include <QDebug>

CurrentTracklistManager::CurrentTracklistManager() : currentIndex(-1)
{
}

CurrentTracklistManager::~CurrentTracklistManager()
{
}

void CurrentTracklistManager::scanDirectory(const QString &directoryPath)
{
    QDir dir(directoryPath);
    QStringList filters;
    filters << "*.mp3" << "*.wav" << "*.flac" << "*.ogg";
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
    if (index >= 0 && index < tracks.size()) {
        currentIndex = index;
        return true;
    }
    qDebug() << "tracks.size():" << tracks.size();
    return false;
}

Track CurrentTracklistManager::getCurrentTrack() const
{
    if (currentIndex >= 0 && currentIndex < tracks.size())
        return tracks.at(currentIndex);
    return Track();
}

bool CurrentTracklistManager::nextTrack()
{
    if (currentIndex + 1 < tracks.size()) {
        ++currentIndex;
        return true;
    }
    return false;
}

bool CurrentTracklistManager::previousTrack()
{
    if (currentIndex - 1 >= 0) {
        --currentIndex;
        return true;
    }
    return false;
}
