import sys
import build.avio as avio
import numpy as np
import platform
import cv2
from PyQt6.QtWidgets import QApplication, QMainWindow, QPushButton, \
QGridLayout, QWidget, QSlider, QLabel
from PyQt6.QtCore import Qt, QSize, QRect
from PyQt6.QtGui import QPainter, QImage, QGuiApplication
from PyQt6.QtOpenGLWidgets import QOpenGLWidget

class AVWidget(QOpenGLWidget):
    def __init__(self):
        super().__init__()
        self.image = QImage()
    
    def sizeHint(self):
        return QSize(640, 480)

    def renderCallback(self, f):
        ary = np.array(f, copy = False)
        h, w, d = ary.shape
        self.image = QImage(ary.data, w, h, d * w, QImage.Format.Format_RGB888)
        self.update()

    def getImageRect(self):
        ratio = min(self.width() / self.image.width(), self.height() / self.image.height())
        width = self.image.width() * ratio
        height = self.image.height() * ratio
        x = (self.width() - width) / 2
        y = (self.height() - height) / 2
        return QRect(int(x), int(y), int(width), int(height))

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
        self.count = 0
        self.uri = filename
        self.player = avio.Player()

        self.btnPlay = QPushButton("play")
        self.btnPlay.clicked.connect(self.btnPlayClicked)

        self.btnPause = QPushButton("pause")
        self.btnPause.clicked.connect(self.btnPauseClicked)

        self.btnStop = QPushButton("stop")
        self.btnStop.clicked.connect(self.btnStopClicked)

        self.avWidget = AVWidget()
        self.progress = QSlider(Qt.Orientation.Horizontal)

        pnlMain = QWidget()
        lytMain = QGridLayout(pnlMain)
        lytMain.addWidget(self.avWidget,           0, 0, 6, 1)
        lytMain.addWidget(self.progress,           6, 0, 1, 1)
        lytMain.addWidget(self.btnPlay,            0, 1, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytMain.addWidget(self.btnPause,           1, 1, 1, 1, Qt.AlignmentFlag.AlignCenter)
        lytMain.addWidget(self.btnStop,            2, 1, 1, 1, Qt.AlignmentFlag.AlignCenter)

        self.setCentralWidget(pnlMain)

    def closeEvent(self, e):
        print(e)
        self.player.running = False
        from time import sleep
        sleep(0.5)

    def pythonCallback(self, f):
        img = np.array(f, copy = False)
        cv2.rectangle(img, (500, 600), (900, 800), (0, 255, 0), 5)
        return f

    def updateProgress(self, n):
        self.progress.setValue(int(n * 100))
        self.progress.update()

    def btnPauseClicked(self):
        print("btnPauseClicked")
        self.player.togglePaused()

    def btnStopClicked(self):
        print("btnStopClicked")
        self.player.running = False

    def mediaPlayingStopped(self):
        self.avWidget.clear()

    def btnPlayClicked(self):
        try:
            print("btnPlay")
            if self.player.running:
                return

            self.player.uri = self.uri
            self.player.width = lambda : self.avWidget.width()
            self.player.height = lambda : self.avWidget.height()
            #self.player.hWnd = self.avWidget.winId()
            self.player.video_filter = "format=rgb24"
            self.player.renderCallback = lambda f : self.avWidget.renderCallback(f)
            self.player.pythonCallback = lambda f : self.pythonCallback(f)
            self.player.cbMediaPlayingStopped = lambda : self.mediaPlayingStopped()

            #self.player.disable_video = True
            self.player.hw_device_type = avio.AV_HWDEVICE_TYPE_VDPAU
            self.player.start()

        except Exception as e:
            print(e)

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