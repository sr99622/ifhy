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

    progress = new Progress(this);

    glWidget = new GLWidget();

    QWidget* pnlMain = new QWidget();
    QGridLayout* lytMain = new QGridLayout(pnlMain);
    lytMain->addWidget(btnPlay,     0, 1, 1, 1, Qt::AlignCenter);
    lytMain->addWidget(btnStop,     1, 1, 1, 1, Qt::AlignCenter);
    lytMain->addWidget(btnRecord,   2, 1, 1, 1, Qt::AlignCenter);
    lytMain->addWidget(glWidget,    0, 0, 4, 1);
    lytMain->addWidget(progress,    4, 0, 1, 1);
    lytMain->setColumnStretch(0, 10);
    setCentralWidget(pnlMain);

    connect(this, SIGNAL(uiUpdate()), this, SLOT(updateUI()));
}

MainWindow::~MainWindow()
{

}

void MainWindow::setPlayButton()
{
    if (player.running) {
        if (player.isPaused()) {
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
    if (player.running) {
        if (player.isPiping()) {
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
    if (player.running) {
        player.togglePaused();
    }
    else {
        player.width = [&]() { return glWidget->width(); };
        player.height = [&]() { return glWidget->height(); };
        player.uri = uri;
        player.video_filter = "format=rgb24";
        //player.hWnd = glWidget->winId();
        player.renderCallback = [&](const avio::Frame& frame) { glWidget->renderCallback(frame); };
        player.progressCallback = [&](float arg) { progress->setProgress(arg); };
        player.cbMediaPlayingStarted = [&](int64_t duration) { mediaPlayingStarted(duration); };
        player.cbMediaPlayingStopped = [&]() { mediaPlayingStopped(); };
        player.start();

    }
    setPlayButton();
}

void MainWindow::onBtnStopClicked()
{
    std::cout << "stop" << std::endl;
    player.running = false;
    glWidget->invalidateFrame();
    setPlayButton();
    setRecordButton();
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
    player.running = false;
    while (player.display) std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
    //player = nullptr;
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