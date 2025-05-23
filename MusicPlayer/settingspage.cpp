#include <QFileDialog>
#include <QSettings>


#include "settingspage.h"
#include "librarymanager.h"
#include "ui_settingspage.h"

SettingsPage::SettingsPage(QWidget *parent, MediaController *externalMediaController) :
    QWidget(parent),
    ui(new Ui::SettingsPage),
    mediaController(externalMediaController)
{
    ui->setupUi(this);

    // connect(ui->selectDir, &QPushButton::clicked, this, &SettingsPage::onBrowseMusicFolder);
    // initialize button text with the current folder
    if (mediaController) {
        const QString cur = mediaController->currentMusicFolder();
        ui->selectDir->setText(cur.isEmpty() ? tr("Choose Music Folder…") : cur);
        ui->currentDir->setText("Current Dir: "+ LibraryManager::instance().libraryManager().getCurrentMusicDirectory());
    }

}

void SettingsPage::on_selectDir_clicked()
{
    // start browsing from the last‐saved folder
    const QString start = mediaController->currentMusicFolder().isEmpty()
                              ? QDir::homePath()
                              : mediaController->currentMusicFolder();

    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Select Music Folder"),
        start,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );
    if (dir.isEmpty())
        return;

    // 1) Save into your MediaController
    mediaController->setCurrentMusicFolder(dir);

    // 2) Persist into QSettings
    QSettings settings("FractalWave", "FractalWave");
    settings.setValue("musicFolder", dir);

    QString dir2 = settings.value("musicFolder", "").toString();
    qDebug() << "musicDir in QSettings:" << dir2;

    // 3) (Optional) update the button label so the user sees it changed
    ui->selectDir->setText(dir);

    LibraryManager::instance().scanDirectory();
}

SettingsPage::~SettingsPage()
{
    delete ui;
}
