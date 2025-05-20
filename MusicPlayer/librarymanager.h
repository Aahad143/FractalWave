#ifndef LIBRARYMANAGER_H
#define LIBRARYMANAGER_H

#include "playlist.h"
#include <QObject>
#include <QSettings>

class LibraryManager : public QObject
{
    Q_OBJECT
public:
    /// Access the one—and only—instance
    static LibraryManager& instance()
    {
        static LibraryManager mgr;
        return mgr;
    }

    // delete copy/move so nobody can accidentally clone this class
    LibraryManager(const LibraryManager&) = delete;
    LibraryManager& operator=(const LibraryManager&) = delete;

    /// A shorthand for LibraryManager::instance()
    inline LibraryManager& libraryManager()
    {
        return LibraryManager::instance();
    }

    /// Reads last folder from QSettings, scans it, and fills masterPlaylist.
    /// Returns true if the directory existed and contains at least one track.
    bool scanDirectory();

    bool readMusicFolder(QString &outDir);
    bool validateDirectory(const QString &dir);
    QStringList listAudioFiles(const QString &dir);
    bool populateMasterPlaylist(const QStringList &fileNames, QJsonObject &root);
    void prunePlaylists(QJsonObject &root, const QString &musicDir);
    /// Make sure the JSON file exists on disk. Returns true on success.
    bool ensurePlaylistsFileExists();

    // bool loadPlaylist(QString playlistName);
    bool createAndSavePlaylist(const QString& playlistName);
    bool deletePlaylist(const QString& playlistName);
    bool addTrackToPlaylist(const QString& playlistName, const QString& trackPath);
    bool delTrackFromPlaylist(const QString& playlistName, const QString& trackPath);

    bool addTrackToDIr(const QString& trackPath);
    bool delTrackFromDir(const QString& trackPath);

    Playlist& getMasterPlaylist()         {return *masterPlaylist;}

    const QString& getCorrentMusicDirectory() {
        return currentMusicDirectory;
    }

    void setCurrentMusicDirectory (const QString& newDir) {
        currentMusicDirectory = newDir;
    }

    void setLastPlaylistPlayed(QString& playlistName){
        lastPlaylistPlayed = playlistName;
        settings.setValue("lastPlaylistPlayed", lastPlaylistPlayed);
    }

    QString& getLastPlaylistPlayed(){
        return lastPlaylistPlayed;
    }

    QStringList getPlaylistNames();
    QStringList getTracksFromPlaylist(const QString& playlistName);

signals:

private:
    // only instance() may create one
    explicit LibraryManager(QObject *parent = nullptr);
    ~LibraryManager() override = default;

    QSettings settings;

    Playlist *masterPlaylist;
    QString currentMusicDirectory;
    QString lastPlaylistPlayed;

    QString playlistsFilePath() const;

    /// Helper to load the root JSON object from disk.
    bool loadJson(QJsonObject& root);

    /// Helper to write the root JSON object back to disk.
    bool saveJson(const QJsonObject& root);

    QString ffmpegPath_;
    QString ytdlpPath_;

    QString getOutputFilename(const QString& ytdlpPath,
                                              const QString& youtubeUrl,
                                              const QString& outputFolder);

    bool downloadVideo(const QString& ytdlpPath,
                                       const QString& youtubeUrl,
                       const QString& outputFolder);

    bool remuxVideo(const QString& inputPath, const QString& outputPath);

};

#endif // LIBRARYMANAGER_H
