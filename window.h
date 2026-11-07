#pragma once

#include <QElapsedTimer>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QPointF>
#include <QVector3D>

#include "openglwidget.h"

class QLabel;
class QMouseEvent;
class QSlider;
class QWidget;
class QWheelEvent;

class Window : public OpenGLWidget {
    Q_OBJECT

   public:
    Window();
    ~Window();

    void onInit() override;
    void onRender() override;
    void onResize(size_t width, size_t height) override;

   protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

   private:
    QPointF screenToWorld(const QPointF& position) const;

    QElapsedTimer timer_;
    GLint colorUniform_ = -1;
    GLint backgroundUniform_ = -1;
    float phase_ = 0.0;

    GLint mvpUniform_ = -1;

    QMatrix4x4 projection_;

    QOpenGLBuffer vbo_{QOpenGLBuffer::Type::VertexBuffer};
    QOpenGLVertexArrayObject vao_;
    std::unique_ptr<QOpenGLShaderProgram> program_;
    const int ATTRS_ = 3;
    const QVector3D BACKGROUND_ = {0.14f, 0.07f, 0.16f};

    bool animated_ = false;

    QWidget* controlsPanel_ = nullptr;
    QSlider* iterSlider_ = nullptr;
    QLabel* iterValueLabel_ = nullptr;
    QPointF lastMousePosition_;

    int iter_ = 1;
    const int LENGTH_ = 3;
    const float rectWidth_ = LENGTH_ / 12.0;

    float centerX_ = 0.0;
    float centerY_ = 0.0;
    float viewHeight_ = 16.0;
    bool dragging_ = false;
};
