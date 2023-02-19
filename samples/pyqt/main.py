import os
import sys
import numpy as np
import cv2
from time import sleep
from PyQt6.QtWidgets import QApplication, QMainWindow, QPushButton, \
QGridLayout, QWidget, QSlider, QLabel, QMessageBox
from PyQt6.QtCore import Qt, pyqtSignal, QObject, QPoint
from PyQt6.QtGui import QPainter
from glwidget import GLWidget

sys.path.append("../../build")
import avio

class Slider(QSlider):
    def __init__(self, o, w):
        super().__init__(o)
        self.setMouseTracking(True)
        self.w = w

    def leaveEvent(self, e):
        self.w.lblPosition.setText("", 0)

    def mousePressEvent(self, e):
        pct = e.position().x() / self.width()
        self.w.player.seek(pct)

    def mouseMoveEvent(self, e):
        if (self.w.playing):
            x = e.position().x()
            pct = x / self.width()
            position = pct * self.w.duration
            self.w.lblPosition.setText(self.w.timestring(int(position)), x)

class Position(QLabel):
    def __init__(self):
        super().__init__()
        self.pos = 0

    def setText(self, s, n):
        self.pos = n
        super().setText(s)

    def paintEvent(self, e):
        painter = QPainter(self)
        rect = self.fontMetrics().boundingRect(self.text())
        x = min(self.width() - rect.width(), self.pos)
        painter.drawText(QPoint(int(x), self.height()), self.text())

class Signals(QObject):
    error = pyqtSignal(str)
    progress = pyqtSignal(int)
    starting = pyqtSignal(str)
    stopped = pyqtSignal()

