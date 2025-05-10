#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "track.h"
#include <QString>
#include <qdebug.h>

class Playlist
{
public:
    Playlist(const QString& playlistName);

    void addTrack(const Track& t)                               { tracks.push_back(t); }
    const std::vector<Track>& getTracks() const     { return tracks; }
    void clear()                                                                 {
        qDebug() <<"clearing tracks in Playlist:" << name;
        tracks.clear();
    }

    QString getPlaylistName() const                             { return name; }
    void setName(const QString& name_)                  { name = name_; }

    Track at(const int& index) const
    {
        return tracks.at(index);
    }

    const int size() const
    {
        return tracks.size();
    }

private:
    QString name;
    std::vector<Track> tracks;
};

#endif // PLAYLIST_H
