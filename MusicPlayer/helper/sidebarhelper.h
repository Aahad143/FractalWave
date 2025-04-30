#ifndef SIDEBARHELPER_H
#define SIDEBARHELPER_H

#include "CenteredItemDelegate.h"
#include <QListWidget>

void setupSidebar(QListWidget *sidebarWidget, QString styleSheet);
void setupSidebar2(QListWidget *sidebarWidget, int margin);
void adjustSidebarWidth(QListWidget *sidebarWidget, int margin);

void setupSidebar(QListWidget *sidebarWidget, QString styleSheet) {
    sidebarWidget->setViewMode(QListView::IconMode);
    sidebarWidget->setIconSize(QSize(24, 24));
    sidebarWidget->setMovement(QListView::Static);
    sidebarWidget->setFixedWidth(56);
    sidebarWidget->setFlow(QListView::TopToBottom);
    sidebarWidget->setContentsMargins(0, 0, 0, 0);

    // Apply stylesheet for sidebar design
    sidebarWidget->setStyleSheet(styleSheet);

    // Set selection mode (single selection)
    sidebarWidget->setSelectionMode(QAbstractItemView::SingleSelection);
}

void setupSidebar2(QListWidget *sidebarWidget) {
    int margin = 0;

    // Set a custom delegate to center icons
    sidebarWidget->setItemDelegate(new CenteredIconDelegate(sidebarWidget));
    sidebarWidget->setViewMode(QListView::IconMode);
    sidebarWidget->setSpacing(margin); // Adjust the value as needed for your desired margin
    adjustSidebarWidth(sidebarWidget, margin);
}

void adjustSidebarWidth(QListWidget *sidebarWidget, int margin) {
    // Get the item delegate to calculate the correct size
    QAbstractItemDelegate *delegate = sidebarWidget->itemDelegate();
    int maxWidth = 0;

    QListWidgetItem *item = sidebarWidget->item(0);
    // Get the model index for this item
    QModelIndex index = sidebarWidget->model()->index(0, 0);

    // Use the delegate's sizeHint to get the proper size
    QSize itemSize = delegate->sizeHint(
        QStyleOptionViewItem(),
        index
        );

    qDebug() << "Item size:" << itemSize;
    maxWidth = itemSize.width();

    // Add some padding for comfort (adjust as needed)
    maxWidth += margin*2 + 4;

    qDebug() << "Setting sidebar width to:" << maxWidth;
    sidebarWidget->setFixedWidth(maxWidth);
}


#endif // SIDEBARHELPER_H
