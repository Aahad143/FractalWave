#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QWidget>
#include <QDir>

#include "mediacontroller.h"

namespace Ui {
class SettingsPage;
}

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr, MediaController *externalMediaController = nullptr);
    ~SettingsPage();

    // void onBrowseMusicFolder();

private slots:
    void on_selectDir_clicked();

private:
    Ui::SettingsPage *ui;
    MediaController *mediaController;
    QDir currentMusicFolder;
};

#endif // SETTINGSPAGE_H
