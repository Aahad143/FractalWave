#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "helper/sidebarhelper.h"
#include <qevent.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , player(nullptr)
    , mediaController(nullptr)
{
    ui->setupUi(this);

    // setup the music player
    mediaController = new MediaController(this);

    // --------------------------------------------------------------------------------
    //   Re-initializing HomePage with custom constructors
    // --------------------------------------------------------------------------------

    // 1) extract the index of the placeholder
    int idx = ui->stackedWidget->indexOf(ui->homePage);

    HomePage *homePagePlaceholder = ui->homePage;
    // 2) kill off the default-constructed homepage
    ui->stackedWidget->removeWidget(homePagePlaceholder);
    delete homePagePlaceholder;

    // 3) initialization with custom parameters
    HomePage* customHomePage = new HomePage(ui->stackedWidget, mediaController);
    ui->stackedWidget->insertWidget(idx, customHomePage);

    // 4) if you want to keep the same pointer name:
    ui->homePage = customHomePage;

    // --------------------------------------------------------------------------------

    // --------------------------------------------------------------------------------
    //   Re-initializing SettingsPage with custom constructors
    // --------------------------------------------------------------------------------

    // 1) extract the index of the placeholder
    idx = ui->stackedWidget->indexOf(ui->settingsPage);

    SettingsPage *settingsPagePlaceholder = ui->settingsPage;
    // 2) kill off the default-constructed homepage
    ui->stackedWidget->removeWidget(settingsPagePlaceholder);
    delete settingsPagePlaceholder;

    // 3) initialization with custom parameters
    SettingsPage* customSettingsPage = new SettingsPage(ui->stackedWidget, mediaController);
    ui->stackedWidget->insertWidget(idx, customSettingsPage);

    // 4) if you want to keep the same pointer name:
    ui->settingsPage = customSettingsPage;

    // --------------------------------------------------------------------------------

    player = new Player(ui->centralwidget, mediaController);
    player->setVisible(false);

    // Start in maximized windowed mode
    showMaximized();

    // setupSidebar(ui->sidebarWidget, applyStyles(":/styles/styles/sidebar.qss"));
    setupSidebar2(ui->sidebarWidget);

    ui->stackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);  // Let it expand within available space
    ui->stackedWidget->setMinimumSize(500, 360); // Adjust this based on desired size

    // Connect item selection to page change
    connect(ui->sidebarWidget, &QListWidget::currentRowChanged, this, &MainWindow::changePage);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changePage(int index)
{
    ui->stackedWidget->setCurrentIndex(index);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (player) {
        QSize newSize = event->size();

        // Convert local coordinates to global screen coordinates
        // QPoint globalPos = this->mapToGlobal(QPoint(0, newSize.height() - player->height()));

        // Set the player's geometry using global coordinates
        player->setGeometry(0, newSize.height() - player->height(), newSize.width(), player->height());
    }
}

void MainWindow::moveEvent(QMoveEvent *event) {
    QMainWindow::moveEvent(event);
    if (player) {
        // QPoint globalPos = this->mapToGlobal(QPoint(0, this->height() - player->height()));
        // player->setGeometry(globalPos.x(), globalPos.y(), this->width(), player->height());
        player->setGeometry(0, this->height() - player->height(), this->width(), player->height());
    }
}
