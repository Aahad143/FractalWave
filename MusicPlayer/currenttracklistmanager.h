#ifndef CURRENTTRACKLISTMANAGER_H
#define CURRENTTRACKLISTMANAGER_H

#include "playlist.h"
#include <QString>
#include <QList>
#include <track.h>

#include "librarymanager.h"

class CurrentTracklistManager
{
public:
    CurrentTracklistManager();
    ~CurrentTracklistManager();

    // Scans a directory and populates the playlist.
    void scanDirectory(const QString &directoryPath);

    void initializePlaylist(const QString& playlistName);
    // Get the list of tracks.
    QList<Track> getTracks() const;

    // Get the current track index.
    int getCurrentIndex() const;

    // Set the current track (returns true if valid).
    bool setCurrentIndex(int index);

    // Get the current track.
    Track getCurrentTrack() const;

    // Move to the next track; returns true if successful.
    bool nextTrack();

    // Move to the previous track; returns true if successful.
    bool previousTrack();

    /// Access to the current playlist
    const Playlist& getCurrentPlaylist() const    { return currentPlaylist; }

signals:
    /// emitted whenever initializePlaylist() loads a new list
    void playlistChanged(const QString& newName);

private:
    QList<Track> tracks;
    Playlist currentPlaylist;
    int currentIndex;
};

#endif // CURRENTTRACKLISTMANAGER_H
