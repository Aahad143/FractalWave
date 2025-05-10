#ifndef PLAYER_H
#define PLAYER_H

#include "audioplayback.h"
#include "mediacontroller.h"
#include <QFrame>
#include <QPushButton>
#include <QTimer>

namespace Ui {
class Player;
}

class Player : public QFrame
{
    Q_OBJECT

public:
    explicit Player(QWidget *parent = nullptr, MediaController *externalMediaController = nullptr);
    ~Player();

private slots:
    void checkAudioState();
    void onPrevButtonClicked();
    void onPlayButtonClicked();  // Slot for button press
    void onNextButtonClicked();
    void onSeekSliderReleased();
    // Slot: When the user starts dragging the slider.
    void onSeekSliderPressed();
    void updateSeekSlider();

private:
    Ui::Player *ui;

    QTimer *audioCheckTimer; // For continuously checking audio state

    QPushButton *play;
    QPushButton *previous;
    QPushButton *next;

    MediaController *mediaController;

    AudioPlayback *audioPlayback;

    QTimer *updateTimer;

    bool isSliderPressed;

    void setUpPlayer();
};

#endif // PLAYER_H
