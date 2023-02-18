import sys
import build.avio as avio
import numpy as np
import platform
import cv2
from time import sleep
from PyQt6.QtWidgets import QApplication, QMainWindow, QPushButton, \
QGridLayout, QWidget, QSlider, QLabel, QMessageBox
from PyQt6.QtCore import Qt, QSize, QRect, pyqtSignal, QObject
from PyQt6.QtGui import QPainter, QImage, QGuiApplication
from PyQt6.QtOpenGLWidgets import QOpenGLWidget

class Signals(QObject):
    error = pyqtSignal(str)
    progress = pyqtSignal(int)

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

class MainWindow(QMainWindow):

    def __init__(self, filename):
        super().__init__()
        self.setWindowTitle("avio")

        self.signals = Signals()
        self.signals.error.connect(self.showErrorDialog)
        self.signals.progress.connect(self.updateSlider)

        self.playing = False
        self.uri = filename
        self.player = avio.Player()

        self.btnPlay = QPushButton("play")
        self.btnPlay.clicked.connect(self.btnPlayClicked)

        self.btnPause = QPushButton("pause")
        self.btnPause.clicked.connect(self.btnPauseClicked)

        self.btnStop = QPushButton("stop")
        self.btnStop.clicked.connect(self.btnStopClicked)

        self.btnTest = QPushButton("test")
        self.btnTest.clicked.connect(self.btnTestClicked)

        self.avWidget = AVWidget()
        self.progress = QSlider(Qt.Orientation.Horizontal)
        self.progress.setMaximum(1000)

        pnlMain = QWidget()
        lytMain = QGridLayout(pnlMain)
        lytMain.addWidget(self.avWidget,           0, 0, 6, 1)
        lytMain.addWidget(self.progress,           6, 0, 1, 1)
        lytMain.addWidget(self.btnPlay,            0, 1, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytMain.addWidget(self.btnPause,           1, 1, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytMain.addWidget(self.btnStop,            2, 1, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytMain.addWidget(self.btnTest,            3, 1, 1, 1, Qt.AlignmentFlag.AlignCenter)

        self.setCentralWidget(pnlMain)

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
        self.progress.setValue(n)
        self.progress.update()

    def progressCallback(self, f):
        print("progressCallback", f)
        self.signals.progress.emit(int(f * self.progress.maximum()))

    def btnPauseClicked(self):
        print("btnPauseClicked")
        self.player.togglePaused()

    def btnStopClicked(self):
        print("btnStopClicked")
        self.player.running = False

    def btnTestClicked(self):
        print("this is a test")

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