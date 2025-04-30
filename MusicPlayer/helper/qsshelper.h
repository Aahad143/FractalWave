#ifndef QSSHELPER_H
#define QSSHELPER_H

#include <QString>
#include <QDebug>
#include <QFile>
#include <QTextStream>

inline QString applyStyles(const char *path);

inline QString applyStyles(const char *path)
{
    // Open the QSS file
    QFile file(path);  // Assuming it's in the Qt resource file or provide the full path
    if (file.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream stream(&file);
        QString styleSheet = stream.readAll();  // Read the entire file
        return styleSheet;
    }
    qDebug() << "Invalid resource path";
    return "";
}

#endif // QSSHELPER_H
