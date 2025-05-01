#ifndef CURRENTTRACKLISTMANAGER_H
#define CURRENTTRACKLISTMANAGER_H

#include <QString>
#include <QList>

struct Track {
    QString filePath;
    QString title;
};

class CurrentTracklistManager
{
public:
    CurrentTracklistManager();
    ~CurrentTracklistManager();

    // Scans a directory and populates the playlist.
    void scanDirectory(const QString &directoryPath);

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

private:
    QList<Track> tracks;
    int currentIndex;
};

#endif // CURRENTTRACKLISTMANAGER_H
