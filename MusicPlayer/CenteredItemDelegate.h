#ifndef CENTEREDICONDELEGATE_H
#define CENTEREDICONDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>

class CenteredIconDelegate : public QStyledItemDelegate
{
public:
    CenteredIconDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        painter->save();

        // Get the icon
        QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();

        // Enable antialiasing for smoother rounded corners
        painter->setRenderHint(QPainter::Antialiasing);

        // Create rounded rectangle path
        QPainterPath path;
        int radius = 8; // Adjust the radius to your preference
        path.addRoundedRect(option.rect, radius, radius);

        // Set background color based on hover or selection
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, QColor("#3a3a3a")); // Selected color
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(option.rect, QColor("#404040")); // Hover color
        }

        // Icon size and margins
        QRect iconRect(option.rect.x() + padding, option.rect.y() + padding, iconSize, iconSize);

        // Draw the icon
        icon.paint(painter, iconRect, Qt::AlignCenter);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        Q_UNUSED(option);
        Q_UNUSED(index);
        return QSize(iconSize + (padding*2), iconSize + (padding*2)); // Adjust size of items (width, height)
    }

private:
    int iconSize = 24;
    int padding = 15;
};

#endif // CENTEREDICONDELEGATE_H
