#include <iostream>
#include <QGridLayout>
#include <QMessageBox>
#include "mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Main Window");
    
    btnPlay = new QPushButton("Play");
    connect(btnPlay, SIGNAL(clicked()), this, SLOT(onBtnPlayClicked()));

    btnStop = new QPushButton("Stop");
    connect(btnStop, SIGNAL(clicked()), this, SLOT(onBtnStopClicked()));

    btnRecord = new QPushButton("Record");
    connect(btnRecord, SIGNAL(clicked()), this, SLOT(onBtnRecordClicked()));
    btnRecord->setEnabled(false);

    QWidget* pnlControl = new QWidget();
    QGridLayout* lytControl = new QGridLayout(pnlControl);
    lytControl->addWidget(btnPlay,      0, 0, 1, 1, Qt::AlignCenter);
    lytControl->addWidget(btnStop,      1, 0, 1, 1, Qt::AlignCenter);
    lytControl->addWidget(btnRecord,    2, 0, 1, 1, Qt::AlignCenter);
    lytControl->addWidget(new QLabel(), 3, 0, 1, 1);
    lytControl->setRowStretch(3, 10);

    progress = new Progress(this);

    glWidget = new GLWidget();

    QWidget* pnlMain = new QWidget();
    QGridLayout* lytMain = new QGridLayout(pnlMain);
    lytMain->addWidget(glWidget,    0, 0, 4, 1);
    lytMain->addWidget(pnlControl,  0, 1, 5, 1);
    lytMain->addWidget(progress,    4, 0, 1, 1);
    lytMain->setColumnStretch(0, 10);
    lytMain->setRowStretch(0, 10);
    setCentralWidget(pnlMain);

    connect(this, SIGNAL(uiUpdate()), this, SLOT(updateUI()));
    connect(progress, SIGNAL(seek(float)), this, SLOT(seek(float)));
}

MainWindow::~MainWindow()
{

}

void MainWindow::setPlayButton()
{
    if (player) {
        if (player->isPaused()) {
            btnPlay->setText("Play");
        }
        else {
            btnPlay->setText("Pause");
        }
    }
    else {
        btnPlay->setText("Play");
    }
}

void MainWindow::setRecordButton()
{
    if (player) {
        if (player->isPiping()) {
            btnRecord->setText("=-=-=");
        }
        else {
            btnRecord->setText("Record");
        }
        btnRecord->setEnabled(true);
    }
    else {
        btnRecord->setText("Record");
        btnRecord->setEnabled(false);
    }
}

void MainWindow::onBtnPlayClicked()
{
    std::cout << "onBtnPlayClicked" << std::endl;
    if (player) {
        player->togglePaused();
    }
    else {
        player = new avio::Player();
        player->width = [&]() { return glWidget->width(); };
        player->height = [&]() { return glWidget->height(); };
        player->uri = uri;
#ifdef _WIN32        
        player->hWnd = glWidget->winId();
#else
        player->video_filter = "format=rgb24";
        player->renderCallback = [&](const avio::Frame& frame) { glWidget->renderCallback(frame); };
#endif
        player->progressCallback = [&](float arg) { progress->setProgress(arg); };
        player->cbMediaPlayingStarted = [&](int64_t duration) { mediaPlayingStarted(duration); };
        player->cbMediaPlayingStopped = [&]() { mediaPlayingStopped(); };
        player->start();

    }
    setPlayButton();
}

void MainWindow::onBtnStopClicked()
{
    std::cout << "stop" << std::endl;
    if (player) {
        player->running = false;
        if (player->isPaused()) player->togglePaused();
    }
    //setPlayButton();
    //setRecordButton();
}

void MainWindow::onBtnRecordClicked()
{
    std::cout << "record" << std::endl;
    //if (glWidget->player)
    //    glWidget->player->toggle_pipe_out("test.webm");
    setRecordButton();
}

void MainWindow::updateUI()
{
    setPlayButton();
    setRecordButton();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (player) {
        player->running = false;
        if (player->isPaused()) player->togglePaused();
    }
    while (player) std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void MainWindow::seek(float arg)
{
    std::cout << "seek: " << arg << std::endl;
    if (player) player->seek(arg);
}

void MainWindow::mediaPlayingStarted(qint64 duration)
{
    std::cout << "MainWindow::mediaPlayingStarted" << std::endl;
    progress->setDuration(duration);
    emit uiUpdate();
    //setPlayButton();
    //setRecordButton();
}

void MainWindow::mediaPlayingStopped()
{
    std::cout << "MainWindow::mediaPlayingStopped" << std::endl;
    progress->setProgress(0);
    emit uiUpdate();
    delete player;
    player = nullptr;
    glWidget->f.invalidate();
    glWidget->update();
    //setPlayButton();
    //setRecordButton();
}

void MainWindow::mediaProgress(float arg)
{
    progress->setProgress(arg);
}

void MainWindow::criticalError(const QString& msg)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Critical Error");
    msgBox.setText(msg);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();
    setPlayButton();
    setRecordButton();
}

void MainWindow::infoMessage(const QString& msg)
{
    std::cout << "message from mainwindow" << std::endl;
    std::cout << msg.toLatin1().data() << std::endl;
}