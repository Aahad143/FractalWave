#ifndef MEDIACONTROLLER_H
#define MEDIACONTROLLER_H

#include <QString>
#include <qlistwidget.h>
#include "CurrentTracklistManager.h"
#include "audioplayback.h"
#include <QTimer>

class MediaController : public QObject
{
    Q_OBJECT
public:
    MediaController(QObject* parent = nullptr);
    ~MediaController();

    // Initializes the playlist by scanning the given music directory.
    void initializePlaylist(const QString &musicDir);

    // method for focusing on current tracklist item
    void focusCurrentTracklisstItem(int index);

    // Loads and plays the track at the given index.
    bool loadAndPlayTrack(int index);

    // Moves to the next track and plays it.
    bool nextTrack();

    // Moves to the previous track and plays it.
    bool previousTrack();

    // Provides access to the AudioPlayback instance.
    AudioPlayback* getAudioPlayback();

    // (Optional) Provides access to the CurrentTracklistManager.
    CurrentTracklistManager* getCurrentTracklistManager();

    QString currentMusicFolder() const { return currentMusicFolder_; }
    void    setCurrentMusicFolder(const QString& dir) { currentMusicFolder_ = dir; }

    void setTracklist(QListWidget *tracklist);

    // void updateFFTData();
private:
    CurrentTracklistManager *currentTracklistManager;
    AudioPlayback *audioPlayback;
    QListWidget *tracklist;

    QTimer* fftTimer; // Timer for updating FFT data.

    QString currentMusicFolder_;
};

#endif // MEDIACONTROLLER_H
