#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "helper/sidebarhelper.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mediaController(nullptr)
{
    ui->setupUi(this);

    // setup the music player
    mediaController = new MediaController(this);

    // 1) figure out where the placeholder lives
    int idx = ui->stackedWidget->indexOf(ui->homePage);

    HomePage *placeholder = ui->homePage;
    // 2) kill off the default-constructed one
    ui->stackedWidget->removeWidget(placeholder);
    delete placeholder;

    // 3) create your own with parameters, and put it back in the same slot
    HomePage* customPage = new HomePage(ui->stackedWidget, mediaController);
    ui->stackedWidget->insertWidget(idx, customPage);

    // 4) if you want to keep the same pointer name:
    ui->homePage = customPage;

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

    // Handle Unity embedding only when switching to Visualizer
    if (index == 1) {
        // unityPage->embedUnity();
        // unityPage->resizeWindow();
    }
}
