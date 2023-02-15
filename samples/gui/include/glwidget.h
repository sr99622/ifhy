#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QMutex>
#include <QPaintEvent>
#include "avio.h"

class GLWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    GLWidget();
    ~GLWidget();
    void renderCallback(const avio::Frame& frame);
    QSize sizeHint() const override;
    void invalidateFrame();

    avio::Frame f;
    QMutex mutex;

protected:
    void paintEvent(QPaintEvent* event) override;

};

#endif // GLWIDGET_H