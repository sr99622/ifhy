#include <iostream>
#include <QGridLayout>
#include <QMessageBox>
//#include <QThread>
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
    connect(glWidget, SIGNAL(mediaPlayingStarted(qint64)), this, SLOT(mediaPlayingStarted(qint64)));
    connect(glWidget, SIGNAL(mediaPlayingStopped()), this, SLOT(mediaPlayingStopped()));
    connect(glWidget, SIGNAL(mediaProgress(float)), this, SLOT(mediaProgress(float)));
    connect(glWidget, SIGNAL(criticalError(const QString&)), this, SLOT(criticalError(const QString&)));
    connect(glWidget, SIGNAL(infoMessage(const QString&)), this, SLOT(infoMessage(const QString&)));
    connect(progress, SIGNAL(seek(float)), glWidget, SLOT(seek(float)));

    QWidget* pnlMain = new QWidget();
    QGridLayout* lytMain = new QGridLayout(pnlMain);
    lytMain->addWidget(btnPlay,     0, 1, 1, 1, Qt::AlignCenter);
    lytMain->addWidget(btnStop,     1, 1, 1, 1, Qt::AlignCenter);
    lytMain->addWidget(btnRecord,   2, 1, 1, 1, Qt::AlignCenter);
    lytMain->addWidget(glWidget,    0, 0, 4, 1);
    lytMain->addWidget(progress,    4, 0, 1, 1);
    lytMain->setColumnStretch(0, 10);
    setCentralWidget(pnlMain);
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
        std::cout << "toggle player" << std::endl;
        player->togglePaused();
    }
    else {
        std::cout << "play" << std::endl;

        std::function<void(const avio::Frame& frame)> renderCallback = [&](const avio::Frame& frame)
        {
            std::cout << "renderCallback start" << std::endl;
            if (!frame.isValid()) {
                glWidget->emit infoMessage("render callback recvd invalid Frame");
                return;
            }
            glWidget->mutex.lock();
            glWidget->f = frame;
            glWidget->img = QImage(glWidget->f.m_frame->data[0], glWidget->f.m_frame->width,
                                glWidget->f.m_frame->height, QImage::Format_RGB888);
            glWidget->mutex.unlock();
            glWidget->update();
            std::cout << "renderCallback finish" << std::endl;
        };

        std::function<void(float)> progressCallback = [&](float arg) 
        {
            progress->setProgress(arg);
        };

        //glWidget->play(uri);
        player = new avio::Player();
        player->width = [&]() { return glWidget->width(); };
        player->height = [&]() { return glWidget->height(); };
        player->uri = uri;
        player->hWnd = glWidget->winId();
        //player->renderCallback = renderCallback;
        player->progressCallback = [&](float arg) { progress->setProgress(arg); };
        player->cbMediaPlayingStarted = [&](int64_t duration) { mediaPlayingStarted(duration); };
        player->cbMediaPlayingStopped = [&]() { mediaPlayingStopped(); };
        player->start();
        //while (player->running) std::this_thread::sleep_for(std::chrono::milliseconds(1));

    }
    setPlayButton();
}

void MainWindow::onBtnStopClicked()
{
    std::cout << "stop" << std::endl;
    player->running = false;
    setPlayButton();
    setRecordButton();
}

void MainWindow::onBtnRecordClicked()
{
    std::cout << "record" << std::endl;
    if (glWidget->player)
        glWidget->player->toggle_pipe_out("test.webm");
    setRecordButton();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    glWidget->stop();
}

void MainWindow::mediaPlayingStarted(qint64 duration)
{
    std::cout << "MainWindow::mediaPlayingStarted" << std::endl;
    progress->setDuration(duration);
    setPlayButton();
    setRecordButton();
}

void MainWindow::mediaPlayingStopped()
{
    std::cout << "MainWindow::mediaPlayingStopped" << std::endl;
    progress->setProgress(0);
    player = nullptr;
    setPlayButton();
    setRecordButton();
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