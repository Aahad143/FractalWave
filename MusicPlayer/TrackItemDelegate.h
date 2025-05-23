#ifndef TRACKITEMDELEGATE_H
#define TRACKITEMDELEGATE_H

#include "mediacontroller.h"
#include <QStyledItemDelegate>

enum TrackRoles {
    IsFavouriteRole       = Qt::UserRole + 1,
    IsPlayingRole,
    PlaylistRole
};

// 1) Define your enum (register it if you want QMetaEnum support)
enum OverflowCommand {
    AddToPlaylist,
    Delete
};

enum Context{
    Default,
    Favourites
};

class TrackItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TrackItemDelegate(QObject* parent = nullptr, Context ctx = Default, MediaController* controller = nullptr);

    // paint the row
    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    // size of each row
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override
    {
        // fixed height, full available width
        return { 0, 60 };
    }

    // handle clicks on fav/overflow
    bool editorEvent(QEvent* event,
                     QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

signals:
    void overflowActionRequested(const QModelIndex& index, const OverflowCommand action, const QString& trackPath);
    void playIconClicked(const int row);
    void favouritesIconClicked(const QModelIndex& index);

private:
    QIcon playIcon, pauseIcon, favOn, favOff, moreIcon;
    mutable QModelIndex hoverOverflowIndex;
    // mutable QRect lastMoreRect;
    const int margin = 8;
    const int iconSize = 24;

    static bool toggleSelect;

    Context context;
    MediaController* mediaController;
};

#endif // TRACKITEMDELEGATE_H
