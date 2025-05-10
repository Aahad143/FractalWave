#include <QMessageBox>

#include "addcontentform.h"
#include "ui_addcontentform.h"
#include "librarymanager.h"

AddContentForm::AddContentForm(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::AddContentForm)
{
    ui->setupUi(this);

    resetForm();

    connect(ui->listWidget, &QListWidget::itemClicked, this, &AddContentForm::onListItemClicked);
    connect(ui->backBtn, &QPushButton::clicked, this, &AddContentForm::resetForm);
    connect(this, &AddContentForm::createPlaylist, &LibraryManager::instance(), &LibraryManager::createAndSavePlaylist);
    connect(ui->cancelBtn, &QPushButton::clicked, this, [=](){
        this->hide();
        this->resetForm();
    });

    connect(ui->createPlaylistBtn, &QPushButton::clicked, this, [=](){
        QString playlistName = ui->playlistLineEdit->text();
        if (playlistName.isEmpty()) {
            QMessageBox::warning(this, tr("Name Required"),
                                 tr("Please enter a playlist name."));
            return;
        }

        bool ok = LibraryManager::instance().createAndSavePlaylist(playlistName);
        if (!ok) {
            QMessageBox::warning(this, tr("Error"),
                                 tr("A playlist named “%1” already exists.").arg(playlistName));
        }
        else {
            QMessageBox::information(this, tr("Created"),
                                     tr("Playlist “%1” was created.").arg(playlistName));
            this->hide();
            this->resetForm();
        }
    });

    connect(ui->addTrackBtn, &QPushButton::clicked, this, [this]() {
        QString url = ui->trackLineEdit->text().trimmed();
        if (url.isEmpty()) {
            QMessageBox::warning(this, tr("URL Required"),
                                 tr("Please enter a track URL."));
            return;
        }
        bool ok = LibraryManager::instance().addTrackToDIr(url);
        if (!ok) {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Failed to add track. It may already exist or be invalid."));
        } else {
            QMessageBox::information(this, tr("Added"),
                                     tr("Track was added successfully."));
            this->hide();
            this->resetForm();
        }
    });
}

AddContentForm::~AddContentForm()
{
    delete ui;
}

void AddContentForm::hideEvent(QHideEvent *event)
{
    QFrame::hideEvent(event);
    emit closed();
}

void AddContentForm::onListItemClicked(QListWidgetItem *item){
    // Option A: compare pointers
    ui->listWidget->setVisible(false);
    ui->addValueEdit->setVisible(true);

    // disconnect old connections so we don’t double-fire
    // ui->createPlaylistBtn->disconnect();
    // ui->addTrackBtn    ->disconnect();

    if (item == ui->listWidget->item(0))
    {
        ui->trackLineEdit->setVisible(false);
        ui->playlistLineEdit->setVisible(true);
        ui->createPlaylistBtn->setVisible(true);
        ui->label->setText("Add New Playlist");
    }
    else if (item == ui->listWidget->item(1))
    {
        ui->playlistLineEdit->setVisible(false);
        ui->trackLineEdit->setVisible(true);
        ui->addTrackBtn->setVisible(true);
        ui->label->setText("Add New Track");

    }
}

void AddContentForm::resetForm()
{
    ui->listWidget->setVisible(true);
    ui->addValueEdit->setVisible(false);
    ui->createPlaylistBtn->setVisible(false);
    ui->addTrackBtn->setVisible(false);
    ui->listWidget->clearSelection();

    ui->playlistLineEdit->setText("");
    ui->trackLineEdit->setText("");

    ui->label->setText("Add New...");
}
