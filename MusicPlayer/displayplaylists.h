#ifndef DISPLAYPLAYLISTS_H
#define DISPLAYPLAYLISTS_H

#include <QFrame>
#include <qlistwidget.h>

namespace Ui {
class DisplayPlaylists;
}

class DisplayPlaylists : public QFrame
{
    Q_OBJECT

public:
    explicit DisplayPlaylists(QWidget *parent = nullptr);
    ~DisplayPlaylists();

    // Call this to populate the list
    void setPlaylists(const QStringList& names);

    void setListOfTrackNames(const QStringList& lOTN)
    {
        listOfTrackNames = lOTN;
    }

signals:
    void playlistSelected(const QString& playlistName);
    void closed();

private slots:
    void handleSelect();

private:
    Ui::DisplayPlaylists *ui;
    QListWidget* m_listWidget;

    void findPlaylistsContainingTrack(const QString& trackPath);
    void syncTrackWithSelectedPlaylists();

    QStringList alreadyInPlaylists;
    QStringList listOfTrackNames;
    QStringList selectedPlaylists;
};

#endif // DISPLAYPLAYLISTS_H
