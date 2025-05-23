#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QMenu>
#include <QAbstractItemView>
#include <qfileinfo.h>

#include "TrackItemDelegate.h"

TrackItemDelegate::TrackItemDelegate(QObject* parent, Context ctx, MediaController* externalMediaController) :
    QStyledItemDelegate(parent),
    context(ctx),
    mediaController(externalMediaController),
    playIcon(QIcon(":resources/icons/play-button-white.png")),
    pauseIcon(QIcon(":resources/icons/pause-button-white.png")),
    favOn  (QIcon(":resources/icons/heart-filled.png")),
    favOff (QIcon(":resources/icons/heart-empty.png")),
    moreIcon(QIcon(":resources/icons/overflow.png"))
{
}

static bool TrackItemDelegatetoggleSelect = false;

void TrackItemDelegate::paint(QPainter* painter,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const
{
    // Save painter state
    painter->save();

    // Draw hover / selection background
    QColor hover(  128, 128, 128,  30);
    if (option.state & QStyle::State_Selected)
    {
        painter->fillRect(option.rect, hover);
    }
    else if (option.state & QStyle::State_MouseOver)
    {
        // QColor hover = option.palette.highlight().color();
        painter->fillRect(option.rect, hover);
    }

    // Fetch data from the model
    Track* track = qvariant_cast<Track*>(index.data(Qt::UserRole));
    QString title  = track->title;
    QString artist = track->artist;
    bool    fav    = index.data(IsFavouriteRole ).toBool();
    bool isPlaying = index.data(IsPlayingRole).toBool();

    // 0) If this row is the currently playing track, give it a special highlight:
    QString thisPath = track->filePath;
    QString playing  = mediaController->getAudioPlayback()->getCurrentTrackPath();
    if (!playing.isEmpty() && QFileInfo(playing).canonicalFilePath() ==
                                  QFileInfo(thisPath).canonicalFilePath())
    {
        painter->fillRect(option.rect, hover);
    }
    else
    {
        isPlaying = false;
        painter->fillRect(option.rect, option.palette.base());
    }

    // Compute sub-rectangles
    QRect r = option.rect.adjusted(margin, margin, -margin, -margin);

    // play icon on the left
    QRect playRect(r.left() + margin, r.top() + (iconSize/2) -2, iconSize, iconSize);

    // overflow "⋮" icon on the right
    QRect moreRect(r.right() - (iconSize + margin/4), r.top() + (iconSize/2), iconSize, iconSize);
    // favourite icon on the right before overflow icon
    QRect heartRect(r.right() - (iconSize*2 + margin*2), r.top() + (iconSize/2), iconSize, iconSize);

    // text in the middle
    int textX = playRect.right() + margin*3;
    int textW = heartRect.left() - margin - textX;
    QRect titleRect(textX, r.top() + margin/2, textW, iconSize);
    QRect artistRect(textX, r.top() + (margin*2) + 4, textW, iconSize);

    // Draw the play icon
    (isPlaying ? pauseIcon : playIcon).paint(painter, playRect);

    // Draw the overflow icon
    moreIcon.paint(painter, moreRect);
    // Draw heart icon
    if (context != Favourites) {
        (fav ? favOn : favOff).paint(painter, heartRect);
    }

    // 7) Draw the text
    painter->setPen(option.palette.text().color());
    painter->drawText(titleRect, Qt::AlignVCenter | Qt::TextSingleLine, title);
    painter->setPen(option.palette.text().color());
    painter->drawText(artistRect, Qt::AlignVCenter | Qt::TextSingleLine, artist);

    // if this index is hovered OVER the more icon, draw a little circle behind it
    if (index == hoverOverflowIndex) {
        QColor hi(192, 192, 192); // RGB values for medium grey
        hi.setAlpha(60);
        painter->setBrush(hi);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(moreRect.adjusted(-4, -4, 4, 4));
    }

    // draw the overflow icon itself
    moreIcon.paint(painter, moreRect);

    // store it so editorEvent can reuse if needed
    // lastMoreRect = moreRect;

    // 8) Restore painter
    painter->restore();
}

bool TrackItemDelegate::editorEvent(QEvent* event,
                         QAbstractItemModel* model,
                         const QStyleOptionViewItem& option,
                         const QModelIndex& index)
{
    QRect r = option.rect.adjusted(margin, margin, -margin, -margin);  // same adjusted rect as in paint()
    QRect playRect(r.left() + margin, r.top() + (iconSize/2) -2, iconSize, iconSize);
    QRect moreRect(r.right() - (iconSize + margin/4), r.top() + (iconSize/2), iconSize, iconSize);
    QRect heartRect(r.right() - (iconSize*2 + margin*2), r.top() + (iconSize/2), iconSize, iconSize);

    auto *viewWidget = const_cast<QWidget*>(option.widget);
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(viewWidget);

    switch (event->type())
    {
    case QEvent::MouseMove: {
        auto *me = static_cast<QMouseEvent*>(event);
        QPoint pos = me->pos();

        // if over the play icon, show pointing‐hand, else arrow
        if (playRect.contains(pos)) {
            view->viewport()->setCursor(Qt::PointingHandCursor);
        } else {
            view->viewport()->setCursor(Qt::ArrowCursor);
        }

        bool overNow = moreRect.contains(me->pos());
        if (overNow && hoverOverflowIndex != index) {
            // hover entered a new index
            hoverOverflowIndex = index;
            if (view) view->viewport()->update(option.rect);
        }
        else if (!overNow && hoverOverflowIndex == index) {
            // hover left this index
            QModelIndex old = hoverOverflowIndex;
            hoverOverflowIndex = QModelIndex();
            if (view) view->viewport()->update(option.rect);
        }
        break;
    }

    // case QEvent::MouseButtonPress: {
    //     auto *me = static_cast<QMouseEvent*>(event);
    //     QPoint pos = me->pos();

    //     if (playRect.contains(pos))
    //     {
    //         // if (!mediaController->loadAndPlayTrack(index.row()))
    //         //     isPlaying = mediaController->isPlaying();
    //         // qDebug() << "Failed to load/play track at" << index.row();

    //         // 2) Emit and swallow
    //         emit playIconClicked(index.row());
    //         return true;
    //     }
    //     break;
    // }

    case QEvent::MouseButtonRelease: {
        auto *me = static_cast<QMouseEvent*>(event);
        QPoint pos = me->pos();              // position *inside* the item rectangle

        if (playRect.contains(pos))
        {
            // 3a) Only on playRect: select *this* row
            QItemSelectionModel* sel = view->selectionModel();
            sel->select(index,
                        QItemSelectionModel::ClearAndSelect |
                            QItemSelectionModel::Rows);

            // Emit and swallow
            emit playIconClicked(index.row());
            return true;
        }

        // If the click was on the overflow icon:
        if (moreRect.contains(pos))
        {
            // 1) Build the menu
            QMenu menu;
            Track* track = qvariant_cast<Track*>(index.data(Qt::UserRole));
            QString trackName = QFileInfo(track->filePath).fileName();
            qDebug() << "trackName" << trackName;
            menu.addAction("Add/Remove in Playlist", [=]{
                emit overflowActionRequested(index, OverflowCommand::AddToPlaylist, trackName);
            });
            menu.addAction("Delete", [=] {
                emit overflowActionRequested(index, OverflowCommand::Delete, trackName);
            });

            // 2) Map the icon’s top‐left to global coordinates
            QPoint globalPos = option.widget->mapToGlobal(moreRect.bottomRight());
            // 3) Show the menu
            menu.exec(globalPos);

            // We handled it; stop further processing
            return true;
        }

        if (heartRect.contains(pos)) {
            emit favouritesIconClicked(index);
            return true;
        }

        break;
    }

    default:
        break;
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
