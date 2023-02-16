#include <iostream>
#include <QPainter>
#include <QImage>

#include "glwidget.h"

GLWidget::GLWidget()
{

}

GLWidget::~GLWidget()
{

}

void GLWidget::paintEvent(QPaintEvent* event)
{
    if (f.isValid()) {
        mutex.lock();
        QPainter painter;
        painter.begin(this);
        QImage img = QImage(f.m_frame->data[0], f.m_frame->width, f.m_frame->height, QImage::Format_RGB888);
        img = img.scaled(width(), height(), Qt::KeepAspectRatio);
        int dx = width() - img.width();
        int dy = height() - img.height();
        painter.drawImage(dx/2, dy/2, img);
        painter.end();
        mutex.unlock();
    }
    else {
        QOpenGLWidget::paintEvent(event);
    }
}

void GLWidget::renderCallback(const avio::Frame& frame)
{
    if (!frame.isValid()) {
        std:: cout << "render callback recvd invalid Frame" << std::endl;
        return;
    }
    mutex.lock();
    f = frame;
    mutex.unlock();
    update();
}

QSize GLWidget::sizeHint() const
{
    return QSize(640, 480);
}