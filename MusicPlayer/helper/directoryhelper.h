#ifndef DIRECTORYHELPER_H
#define DIRECTORYHELPER_H

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>

// Returns the absolute path to `exeName` if found under <appDir>/<relativeDir>,
// otherwise returns an empty QString.
static QString findExecutableInAppDir(const QString& relativeDir,
                                      const QString& exeName)
{
    // 1. Compute the full path to the directory to search
    QString appDir   = QCoreApplication::applicationDirPath();
    qDebug() << "appDir:" << appDir;
    QString searchDirPath = QDir(appDir).filePath(relativeDir);
    qDebug() << "searchDirPath:" << searchDirPath;
    QDir    searchDir(searchDirPath);
    if (!searchDir.exists())
        return QString();

    // 2. First try a direct lookup
    QString direct = searchDir.filePath(exeName);
    if (QFile::exists(direct))
        return direct;

    // 3. Fallback: recurse through subdirectories
    QDirIterator it(searchDirPath,
                    QStringList{ exeName },
                    QDir::Files | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);
    if (it.hasNext())
        return it.next();

    // 4. Not found
    return QString();
}


#endif // DIRECTORYHELPER_H
