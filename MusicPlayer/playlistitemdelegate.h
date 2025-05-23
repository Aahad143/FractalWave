#ifndef PLAYLISTITEMDELEGATE_H
#define PLAYLISTITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QApplication>
#include <qevent.h>

class PlaylistItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit PlaylistItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    void paint(QPainter* painter,
                                     const QStyleOptionViewItem& option,
                                     const QModelIndex& index) const override
    {
        painter->save();

        // Prepare style option for background and selection
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        // Prevent the default text rendering
        opt.text.clear();

        // Draw only the background, focus, icon, etc.
        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter);

        QRect rect = option.rect;
        // Determine checkbox size
        QStyleOptionButton checkOpt;
        bool checked = index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
        checkOpt.state = QStyle::State_Enabled | (checked ? QStyle::State_On : QStyle::State_Off);
        QSize chkSize = QApplication::style()->sizeFromContents(
            QStyle::CT_CheckBox, &checkOpt, QSize(), nullptr);
        // Position checkbox at left with a small margin
        int margin = 4;
        QRect chkRect(rect.x() + margin,
                      rect.y() + (rect.height() - chkSize.height()) / 2,
                      chkSize.width(), chkSize.height());
        checkOpt.rect = chkRect;
        QApplication::style()->drawControl(QStyle::CE_CheckBox, &checkOpt, painter);

        // Draw text to the right of checkbox
        QString text = index.data(Qt::DisplayRole).toString();
        QRect textRect = QRect(chkRect.right() + margin,
                               rect.y(),
                               rect.width() - chkSize.width() - 3 * margin,
                               rect.height());
        painter->setFont(opt.font);
        painter->setPen(opt.palette.color(QPalette::Text));
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override
    {
        // Base height on font metrics and checkbox height
        QFontMetrics fm(option.font);
        int textHeight = fm.height();
        QStyleOptionButton checkOpt;
        QSize chkSize = QApplication::style()->sizeFromContents(
            QStyle::CT_CheckBox, &checkOpt, QSize(), nullptr);
        int h = qMax(textHeight, chkSize.height()) + 8; // add vertical padding
        // Width determined by view, so return minimum
        return QSize(0, h);
    }

    bool editorEvent(QEvent* event, QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override
    {
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            QRect rect = option.rect;
            QStyleOptionButton checkOpt;
            QSize chkSize = QApplication::style()->sizeFromContents(
                QStyle::CT_CheckBox, &checkOpt, QSize(), nullptr);
            int margin = 4;
            QRect chkArea(rect.x() + margin,
                          rect.y() + (rect.height() - chkSize.height()) / 2,
                          chkSize.width(), chkSize.height());
            if (chkArea.contains(me->pos())) {
                // Toggle state
                Qt::CheckState newState =
                    index.data(Qt::CheckStateRole).toInt() == Qt::Checked
                        ? Qt::Unchecked : Qt::Checked;
                model->setData(index, newState, Qt::CheckStateRole);
                return true;
            }
        }
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }
};
#endif // PLAYLISTITEMDELEGATE_H
