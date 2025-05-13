#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QFile>
#include <QDateTime>
#include <QMessageBox>
#include <QRegularExpression>

#include "helper/directoryhelper.h"
#include "librarymanager.h"

LibraryManager::LibraryManager(QObject *parent)
    : QObject{parent},
    masterPlaylist(nullptr),
    settings("FractalWave", "FractalWave")
{
    masterPlaylist = new Playlist("All Songs");
    // ensure the file exists at startup
    ensurePlaylistsFileExists();
}

bool LibraryManager::scanDirectory()
{
    qDebug() << "Scanning directory";
    // QSettings settings("FractalWave", "FractalWave");
    QString dir = settings.value("musicFolder", "").toString();
    if (dir.isEmpty()) {
        qDebug() << "Directory in QSettings is empty";
        return false;
    }

    qDebug() << " current music dir in scanDirectory():" << dir;

    QDir musicDir(dir);
    if (!musicDir.exists()) {
        qDebug() << "Directory doesn't exist in device";
        return false;
    }

    // Persist back
    settings.setValue("musicFolder", dir);
    currentMusicDirectory = dir;

    qDebug() << "currentMusicDirectory in scanDirectory():" << currentMusicDirectory;

    // 1) Scan folder for audio files
    const QStringList nameFilters = { "*.mp3", "*.wav", "*.flac", "*.ogg", ".opus" };
    // note: pass filters, flags, then sort
    QStringList fileList = musicDir.entryList(
        nameFilters,
        QDir::Files | QDir::NoSymLinks,
        QDir::Name
        );

    masterPlaylist->clear();
    masterPlaylist->setName("All Songs");

    QSet<QString> validPaths;
    validPaths.reserve(fileList.size());

    for (const QString& fname : fileList) {
        QString fullPath = musicDir.absoluteFilePath(fname);
        validPaths.insert(fullPath);

        Track t;
        t.filePath = fullPath;
        t.title    = QFileInfo(fullPath).baseName();
        qDebug() << "fullPath in scanDirectory for loop" << t.filePath;
        qDebug() << "title in scanDirectory for loop" << t.title;
        masterPlaylist->addTrack(t);
    }

    if (masterPlaylist->getTracks().empty())
        return false;

    // 2) Load playlists JSON
    QJsonObject root;
    if (!loadJson(root))
        return true;  // no JSON to prune

    QJsonObject pls = root.value(QStringLiteral("playlists")).toObject();
    bool modified = false;

    // 3) Prune each playlist
    for (const QString& name : pls.keys()) {
        QJsonArray oldArr = pls.value(name).toArray();
        QJsonArray newArr;

        for (const QJsonValue& v : oldArr) {
            QString trackName = v.toString();
            if (musicDir.exists(trackName )) {
                newArr.append(trackName );
            }
            else
                modified = true;
        }

        if (newArr.size() != oldArr.size())
            pls[name] = newArr;
    }

    if (modified) {
        root[QStringLiteral("playlists")] = pls;
        saveJson(root);
    }

    // Set lastPlaylistPlayed
    lastPlaylistPlayed = root.value(QStringLiteral("last_playlist_in_queue")).toArray().at(0).toString();

    return true;
}



QString LibraryManager::playlistsFilePath() const
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    return dataDir + "/playlists.json";
}

bool LibraryManager::ensurePlaylistsFileExists()
{
    QString path = playlistsFilePath();
    qDebug() << "playlistsFilePath() in saveJson():" << playlistsFilePath();
    QFile file(path);
    if (file.exists())
        return true;

    // create {"playlists":{}}
    QJsonObject root;
    root["playlists"] = QJsonObject{};
    QJsonDocument doc(root);

    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(doc.toJson());
    file.close();
    return true;
}

