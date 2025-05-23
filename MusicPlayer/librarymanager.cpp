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

    // Ensure we have tool paths
    if (ffmpegPath_.isEmpty()) ffmpegPath_ = findExecutableInAppDir("ffmpeg/bin", "ffmpeg.exe");
    if (ffprobePath_.isEmpty()) ffprobePath_ = findExecutableInAppDir("ffmpeg/bin", "ffprobe.exe");
    if (ytdlpPath_.isEmpty())  ytdlpPath_  = findExecutableInAppDir("yt-dlp", "yt-dlp.exe");
}

bool LibraryManager::scanDirectory()
{
    qDebug() << "Scanning directory";

    QString dir;
    if (!readMusicFolder(dir))

    currentMusicDirectory = dir;
    settings.setValue("musicFolder", dir);


    // 2) Prune per‑saved playlists JSON
    QJsonObject root;
    if (loadJson(root)) {
        // 1) Gather audio files and populate master playlist
        QStringList fileList = listAudioFiles(dir);
        if (!populateMasterPlaylist(fileList, root))
            return false;

        prunePlaylists(root, dir);
    }

    return true;
}
bool LibraryManager::readMusicFolder(QString &outDir)
{
    outDir = settings.value("musicFolder", "").toString();
    if (outDir.isEmpty()) {
        qDebug() << "Directory in QSettings is empty";
        return false;
    }
    qDebug() << "Configured music dir:" << outDir;
    return true;
}

bool LibraryManager::validateDirectory(const QString &dir)
{
    QDir d(dir);
    if (!d.exists()) {
        qDebug() << "Directory doesn't exist on disk:" << dir;
        return false;
    }
    qDebug() << "Validated music directory:" << dir;
    return true;
}

