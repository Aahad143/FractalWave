// Track.h
#ifndef TRACK_H
#define TRACK_H

#include <QString>
#include <QMetaType>

struct Track {
    QString filePath;
    QString title;
    QString artist;
    QString album;
    QString  coverImage;  // extracted from tags, if available
};

Q_DECLARE_METATYPE(Track*)

#endif // TRACK_H