bool LibraryManager::loadJson(QJsonObject& root)
{
    QFile file(playlistsFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return false;
    auto doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
        return false;
    root = doc.object();
    return true;
}

// Helper: replace illegal filename chars with '_'
static QString sanitizeForFilename(const QString& raw) {
    QString s = raw;
    s.replace(QRegularExpression(R"([^A-Za-z0-9 _\.-])"), "_");
    s.replace(QRegularExpression(R"(_{2,})"), "_");
    return s.trimmed();
}

bool LibraryManager::addTrackToDIr(const QString& youtubeUrl)
{
    // Ensure we have tool paths
    if (ffmpegPath_.isEmpty()) ffmpegPath_ = findExecutableInAppDir("ffmpeg/bin", "ffmpeg.exe");
    if (ytdlpPath_.isEmpty())  ytdlpPath_  = findExecutableInAppDir("yt-dlp", "yt-dlp.exe");

    if (ffmpegPath_.isEmpty() || ytdlpPath_.isEmpty()) {
        QMessageBox::critical(nullptr, tr("Missing Tools"),
                              tr("This feature requires ffmpeg and yt‑dlp.\n"
                                 "Please make sure they are installed or included in the app bundle."));
        return false;
    }

    // 1) figure out what the filename *will* be
    qDebug() << "currentMusicDirectory in addTracksToDIr():" << currentMusicDirectory;
    QString inputPath = getOutputFilename(ytdlpPath_, youtubeUrl, currentMusicDirectory);
    qDebug() << "Will download to:" << inputPath ;

    // 2) then actually download it there
    if (downloadVideo(ytdlpPath_, youtubeUrl, currentMusicDirectory)) {
        QMessageBox::critical(nullptr, tr("Download failed"),
                              tr("Make sure the link is a valid YouTube link."));
        return false;
    }

    qDebug() << "Download finished!";

    int dotIndex = inputPath .lastIndexOf('.');
    QString outputPath = (dotIndex != -1) ? inputPath .left(dotIndex) + ".ogg" : inputPath  + ".ogg";

    // 3) remux the .mp4 file into .ogg format
    if (remuxVideo(inputPath, outputPath))
    {
        QMessageBox::critical(nullptr, tr("FFmpeg remux failed"),
                              tr("Video file doesn't exist or is malformed."));
        return false;
    }

    // 4) Verify file exists
    if (!QFile::exists(outputPath)) {
        qWarning() << "Output file missing:" << outputPath;
        return false;
    }

    if (QFile::exists(inputPath)) {
        if (!QFile::remove(inputPath)) {
            qWarning() << "Failed to delete temp .mp4 file:" << inputPath;
            // (you can choose to continue or return false here)
        }
        else {
            qDebug() << "Deleted temp .mp4 file:" << inputPath;
        }
    }

    // trigger a rescan so the master playlist picks it up
    scanDirectory();

    return true;
}

QString LibraryManager::getOutputFilename(const QString& ytdlpPath,
                          const QString& youtubeUrl,
                          const QString& outputFolder)
{
    qDebug() << "outputFolder in getOutputFilename():" << outputFolder;
    QProcess p;
    // Build the argument list
    QStringList args;
    args << "--get-filename"        // just print the filename
         << "-o" << "%(title)s.mp4"  // use clean template
         << youtubeUrl;

    p.start(ytdlpPath, args);
    if (!p.waitForFinished(-1))
        throw std::runtime_error("yt-dlp timed out");

    QString filename = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    filename = sanitizeForFilename(filename);
    // prepend your desired folder
    return QDir(outputFolder).filePath(filename);
}

bool LibraryManager::downloadVideo(const QString& ytdlpPath,
                   const QString& youtubeUrl,
                   const QString& outputFolder)
{
    QProcess p;
    QStringList args;
    args << "-o" << (outputFolder + "/%(title)s.%(ext)s")
         << "--merge-output-format" << "mp4"
        << "--add-metadata"
        << "--embed-thumbnail"
         << youtubeUrl;

    p.start(ytdlpPath, args);
    if (!p.waitForFinished(-1))
        throw std::runtime_error("yt-dlp download failed or was interrupted");
        return false;

    return true;
}

bool LibraryManager::remuxVideo(const QString& inputPath, const QString& outputPath)
{
    QProcess ff;
    QStringList args = {
        "-y",
        "-i", inputPath,
        "-map_metadata", "0",       // copy all global metadata from input
        "-vn",
        "-c:a", "libvorbis",
        "-q:a", "10",
        outputPath
    };
    ff.start(ffmpegPath_, args);
    if (!ff.waitForFinished(-1) ||
        ff.exitStatus() != QProcess::NormalExit ||
        ff.exitCode()   != 0)
    {
        qWarning() << "ffmpeg failed:" << ff.readAllStandardError();
        return false;
    }

    return true;
}

bool LibraryManager::saveJson(const QJsonObject& root)
{
    QFile file(playlistsFilePath());
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool LibraryManager::createAndSavePlaylist(const QString& playlistName)
{
    QJsonObject root;
    if (!loadJson(root))
        return false;

    QJsonObject pls = root.value("playlists").toObject();
    if (pls.contains(playlistName))
        return false;  // already exists

    // insert empty array
    pls[playlistName] = QJsonArray{};
    root["playlists"] = pls;
    return saveJson(root);
}

bool LibraryManager::addTrackToPlaylist(const QString& playlistName, const QString& trackPath)
{
    QJsonObject root;
    if (!loadJson(root))
        return false;

    QJsonObject pls = root.value("playlists").toObject();
    if (!pls.contains(playlistName))
        return false;  // no such playlist

    QJsonArray arr = pls.value(playlistName).toArray();
    // check for duplicate
    for (auto v : arr) {
        if (v.toString() == trackPath)
            return false;
    }

    arr.append(trackPath);
    pls[playlistName] = arr;
    root["playlists"]   = pls;
    return saveJson(root);
}

QStringList LibraryManager::getPlaylistNames(){
    QFile file(playlistsFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Couldn't open playlists.json";
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        qWarning() << "Malformed JSON";
    }

    QJsonObject rootObj       = doc.object();
    QJsonObject playlistsObj  = rootObj.value("playlists").toObject();

    return playlistsObj.keys();
}

QStringList LibraryManager::getTracksFromPlaylist(const QString& playlistName) {
    QFile file(playlistsFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Couldn't open playlists.json";
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        qWarning() << "Malformed JSON";
    }

    QJsonObject rootObj       = doc.object();
    QJsonObject playlistsObj  = rootObj.value("playlists").toObject();

    QJsonArray tracks = playlistsObj.value(playlistName).toArray();

    QStringList result;
    result.reserve(tracks.size());
    for (const QJsonValue& val : tracks)
        result.append(val.toString());

    return result;
}