bool LibraryManager::populateMasterPlaylist(const QStringList &fileNames, QJsonObject &root)
{
    // 1) Clear & refill in‑memory playlist
    masterPlaylist->clear();
    masterPlaylist->setName("All Songs");
    if (fileNames.isEmpty())
        return false;

    QDir d(currentMusicDirectory);
  
void LibraryManager::prunePlaylists(QJsonObject &root, const QString &musicDir)
{
    QJsonObject pls = root.value("playlists").toObject();
    bool modified = false;
    QDir d(musicDir);

    for (const QString &name : pls.keys()) {
        QJsonArray oldArr = pls.value(name).toArray();
        QJsonArray newArr;
        for (const QJsonValue &v : oldArr) {
            QString trackName = v.toString();
            if (d.exists(trackName))
                newArr.append(trackName);
            else
                modified = true;
        }
        if (newArr.size() != oldArr.size())
            pls[name] = newArr;
    }

    if (modified) {
        root["playlists"] = pls;
        qDebug() << "Pruned playlists JSON, saving changes";
    } else {
        qDebug() << "No playlist entries needed pruning";
    }
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
    qDebug() << "playlistsFilePat() in saveJson():" << playlistsFilePath();
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

bool LibraryManager::addTrackToDIr(const QString& youtubeUrl)
{
    if (ffmpegPath_.isEmpty() || ytdlpPath_.isEmpty()) {
        QMessageBox::critical(nullptr, tr("Missing Tools"),
                              tr("This feature requires ffmpeg and yt-dlp.\n"
                                 "Please make sure they are installed or included in the app bundle."));
        return false;
    }

    // 1) figure out what the filename *will* be
    QString inputPath = getOutputFilename(ytdlpPath_, youtubeUrl, currentMusicDirectory);
    qDebug() << "Will download to:" << inputPath ;

    // ——— Check: avoid duplicates by base name ———
    {
        QFileInfo fi(inputPath);
        QString baseName = fi.completeBaseName();               // name without extension
        QDir musicDir(currentMusicDirectory);
        // match any file like "baseName.*"
        QStringList matches = musicDir.entryList(
            QStringList() << baseName + ".*",
            QDir::Files | QDir::NoSymLinks
            );
        if (!matches.isEmpty()) {
            QMessageBox::warning(nullptr,
                                 tr("Duplicate Track"),
                                 tr("A track named \"%1\" already exists in your library.")
                                     .arg(baseName)
                                 );
            return false;
        }
    }

    if (!downloadVideo(ytdlpPath_, youtubeUrl, currentMusicDirectory)) {
        QMessageBox::critical(nullptr, tr("Download failed"),
                              tr("Make sure the link is a valid YouTube link."));
        return false;
    }

    if (!downloadCoverArt(youtubeUrl) ) {
        QMessageBox::warning(nullptr,
                             tr("Cover Art Failed"),
                             tr("Unable to download cover art for this track.")
                             );
    }

    qDebug() << "Download finished!";

    int dotIndex = inputPath.lastIndexOf('.');
    QString outputPath = (dotIndex != -1)
                             ? inputPath.left(dotIndex) + ".ogg"
                             : inputPath + ".ogg";

        QMessageBox::critical(nullptr, tr("FFmpeg remux failed"),
                              tr("Video file doesn't exist or is malformed."));
        return false;
    }

    if (!QFile::exists(outputPath)) {
        qWarning() << "Output file missing:" << outputPath;
        return false;
    }

    if (QFile::exists(inputPath)) {
        if (!QFile::remove(inputPath)) {
            qWarning() << "Failed to delete temp .mp4 file:" << inputPath;
        } else {
            qDebug() << "Deleted temp .mp4 file:" << inputPath;
        }
    }

    // trigger a rescan so the master playlist picks it up
    scanDirectory();
    return true;
}

QString LibraryManager::getOutputFilename(
    const QString& ytdlpPath,
    const QString& youtubeUrl,
    const QString& outputFolder)
{
    QProcess p;
    // force utf‑8 from yt‑dlp
    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("PYTHONIOENCODING", "utf-8");
    p.setProcessEnvironment(env);

    QStringList args;
    args << "--get-filename"
         << "--encoding" << "utf-8"
         << "-o" << "%(title)s.mp4"
         << youtubeUrl;

    p.start(ytdlpPath, args);
    if (!p.waitForFinished(-1))
        throw std::runtime_error("yt-dlp timed out");

    // decode as UTF‑8 now that we forced yt-dlp to output it
    QString filename = QString::fromUtf8(
                           p.readAllStandardOutput()
                           ).trimmed();

    return QDir(outputFolder).filePath(filename);
}

bool LibraryManager::downloadCoverArt(const QString& youtubeUrl)
{
    // 1) Ensure the yt-dlp path is set up
    if (ytdlpPath_.isEmpty())
        ytdlpPath_ = findExecutableInAppDir("yt-dlp", "yt-dlp.exe");  // implement this helper as you do elsewhere

    QString tool = ytdlpPath_.isEmpty() ? "yt-dlp" : ytdlpPath_;

    // 2) Prepare the output directory (same as playlists.json)
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    // 3) Build arguments: skip video, write thumbnail, convert to jpg, output into dataDir
    QString outputTemplate = dataDir + QDir::separator() + "%(title)s";
    QStringList args = {
        "--skip-download",
        "--write-thumbnail",
        "--convert-thumbnails", "jpg",
        "-o", outputTemplate,
        youtubeUrl
    };

    // 4) Run yt-dlp
    QProcess proc;
    proc.start(tool, args);
    if (!proc.waitForFinished(-1)) {
        qWarning() << "downloadCoverArt: yt-dlp did not finish";
        return false;
    }
    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        qWarning() << "downloadCoverArt: yt-dlp failed:" << proc.readAllStandardError();
        return false;
    }

    // 5) Success
    qDebug() << "downloadCoverArt: thumbnail downloaded to" << dataDir;
    return true;
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
    if (!p.waitForFinished(-1)){
        qWarning() << "downloadVideo: yt-dlp did not finish";
        return false;
    }

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

bool LibraryManager::addTrackToPlaylist(const QString& playlistName, const QString& trackName)
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
        if (v.toString() == trackName)
            return false;
    }

    arr.append(trackName);
    pls[playlistName] = arr;
    root["playlists"]   = pls;

    scanDirectory();
    return saveJson(root);
}

