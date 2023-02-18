import sys
import build.avio as avio
import numpy as np
import platform
import cv2
from time import sleep
from datetime import timedelta
from PyQt6.QtWidgets import QApplication, QMainWindow, QPushButton, \
QGridLayout, QWidget, QSlider, QLabel, QMessageBox
from PyQt6.QtCore import Qt, QSize, QRect, pyqtSignal, QObject
from PyQt6.QtGui import QPainter, QImage, QFont, QFontMetrics
from PyQt6.QtOpenGLWidgets import QOpenGLWidget

class Signals(QObject):
    error = pyqtSignal(str)
    progress = pyqtSignal(int)
    starting = pyqtSignal(str)

class AVWidget(QOpenGLWidget):
    def __init__(self):
        super().__init__()
        self.image = QImage()
    
    def sizeHint(self):
        return QSize(640, 480)

    def renderCallback(self, F):
        ary = np.array(F, copy = False)
        h, w, d = ary.shape
        self.image = QImage(ary.data, w, h, d * w, QImage.Format.Format_RGB888)
        self.update()

    def getImageRect(self):
        ratio = min(self.width() / self.image.width(), self.height() / self.image.height())
        w = self.image.width() * ratio
        h = self.image.height() * ratio
        x = (self.width() - w) / 2
        y = (self.height() - h) / 2
        return QRect(int(x), int(y), int(w), int(h))

    def paintGL(self):
        if (not self.image.isNull()):
            painter = QPainter(self)
            painter.setRenderHint(QPainter.RenderHint.Antialiasing)
            painter.drawImage(self.getImageRect(), self.image)

    def clear(self):
        self.image.fill(0)
        self.update()

class Slider(QSlider):
    def __init__(self, o, w):
        super().__init__(o)
        self.w = w

    def mousePressEvent(self, e):
        pct = e.position().x() / self.width()
        self.w.player.seek(pct)

class MainWindow(QMainWindow):

    def __init__(self, filename):
        super().__init__()
        self.setWindowTitle("avio")

        self.signals = Signals()
        self.signals.error.connect(self.showErrorDialog)
        self.signals.progress.connect(self.updateSlider)
        self.signals.starting.connect(self.updateDuration)

        self.playing = False
        self.duration = 0
        self.uri = filename
        self.player = avio.Player()

        self.btnPlay = QPushButton("play")
        self.btnPlay.clicked.connect(self.btnPlayClicked)
        self.btnPause = QPushButton("pause")
        self.btnPause.clicked.connect(self.btnPauseClicked)
        self.btnStop = QPushButton("stop")
        self.btnStop.clicked.connect(self.btnStopClicked)
        pnlControl = QWidget()
        lytControl = QGridLayout(pnlControl)
        lytControl.addWidget(self.btnPlay,    0, 0, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytControl.addWidget(self.btnPause,   1, 0, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytControl.addWidget(self.btnStop,    2, 0, 1, 1, Qt.AlignmentFlag.AlignCenter)

        self.avWidget = AVWidget()
        
        self.sldProgress = Slider(Qt.Orientation.Horizontal, self)
        self.sldProgress.setMaximum(1000)
        self.lblProgress = QLabel("0:00")
        self.setLabelWidth(self.lblProgress)
        self.lblDuration = QLabel("0:00")
        self.setLabelWidth(self.lblDuration)
        pnlProgress = QWidget()
        lytProgress = QGridLayout(pnlProgress)
        lytProgress.addWidget(self.lblProgress,  0, 0, 1, 1)
        lytProgress.addWidget(self.sldProgress,  0, 1, 1, 1)
        lytProgress.addWidget(self.lblDuration,  0, 2, 1, 1)
        lytProgress.setContentsMargins(0, 0, 0, 0)
        lytProgress.setColumnStretch(1, 10)

        pnlMain = QWidget()
        lytMain = QGridLayout(pnlMain)
        lytMain.addWidget(self.avWidget,   0, 0, 1, 1)
        lytMain.addWidget(pnlProgress,     1, 0, 1, 1)
        lytMain.addWidget(pnlControl,      0, 1, 2, 1)
        lytMain.setColumnStretch(0, 10)
        lytMain.setRowStretch(0, 10)

        self.setCentralWidget(pnlMain)

    def setLabelWidth(self, l):
        font = QFont("Arial")
        l.setFont(font)
        metrics = l.fontMetrics()
        rect = metrics.boundingRect("00:00:00")
        l.setFixedWidth(rect.width())
        l.setAlignment(Qt.AlignmentFlag.AlignCenter)

    def closeEvent(self, e):
        print(e)
        self.player.running = False
        while self.playing:
            sleep(0.001)

    def pythonCallback(self, F):
        img = np.array(F, copy = False)
        cv2.rectangle(img, (500, 600), (900, 800), (0, 255, 0), 5)
        return F

    def updateSlider(self, n):
        self.sldProgress.setValue(n)
        self.lblProgress.setText(self.timestring(int(self.duration * n / 1000)))

    def progressCallback(self, f):
        self.signals.progress.emit(int(f * self.sldProgress.maximum()))

    def btnPauseClicked(self):
        print("btnPauseClicked")
        self.player.togglePaused()

    def btnStopClicked(self):
        print("btnStopClicked")
        self.player.running = False

    def showErrorDialog(self, s):
        msgBox = QMessageBox(self)
        msgBox.setWindowTitle("Error")
        msgBox.setText(s)
        msgBox.setStandardButtons(QMessageBox.StandardButton.Ok)
        msgBox.setIcon(QMessageBox.Icon.Critical)
        msgBox.exec()

    def mediaPlayingStopped(self):
        print("mediaPlayingStopped")
        self.avWidget.clear()
        self.playing = False

    def updateDuration(self, str):
        self.lblDuration.setText(str)

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

    def btnPlayClicked(self):
        print("btnPlay")
        if self.playing:
            return

        self.playing = True
        self.player.uri = self.uri
        self.player.width = lambda : self.avWidget.width()
        self.player.height = lambda : self.avWidget.height()
        #self.player.hWnd = self.avWidget.winId()
        self.player.progressCallback = lambda f : self.progressCallback(f)
        self.player.video_filter = "format=rgb24"
        self.player.renderCallback = lambda F : self.avWidget.renderCallback(F)
        self.player.pythonCallback = lambda F : self.pythonCallback(F)
        self.player.cbMediaPlayingStarted = lambda n : self.mediaPlayingStarted(n)
        self.player.cbMediaPlayingStopped = lambda : self.mediaPlayingStopped()
        self.player.errorCallback = lambda s : self.errorCallback(s)
        #self.player.disable_video = True
        #self.player.hw_device_type = avio.AV_HWDEVICE_TYPE_QSV
        self.player.start()

if __name__ == '__main__':

    print(platform.system())

    if len(sys.argv) < 2:
        print("please specify media filename in command line")
    else:
        print(sys.argv[1])
        app = QApplication(sys.argv)
        window = MainWindow(sys.argv[1])
        window.show()

        app.exec()