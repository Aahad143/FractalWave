#ifndef DELEGATEHELPER_H
#define DELEGATEHELPER_H

#include "mediacontroller.h"
#include "TrackItemDelegate.h"

void toggleDelegatePlayIcon(MediaController *mediaController, QListWidget *llistWidget, bool isPlaying)
{
    qDebug() << "isPlaying in toggleDelegatePlayIcon():" << isPlaying;
    int  current = mediaController->getCurrentTracklistManager()->getCurrentIndex();

    // 3) Update EVERY item’s role, so only the "current" row is marked
    for (int r = 0; r < llistWidget->count(); ++r) {
        llistWidget->item(r)
        ->setData(IsPlayingRole, (r == current) && isPlaying);
    }
    // 4) Tell the view to repaint all visible rows (or just the affected ones)
    //    A) repaint the whole list:
    llistWidget->viewport()->update();
}

#endif // DELEGATEHELPER_H