bool LibraryManager::delTrackFromPlaylist(const QString& playlistName, const QString& trackName)
{
    QJsonObject root;
    if (!loadJson(root)) {
        qWarning() << "Failed to load playlists JSON.";
        return false;
    }

    if (!root.contains("playlists") || !root["playlists"].isObject()) {
        qWarning() << "Invalid playlists JSON format.";
        return false;
    }

    QJsonObject playlists = root["playlists"].toObject();

    if (!playlists.contains(playlistName) || !playlists[playlistName].isArray()) {
        qWarning() << "Playlist" << playlistName << "not found or invalid.";
        return false;
    }

    QJsonArray tracksArray = playlists[playlistName].toArray();
    QJsonArray newArray;
    bool found = false;

    for (const QJsonValue& v : tracksArray) {
        if (v.toString() == trackName) {
            found = true;
            continue;
        }
        newArray.append(v);
    }

    if (!found) {
        qDebug() << "Track not found in playlist:" << trackName;
        return false;
    }

    playlists[playlistName] = newArray;
    root["playlists"] = playlists;

    if (!saveJson(root)) {
        qWarning() << "Failed to save updated playlists JSON.";
        return false;
    }

    qDebug() << "Track" << trackName << "removed from playlist" << playlistName;
    scanDirectory();
    return true;
}

