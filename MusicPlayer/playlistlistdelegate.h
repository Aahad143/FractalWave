#ifndef PLAYLISTLISTDELEGATE_H
#define PLAYLISTLISTDELEGATE_H

#include <QStyledItemDelegate>
#include <QIcon>
#include <QModelIndex>

enum OverflowAction {
    PlaylistRename,
    PlaylistDelete
};

class PlaylistListDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit PlaylistListDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

    bool editorEvent(QEvent* event,
                     QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

signals:
    void playlistItemClicked(const QModelIndex& index);
    /// action is a string like "rename", "delete", etc.
    void overflowRequested(const QModelIndex& index,
                           const OverflowAction action,
                           const QString& playlistName);

private:
    QIcon overflowIcon;
    mutable QModelIndex hoverIndex;
    const int margin = 8;
    const int iconSize = 24;
};

#endif // PLAYLISTLISTDELEGATE_H
