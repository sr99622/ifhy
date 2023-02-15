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
    std::cout << "paint event start" << std::endl;
    mutex.lock();
    if (f.isValid()) {
        QPainter painter;
        painter.begin(this);
        QImage img = QImage(f.m_frame->data[0], f.m_frame->width, f.m_frame->height, QImage::Format_RGB888);
        img = img.scaled(width(), height(), Qt::KeepAspectRatio);
        int dx = width() - img.width();
        int dy = height() - img.height();
        painter.drawImage(dx/2, dy/2, img);
        painter.end();
    }
    mutex.unlock();
    std::cout << "paint event finish" << std::endl;
}

void GLWidget::renderCallback(const avio::Frame& frame)
{
    std::cout << "render callback start" << std::endl;
    if (!frame.isValid()) {
        std:: cout << "render callback recvd invalid Frame" << std::endl;
        return;
    }
    mutex.lock();
    f = frame;
    mutex.unlock();
    update();
    std::cout << "render callback finish" << std::endl;
}

void GLWidget::invalidateFrame()
{
    mutex.lock();
    f = avio::Frame(nullptr);
    mutex.unlock();
}

QSize GLWidget::sizeHint() const
{
    return QSize(640, 480);
}