bool LibraryManager::delTrackFromDir(const QString& trackName)
{
    QDir dir(currentMusicDirectory);
    QString trackPath = dir.absoluteFilePath(trackName);

    QFileInfo trackInfo(trackPath);
    if (!trackInfo.exists() || !trackInfo.isFile()) {
        qWarning() << "Track not found:" << trackPath;
        return false;
    }

    // Confirm it's inside the current music directory
    QDir musicDir(currentMusicDirectory);
    QString relativePath = musicDir.relativeFilePath(trackPath);
    if (relativePath.startsWith("..")) {
        qWarning() << "Track is not inside the current music directory:" << trackPath;
        return false;
    }

    // Remove the audio file
    if (!QFile::remove(trackPath)) {
        qWarning() << "Failed to delete track file:" << trackPath;
        return false;
    }

    // Remove the cover image (if any)
    qDebug() << "QFileInfo(trackPath).baseName() in delTrackFromDir:" << QFileInfo(trackPath).baseName();
    QString coverImagePath = retrieveCoverImagePath(QFileInfo(trackPath).baseName());
    if (QFile::exists(coverImagePath)) {
        QFile::remove(coverImagePath); // Best-effort, non-fatal if it fails
    }

    // Refresh library (this prunes any playlists containing this track)
    scanDirectory();

    qDebug() << "Deleted track and cover image successfully:" << trackPath;
    return true;
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

void LibraryManager::setLastPlaylistPlayed(const QString& playlistName)
{
    // 1) Load the JSON
    QJsonObject root;
    if (!loadJson(root)) {
        qWarning() << "setLastPlaylistPlayed: Failed to load playlists.json";
        return;
    }

    // 2) Replace last_playlist_in_queue with a single-element array
    QJsonArray arr;
    arr.append(playlistName);
    root["last_playlist_in_queue"] = arr;

    // 3) Save back to disk
    if (!saveJson(root)) {
        qWarning() << "setLastPlaylistPlayed: Failed to save playlists.json";
        return;
    }

    // 4) Update in-memory state & QSettings
    lastPlaylistPlayed = playlistName;
    settings.setValue("lastPlaylistPlayed", lastPlaylistPlayed);

    qDebug() << "Last playlist in queue set to:" << playlistName;
}

bool LibraryManager::renamePlaylist(const QString& oldName,
                                    const QString& newName)
{
    // 1) Load the JSON
    QJsonObject root;
    if (!loadJson(root)) {
        qWarning() << "renamePlaylist: failed to load JSON";
        return false;
    }

    // 2) Access the playlists object
    if (!root.contains("playlists") || !root["playlists"].isObject()) {
        qWarning() << "renamePlaylist: malformed playlists JSON";
        return false;
    }
    QJsonObject pls = root["playlists"].toObject();

    // 3) Check existence
    if (!pls.contains(oldName)) {
        qWarning() << "renamePlaylist: no such playlist" << oldName;
        return false;
    }
    if (pls.contains(newName)) {
        qWarning() << "renamePlaylist: target name already exists" << newName;
        return false;
    }

    // 4) Move the array under the new key
    QJsonArray arr = pls.value(oldName).toArray();
    pls.remove(oldName);
    pls.insert(newName, arr);
    root["playlists"] = pls;

    // 5) Update last_playlist_in_queue if needed
    if (root.contains("last_playlist_in_queue") &&
        root["last_playlist_in_queue"].isArray())
    {
        QJsonArray lastArr = root["last_playlist_in_queue"].toArray();
        for (int i = 0; i < lastArr.size(); ++i) {
            if (lastArr[i].toString() == oldName) {
                lastArr[i] = newName;
            }
        }
        root["last_playlist_in_queue"] = lastArr;
    }

    // 6) Persist JSON
    if (!saveJson(root)) {
        qWarning() << "renamePlaylist: failed to save JSON";
        return false;
    }

    // 7) Update in-memory & settings if we were tracking this
    if (lastPlaylistPlayed == oldName) {
        lastPlaylistPlayed = newName;
        settings.setValue("lastPlaylistPlayed", lastPlaylistPlayed);
    }

    return true;
}

bool LibraryManager::deletePlaylist(const QString& playlistName)
{
    // 1) Load existing JSON
    QJsonObject root;
    if (!loadJson(root)) {
        qWarning() << "deletePlaylist: failed to load playlists JSON";
        return false;
    }

    // 2) Locate "playlists" object
    if (!root.contains("playlists") || !root["playlists"].isObject()) {
        qWarning() << "deletePlaylist: malformed JSON (missing \"playlists\")";
        return false;
    }
    QJsonObject pls = root["playlists"].toObject();

    // 3) Verify the playlist exists
    if (!pls.contains(playlistName)) {
        qWarning() << "deletePlaylist: no such playlist" << playlistName;
        return false;
    }

    // 4) Remove it
    pls.remove(playlistName);
    root["playlists"] = pls;

    // 5) If this playlist was in last_playlist_in_queue, remove it
    if (root.contains("last_playlist_in_queue") &&
        root["last_playlist_in_queue"].isArray())
    {
        QJsonArray lastArr = root["last_playlist_in_queue"].toArray();
        QJsonArray newLast;
        for (auto val : lastArr) {
            if (val.toString() != playlistName)
                newLast.append(val);
        }
        root["last_playlist_in_queue"] = newLast;
    }

    // 6) Persist the change
    if (!saveJson(root)) {
        qWarning() << "deletePlaylist: failed to save updated JSON";
        return false;
    }

    // 7) If we were tracking this as lastPlaylistPlayed, clear it
    if (lastPlaylistPlayed == playlistName) {
        lastPlaylistPlayed.clear();
        settings.remove("lastPlaylistPlayed");
    }

    return true;
}

bool LibraryManager::isTrackFavourite(Track* track)
{
    if (!track) return false;

    QString fileName = QFileInfo(track->filePath).fileName();

    QStringList favourites = getTracksFromPlaylist("Favourites");
    return favourites.contains(fileName, Qt::CaseInsensitive);
}
