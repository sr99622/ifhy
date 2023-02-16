#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QSlider>

#include "avio.h"
#include "glwidget.h"
#include "progress.h"

class Label : public QLabel
{
    Q_OBJECT

    QSize sizeHint() const{
        return QSize(640, 480);
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void setPlayButton();
    void setRecordButton();

    QPushButton* btnPlay;
    QPushButton* btnStop;
    QPushButton* btnRecord;
    Progress* progress;
    GLWidget* glWidget;
    avio::Player* player = nullptr;
    bool playing = false;
    const char* uri;

protected:
    void closeEvent(QCloseEvent* event) override;

signals:
    void uiUpdate();

public slots:
    void onBtnPlayClicked();
    void onBtnStopClicked();
    void onBtnRecordClicked();
    void seek(float);
    void mediaPlayingStarted(qint64);
    void mediaPlayingStopped();
    void mediaProgress(float);
    void criticalError(const QString&);
    void infoMessage(const QString&);
    void updateUI();

};


#endif // MAINWINDOW_H