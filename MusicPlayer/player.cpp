#include "player.h"
#include "ui_player.h"
#include "helper/qsshelper.h"

Player::Player(QWidget *parent, MediaController *externalMediaController)
    : QFrame(parent)
    , ui(new Ui::Player)
    , audioPlayback(nullptr)
    , mediaController(externalMediaController)
    , play(nullptr)
    , previous(nullptr)
    , next(nullptr)
    , isSliderPressed(false)
{
    ui->setupUi(this);

    // Timer to continuously check if audio is loaded
    audioPlayback = mediaController->getAudioPlayback();
    audioCheckTimer = new QTimer(this);
    connect(audioCheckTimer, &QTimer::timeout, this, &Player::checkAudioState);
    audioCheckTimer->start(100);  // Check every 100 ms

    setUpPlayer();
}

Player::~Player()
{
    delete ui;
}

void Player::setUpPlayer(){
    // In your player initialization
    this->setObjectName("player");
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // Change to Fixed height
    this->setFixedHeight(100); // Set a fixed height instead of using geometry
    this->setFrameShape(QFrame::StyledPanel);

    // Settings that make this frame render at the top globally
    // this->setAttribute(Qt::WA_TranslucentBackground, false); // Ensure solid background
    // this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);

    // this->setAttribute(Qt::WA_ShowWithoutActivating);
    this->raise();  // Bring the frame to the front

    // Apply a style
    this->setStyleSheet(applyStyles(":/resources/styles/player.qss"));
    previous = ui->previousBtn;
    play = ui->playBtn;
    next = ui->nextBtn;

    previous->setCursor(Qt::PointingHandCursor);
    play->setCursor(Qt::PointingHandCursor);
    next->setCursor(Qt::PointingHandCursor);

    connect(previous, &QPushButton::clicked, this, &Player::onPrevButtonClicked);
    connect(play, &QPushButton::clicked, this, &Player::onPlayButtonClicked);
    connect(next, &QPushButton::clicked, this, &Player::onNextButtonClicked);


    // In your constructor (after ui->setupUi(this)):
    connect(ui->seekSlider, &QSlider::sliderPressed, this, &Player::onSeekSliderPressed);
    connect(ui->seekSlider, &QSlider::sliderReleased, this, &Player::onSeekSliderReleased);

    // In your Player class, declare a QTimer pointer (e.g., updateTimer) as a member.
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &Player::updateSeekSlider);
    updateTimer->start(0); // update every 200 ms
    ui->seekSlider->setMaximum(10000); // Increase resolution


    qDebug() << "player frame x position:" << this->pos().rx();
    qDebug() << "player frame y position:" << this->pos().ry();
}

void Player::checkAudioState()
{
    if (audioPlayback && audioPlayback->hasAudioLoaded())
    {
        // Show the QFrame when audio is loaded
        this->setVisible(true);
    }
    else
    {
        // Hide the QFrame when no audio is loaded
        this->setVisible(false);
    }
}

void Player::onPrevButtonClicked()
{
    if (!audioPlayback) {
        qDebug() << "No track loaded!";
        return;
    }

    mediaController->previousTrack();
}

void Player::onPlayButtonClicked()
{
    if (!audioPlayback) {
        qDebug() << "No track loaded!";
        return;
    }

    audioPlayback->togglePause();  // Pause if playing
}

void Player::onNextButtonClicked()
{
    if (!audioPlayback) {
        qDebug() << "No track loaded!";
        return;
    }

    mediaController->nextTrack();
}

// Slot: When the user starts dragging the slider.
void Player::onSeekSliderPressed()
{
    // Optionally, stop the timer:
    isSliderPressed = true;
}

void Player::onSeekSliderReleased()
{
    // Get the current slider value (assume the slider's range is 0 to 1000)
    int sliderValue = ui->seekSlider->value();
    int sliderMax = ui->seekSlider->maximum();

    // Calculate the new position in seconds.
    double trackLength = audioPlayback->getTrackLength();
    double newPosition = (sliderValue / static_cast<double>(sliderMax)) * trackLength;

    // Set the new position.
    if (isSliderPressed){
        audioPlayback->seek(newPosition);
        isSliderPressed = false;
    }

}

void Player::updateSeekSlider()
{
    if (!audioPlayback->isPlaying())
        return;

    double currentPos = audioPlayback->getCurrentPosition();
    double trackLength = audioPlayback->getTrackLength();
    if (trackLength > 0)
    {
        int sliderMax = ui->seekSlider->maximum();
        int newValue = static_cast<int>((currentPos / trackLength) * sliderMax);

        if (isSliderPressed)
        {
            if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
                isSliderPressed = false;
            }
        }
        else
        {
            ui->seekSlider->setValue(newValue);
        }
    }
}
