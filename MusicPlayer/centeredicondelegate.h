#ifndef CENTEREDICONDELEGATE_H
#define CENTEREDICONDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyleOptionViewItem>

class CenteredIcon2Delegate : public QStyledItemDelegate
{
public:
    CenteredIcon2Delegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();

        // draw hover/selection background
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, QColor("#3a3a3a"));
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(option.rect, QColor("#404040"));
        }

        // fetch and draw the icon centered
        auto icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        if (!icon.isNull()) {
            // compute a square in the middle for our icon
            QRect iconRect;
            iconRect.setSize({iconSize, iconSize});
            iconRect.moveCenter(option.rect.center());
            icon.paint(painter, iconRect, Qt::AlignCenter);
        }

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        Q_UNUSED(index);
        // Width=0 means "use the view/item's own widthHint"
        // Height is just enough to fit icon+padding.
        return { -1, iconSize + padding*2 };
    }

private:
    static constexpr int iconSize = 24;
    static constexpr int padding  = 15;
};


#endif // CENTEREDICONDELEGATE_H
