#ifndef TRACKITEMDELEGATE_H
#define TRACKITEMDELEGATE_H

#include "mediacontroller.h"
#include <QStyledItemDelegate>

enum TrackRoles {
    TitleRole       = Qt::UserRole + 1,
    ArtistRole,
    IsFavoriteRole,
    IsPlayingRole
};

class TrackItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TrackItemDelegate(QObject* parent = nullptr);

    // paint the row
    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    // size of each row
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override
    {
        // fixed height, full available width
        return { option.rect.width(), 60 };
    }

    // handle clicks on fav/overflow
    bool editorEvent(QEvent* event,
                     QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

signals:
    void overflowActionRequested(const QModelIndex& index, const QString& action, const QString& trackPath);
    void playIconClicked(const int row);

private:
    QIcon playIcon, pauseIcon, favOn, favOff, moreIcon;
    mutable QModelIndex hoverOverflowIndex;
    // mutable QRect lastMoreRect;
    const int margin = 8;
    const int iconSize = 24;
};

#endif // TRACKITEMDELEGATE_H
