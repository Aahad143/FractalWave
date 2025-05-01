#ifndef TRACK_H
#define TRACK_H

#include <QString>

struct Track
{
    QString filePath;
    QString title;    // e.g. QFileInfo(filePath).baseName()
};

#endif // TRACK_H