class MainWindow(QMainWindow):

    def __init__(self, filename):
        super().__init__()
        self.setWindowTitle("avio")

        self.signals = Signals()
        self.signals.error.connect(self.showErrorDialog)
        self.signals.progress.connect(self.updateSlider)
        self.signals.starting.connect(self.updateDuration)
        self.signals.stopped.connect(self.playerStopped)

        self.playing = False
        self.closing = False
        self.mute = False
        self.duration = 0
        self.uri = filename
        self.player = avio.Player()

        self.btnPlay = QPushButton("Play")
        self.btnPlay.clicked.connect(self.btnPlayClicked)
        self.btnStop = QPushButton("Stop")
        self.btnStop.clicked.connect(self.btnStopClicked)
        self.btnRecord = QPushButton("Record")
        self.btnRecord.clicked.connect(self.btnRecordClicked)
        self.btnMute = QPushButton("Mute")
        self.btnMute.clicked.connect(self.btnMuteClicked)
        self.sldVolume = QSlider()
        self.sldVolume.setValue(80)
        self.sldVolume.setTracking(True)
        self.sldVolume.valueChanged.connect(self.sldVolumeChanged)
        pnlControl = QWidget()
        lytControl = QGridLayout(pnlControl)
        lytControl.addWidget(self.btnPlay,    0, 0, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytControl.addWidget(self.btnStop,    1, 0, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytControl.addWidget(self.btnRecord,  2, 0, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytControl.addWidget(self.sldVolume,  3, 0, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytControl.addWidget(self.btnMute,    4, 0, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytControl.addWidget(QLabel(),        5, 0, 1, 1)
        lytControl.setRowStretch(5, 10)

        self.glWidget = GLWidget()
        
        
        self.sldProgress = Slider(Qt.Orientation.Horizontal, self)
        self.sldProgress.setMaximum(1000)
        self.lblProgress = QLabel("0:00")
        self.setLabelWidth(self.lblProgress)
        self.lblDuration = QLabel("0:00")
        self.setLabelWidth(self.lblDuration)
        self.lblPosition = Position()

        pnlProgress = QWidget()
        lytProgress = QGridLayout(pnlProgress)
        lytProgress.addWidget(self.lblPosition,  0, 1, 1, 1)
        lytProgress.addWidget(self.lblProgress,  1, 0, 1, 1)
        lytProgress.addWidget(self.sldProgress,  1, 1, 1, 1)
        lytProgress.addWidget(self.lblDuration,  1, 2, 1, 1)
        lytProgress.setContentsMargins(0, 0, 0, 0)
        lytProgress.setColumnStretch(1, 10)
        

        pnlMain = QWidget()
        lytMain = QGridLayout(pnlMain)
        lytMain.addWidget(self.glWidget,   0, 0, 1, 1)
        lytMain.addWidget(pnlProgress,     1, 0, 1, 1)
        lytMain.addWidget(pnlControl,      0, 1, 2, 1)
        lytMain.setColumnStretch(0, 10)
        lytMain.setRowStretch(0, 10)

        self.setCentralWidget(pnlMain)

    def setLabelWidth(self, l):
        l.setFixedWidth(l.fontMetrics().boundingRect("00:00:00").width())
        l.setAlignment(Qt.AlignmentFlag.AlignCenter)

    def pythonCallback(self, F):
        img = np.array(F, copy = False)
        cv2.rectangle(img, (500, 600), (900, 800), (0, 255, 0), 5)
        return F

    def updateSlider(self, n):
        self.sldProgress.setValue(n)
        self.lblProgress.setText(self.timestring(int(self.duration * n / 1000)))

    def progressCallback(self, f):
        self.signals.progress.emit(int(f * self.sldProgress.maximum()))

    def btnMuteClicked(self):
        print("btnMuteClicked")
        self.mute = not self.mute
        self.player.setMute(self.mute)
        if self.mute:
            self.btnMute.setText("UnMute")
        else:
            self.btnMute.setText("Mute")

    def btnRecordClicked(self):
        print("btnRecordClicked")
        if self.playing:
            _, file_ext = os.path.splitext(self.player.uri)
            print(file_ext)
            filename = "output" + file_ext
            self.player.togglePiping(filename)
        self.setRecordButton()

    def sldVolumeChanged(self, value):
        self.player.setVolume(value)

    def showErrorDialog(self, s):
        msgBox = QMessageBox(self)
        msgBox.setWindowTitle("Error")
        msgBox.setText(s)
        msgBox.setStandardButtons(QMessageBox.StandardButton.Ok)
        msgBox.setIcon(QMessageBox.Icon.Critical)
        msgBox.exec()

    def closeEvent(self, e):
        print(e)
        if self.player.isPaused():
            self.player.togglePaused()
        self.closing = True
        self.player.running = False
        while self.playing:
            sleep(0.001)

    def btnStopClicked(self):
        print("btnStopClicked")
        self.player.running = False
        if self.player.isPaused():
            self.player.togglePaused()

    def playerStopped(self):
        if not self.closing:
            self.glWidget.clear()
            self.setPlayButton()
            self.setRecordButton()
            self.updateSlider(0)
            self.duration = 0
            self.lblDuration.setText("0:00")

    def mediaPlayingStopped(self):
        print("mediaPlayingStopped")
        self.playing = False
        self.signals.stopped.emit()

    def updateDuration(self, str):
        self.lblDuration.setText(str)
        self.setPlayButton()

    def timestring(self, n):
        time_interval = int(n / 1000)
        hours = int(time_interval / 3600)
        minutes = int ((time_interval - (hours * 3600)) / 60)
        seconds = int ((time_interval - (hours * 3600) - (minutes * 60)))
        if hours > 0:
            buf = "%02d:%02d:%02d" % (hours, minutes, seconds)
        else:
            buf = "%d:%02d" % (minutes, seconds)
        return buf

    def mediaPlayingStarted(self, n):
        print("mediaPlayingStarted", n)
        print(self.timestring(n))
        self.duration = n
        self.signals.starting.emit(self.timestring(n))

    def errorCallback(self, s):
        self.signals.error.emit(s)

    def setPlayButton(self):
        if self.playing:
            if self.player.isPaused():
                self.btnPlay.setText("Play")
            else:
                self.btnPlay.setText("Pause")
        else:
            self.btnPlay.setText("Play")

    def setRecordButton(self):
        if self.playing:
            if self.player.isPiping():
                self.btnRecord.setText("Recording...")
            else:
                self.btnRecord.setText("Record")
        else:
            self.btnRecord.setText("Record")

    def btnPlayClicked(self):
        print("btnPlay")
        if self.playing:
            self.player.togglePaused()
            self.setPlayButton()
        else:
            self.playing = True
            self.player.uri = self.uri
            self.player.width = lambda : self.glWidget.width()
            self.player.height = lambda : self.glWidget.height()
            #self.player.hWnd = self.glWidget.winId()
            self.player.progressCallback = lambda f : self.progressCallback(f)
            self.player.video_filter = "format=rgb24"
            self.player.renderCallback = lambda F : self.glWidget.renderCallback(F)
            self.player.pythonCallback = lambda F : self.pythonCallback(F)
            self.player.cbMediaPlayingStarted = lambda n : self.mediaPlayingStarted(n)
            self.player.cbMediaPlayingStopped = lambda : self.mediaPlayingStopped()
            self.player.errorCallback = lambda s : self.errorCallback(s)
            self.player.setVolume(self.sldVolume.value())
            self.player.setMute(self.mute)
            #self.player.disable_video = True
            #self.player.hw_device_type = avio.AV_HWDEVICE_TYPE_QSV
            self.player.start()

if __name__ == '__main__':

    if len(sys.argv) < 2:
        print("please specify media filename in command line")
    else:
        print(sys.argv[1])
        app = QApplication(sys.argv)
        window = MainWindow(sys.argv[1])
        window.show()

        app.exec()