#include "MediaController.h"
#include <QDebug>

MediaController::MediaController(QObject* parent) :
    QObject(parent)
    , currentTracklistManager(nullptr)
    , audioPlayback(nullptr)
    , tracklist(nullptr)
    , fftTimer(new QTimer(this))
{
    // No extra initialization is needed here.
    currentTracklistManager = new CurrentTracklistManager();
    audioPlayback = new AudioPlayback();
}

MediaController::~MediaController()
{
    // Cleanup if necessary (nothing explicit needed in this case).
    delete currentTracklistManager;
    delete audioPlayback;
}

void MediaController::initializePlaylist(const QString &musicDir)
{
    qDebug() << "musicDir:" << musicDir;
    // Scan the directory to populate the playlist.
    currentTracklistManager->scanDirectory(musicDir);
    qDebug() << "Playlist initialized with" << currentTracklistManager->getTracks().size() << "tracks.";
}

void MediaController::focusCurrentTracklisstItem(int index)
{
    if (tracklist && index >= 0 && index < tracklist->count()) {
        tracklist->setCurrentRow(currentTracklistManager->getCurrentIndex());
        // Optional: scroll the current item into view.
        tracklist->scrollToItem(tracklist->currentItem(), QAbstractItemView::PositionAtCenter);
    }
}

bool MediaController::loadAndPlayTrack(int index)
{
    // Set the current track index.
    if (!currentTracklistManager->setCurrentIndex(index)) {
        qDebug() << "Invalid track index:" << index;
        return false;
    }

    // Retrieve the current track.
    Track currentTrack = currentTracklistManager->getCurrentTrack();
    qDebug() << "Loading track:" << currentTrack.filePath;

    // if (!audioPlayback->loadFile(currentTrack.filePath)) {
    //     qDebug() << "Failed to load track:" << currentTrack.filePath;
    //     return false;
    // }
    qDebug().noquote() << "audioPlayback->getCurrentTrackPath():" << audioPlayback->getCurrentTrackPath()
                       << "\ncurrentTrack.filePath:" << currentTrack.filePath;
    if (audioPlayback->getCurrentTrackPath() == currentTrack.filePath) {
        audioPlayback->togglePause();
        return true;
    }

    // Load and play the track using AudioPlayback's replaceTrack() method.
    if (!audioPlayback->replaceTrack(currentTrack.filePath)) {
        qDebug() << "Failed to reolace track:" << currentTrack.filePath;
        return false;
    }
    return true;
}

bool MediaController::nextTrack()
{
    // Advance the playlist.
    if (currentTracklistManager->nextTrack()) {
        int index = currentTracklistManager->getCurrentIndex();
        focusCurrentTracklisstItem(index);
        return loadAndPlayTrack(index);
    }
    qDebug() << "Already at the end of the playlist.";
    return false;
}

bool MediaController::previousTrack()
{
    // Move back in the playlist.
    if (currentTracklistManager->previousTrack()) {
        int index = currentTracklistManager->getCurrentIndex();
        focusCurrentTracklisstItem(index);
        return loadAndPlayTrack(index);
    }
    qDebug() << "Already at the beginning of the playlist.";
    return false;
}

AudioPlayback* MediaController::getAudioPlayback()
{
    return audioPlayback;
}

CurrentTracklistManager* MediaController::getCurrentTracklistManager()
{
    return currentTracklistManager;
}

void MediaController::setTracklist(QListWidget *tl)
{
    tracklist = tl;
}
