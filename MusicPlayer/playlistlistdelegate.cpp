#include "PlaylistListDelegate.h"
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include <QMenu>
#include <QAbstractItemView>

PlaylistListDelegate::PlaylistListDelegate(QObject* parent)
    : QStyledItemDelegate(parent),
    overflowIcon(QIcon(":resources/icons/overflow.png"))
{}

void PlaylistListDelegate::paint(QPainter* painter,
                                 const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    painter->save();

    const QString name = index.data(Qt::DisplayRole).toString();

    // 1) draw selection/hover background
    QColor bg(128,128,128,30);
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, bg);
    else if (option.state & QStyle::State_MouseOver)
        painter->fillRect(option.rect, bg);

    // 2) text
    QString text = index.data(Qt::DisplayRole).toString();
    QRect textRect = QRect(
        option.rect.x() + margin,
        option.rect.y(),
        option.rect.width() - iconSize - 3*margin,
        option.rect.height()
        );
    painter->setPen(option.palette.text().color());
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);

    if (name != QLatin1String("All Songs"))
    {
        // 3) overflow icon
        QRect iconRect(
            option.rect.right() - iconSize - margin,
            option.rect.y() + (option.rect.height() - iconSize)/2,
            iconSize, iconSize
            );

        // 4) hover highlight
        if (hoverIndex == index) {
            QColor circle(192,192,192,60);
            painter->setBrush(circle);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(iconRect.adjusted(-4,-4,4,4));
        }

        overflowIcon.paint(painter, iconRect);
    }

    painter->restore();
}

QSize PlaylistListDelegate::sizeHint(const QStyleOptionViewItem& option,
                                     const QModelIndex&) const
{
    // same height as TrackItemDelegate
    return { 0, 60 };
}

bool PlaylistListDelegate::editorEvent(QEvent* event,
                                       QAbstractItemModel* model,
                                       const QStyleOptionViewItem& option,
                                       const QModelIndex& index)
{
    const QString name = index.data(Qt::DisplayRole).toString();

    // compute overflow‐icon geometry
    QRect iconRect(
        option.rect.right() - iconSize - margin,
        option.rect.y() + (option.rect.height() - iconSize)/2,
        iconSize, iconSize
        );

    // need the view to change cursor & update
    auto *view = qobject_cast<QAbstractItemView*>(
        const_cast<QWidget*>(option.widget)
        );

    QRect fullRect = view
                         ? view->visualRect(index)
                         : option.rect;

    switch (event->type()) {
    case QEvent::MouseMove: {
        auto *me = static_cast<QMouseEvent*>(event);
        bool over = iconRect.contains(me->pos());
        // update hoverIndex & repaint that row
        if (over && hoverIndex != index) {
            hoverIndex = index;
            if (view) view->viewport()->update(option.rect);
        }
        else if (!over && hoverIndex == index) {
            hoverIndex = QModelIndex();
            if (view) view->viewport()->update(option.rect);
        }
        // change cursor when over icon
        if (view) {
            view->viewport()->setCursor(
                over ? Qt::PointingHandCursor : Qt::ArrowCursor
                );
        }
        break;
    }

    case QEvent::MouseButtonRelease: {
        auto *me = static_cast<QMouseEvent*>(event);
        if(name != QLatin1String("All Songs"))
        {
            if (iconRect.contains(me->pos())) {
                // show your overflow menu
                QMenu menu;
                QString name = index.data(Qt::DisplayRole).toString();
                menu.addAction("Rename",   [=]{ emit overflowRequested(index, PlaylistRename, name); });
                menu.addAction("Delete",   [=]{ emit overflowRequested(index, PlaylistDelete, name); });
                QPoint global = option.widget->mapToGlobal(iconRect.bottomRight());
                menu.exec(global);
                return true; // consumed
            }
            else
            {
                emit playlistItemClicked(index);
            }
        }
        else
        {
            emit playlistItemClicked(index);
        }
        break;
    }

    default:
        break;
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
