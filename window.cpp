#include "window.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QSlider>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidget>
#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include <vector>

namespace {

QPointF mousePosition(const QMouseEvent* event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->position();
#else
    return event->localPos();
#endif
}

QPointF wheelPosition(const QWheelEvent* event) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return event->position();
#else
    return event->posF();
#endif
}

}  // namespace

Window::Window() {
    controlsPanel_ = new QWidget(this);
    controlsPanel_->setStyleSheet("QLabel { font-size: 12px; color: rgb(255, 255, 255); }");
    auto* controlsLayout = new QVBoxLayout(controlsPanel_);

    // Add label for FPS
    auto* rowLable = new QHBoxLayout;
    auto* fpsLabel = new QLabel("FPS: 0", this);
    fpsLabel->setStyleSheet("QLabel { color: white; }");
    fpsLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    rowLable->addWidget(fpsLabel);
    controlsLayout->addLayout(rowLable);
    connect(this, &Window::fpsUpdated, [fpsLabel](double fps) {
        fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 0));
        fpsLabel->adjustSize();
    });

    // slider
    auto* row = new QHBoxLayout;
    auto* nameLabel = new QLabel("iterations num");
    iterSlider_ = new QSlider(Qt::Horizontal, controlsPanel_);
    iterSlider_->setRange(1, 7);
    iterSlider_->setValue(1);
    iterSlider_->setFixedWidth(100);
    iterValueLabel_ = new QLabel("1");

    row->addWidget(nameLabel);
    row->addWidget(iterSlider_);
    row->addWidget(iterValueLabel_);
    controlsLayout->addLayout(row);
    connect(iterSlider_, &QSlider::valueChanged, this, [this] {
        iter_ = iterSlider_->value();
        iterValueLabel_->setText(QString::number(iter_));
    });

    controlsPanel_->adjustSize();
    controlsPanel_->raise();
}

Window::~Window() {
    program_.reset();
}

void Window::onInit() {
    // Configure shaders
    program_ = std::make_unique<QOpenGLShaderProgram>(this);
    program_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/template.vs");
    program_->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/template.fs");
    if ( !program_->link() ) {
        qDebug() << "Ошибка линковки:" << program_->log();
    }
    program_->bind();

    // Configure uniforms
    mvpUniform_ = program_->uniformLocation("mvp");
    colorUniform_ = program_->uniformLocation("color");
    backgroundUniform_ = program_->uniformLocation("background");
    program_->setUniformValue(colorUniform_, QColor(247, 107, 27));
    program_->setUniformValue(backgroundUniform_, BACKGROUND_);

    // Create VAO object
    vao_.create();
    vao_.bind();

    // Create VBO
    vbo_.create();
    vbo_.bind();
    vbo_.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    // Bind attributes
    program_->enableAttributeArray(0);
    program_->setAttributeBuffer(0, GL_FLOAT, 0, 2, static_cast<int>(ATTRS_ * sizeof(GLfloat)));

    program_->enableAttributeArray(1);
    program_->setAttributeBuffer(1, GL_FLOAT, static_cast<int>(2 * sizeof(GLfloat)), 1, static_cast<int>(ATTRS_ * sizeof(GLfloat)));

    // Release all
    program_->release();

    vao_.release();
    vbo_.release();

    // Start timer for animation (if needed)
    timer_.start();

    // disable depth test
    glDisable(GL_DEPTH_TEST);

    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(BACKGROUND_[0], BACKGROUND_[1], BACKGROUND_[2], 1.0f);
}

void Window::onRender() {
    glClear(GL_COLOR_BUFFER_BIT);

    // fractal
    std::string curr = "A";
    for ( int i = 0; i < iter_; ++i ) {
        std::string next = "";
        for ( char c : curr ) {
            if ( c == 'A' )
                next += "A-B--B+A++AA+B-";
            else if ( c == 'B' )
                next += "+A-BB--B-A++A+B";
            else
                next += c;
        }
        curr = next;
    }

    float x = 0.0, y = 0.0, currAngle = 0.0;
    const float stepAngle = M_PI / 3.0;

    std::vector<float> fractalVertices = {x, y};
    fractalVertices.reserve(std::pow(7, iter_) + 1);

    for ( char c : curr ) {
        if ( c == '+' )
            currAngle += stepAngle;
        else if ( c == '-' )
            currAngle -= stepAngle;
        else if ( c == 'A' || c == 'B' ) {
            x += LENGTH_ * std::cos(currAngle);
            y += LENGTH_ * std::sin(currAngle);
            fractalVertices.insert(fractalVertices.end(), {x, y});
        }
    }

    // projection
    const float halfHeight = viewHeight_ / 2.0;
    const float halfWidth = halfHeight * width() / height();

    projection_.setToIdentity();
    projection_.ortho(
        centerX_ - halfWidth,
        centerX_ + halfWidth,
        centerY_ - halfHeight,
        centerY_ + halfHeight,
        -1.0f, 1.0f);

    // Bind shader program
    program_->bind();

    // Update uniform values
    program_->setUniformValue(mvpUniform_, projection_);

    float rectWidth = rectWidth_;
    if ( animated_ ) {
        const float dt = timer_.restart() / 100.0f;
        phase_ = std::fmod(phase_ + dt / 20.0f, 1.0f);
        rectWidth = rectWidth_ * std::sin(M_PI * phase_);
    }

    // draw rectangles around fractal lines
    std::vector<GLfloat> vertices;

    for ( int i = 0; i < fractalVertices.size() - 3; i += 2 ) {
        const float Ax = fractalVertices[i],
                    Ay = fractalVertices[i + 1],
                    Bx = fractalVertices[i + 2],
                    By = fractalVertices[i + 3];
        const float dx = Bx - Ax, dy = By - Ay;
        const float len = std::sqrt(dx * dx + dy * dy);
        const float ux = dx / len, uy = dy / len;

        // two triangles
        vertices.insert(vertices.end(), {Ax - rectWidth * (0.4f * ux + uy), Ay + rectWidth * (ux - 0.4f * uy), 1.0f});
        vertices.insert(vertices.end(), {Ax - rectWidth * (0.4f * ux - uy), Ay - rectWidth * (ux + 0.4f * uy), -1.0f});
        vertices.insert(vertices.end(), {Bx + rectWidth * (0.4f * ux - uy), By + rectWidth * (ux + 0.4f * uy), 1.0f});
        vertices.insert(vertices.end(), {Bx + rectWidth * (0.4f * ux - uy), By + rectWidth * (ux + 0.4f * uy), 1.0f});
        vertices.insert(vertices.end(), {Ax - rectWidth * (0.4f * ux - uy), Ay - rectWidth * (ux + 0.4f * uy), -1.0f});
        vertices.insert(vertices.end(), {Bx + rectWidth * (0.4f * ux + uy), By - rectWidth * (ux - 0.4f * uy), -1.0f});
    }

    // Bind all
    vao_.bind();
    vbo_.bind();

    // Draw
    vbo_.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(GLfloat)));
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / ATTRS_));

    // Release all
    vao_.release();
    vbo_.release();
    program_->release();

    update();
}

void Window::onResize(const size_t width, const size_t height) {
    // Configure viewport
    glViewport(0, 0, static_cast<GLint>(width), static_cast<GLint>(height));
}

QPointF Window::screenToWorld(const QPointF& position) const {
    const double worldUnitsPerPixel = viewHeight_ / height();
    return QPointF(centerX_ + (position.x() - width() / 2.0) * worldUnitsPerPixel,
                   centerY_ - (position.y() - height() / 2.0) * worldUnitsPerPixel);
}

void Window::mousePressEvent(QMouseEvent* event) {
    if ( event->button() == Qt::LeftButton ) {
        dragging_ = true;
        lastMousePosition_ = mousePosition(event);
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    OpenGLWidget::mousePressEvent(event);
}

void Window::mouseMoveEvent(QMouseEvent* event) {
    if ( dragging_ ) {
        const QPointF position = mousePosition(event);
        const QPointF previousWorld = screenToWorld(lastMousePosition_);
        const QPointF currentWorld = screenToWorld(position);
        centerX_ += previousWorld.x() - currentWorld.x();
        centerY_ += previousWorld.y() - currentWorld.y();
        lastMousePosition_ = position;
        update();
        event->accept();
        return;
    }
    OpenGLWidget::mouseMoveEvent(event);
}

void Window::mouseReleaseEvent(QMouseEvent* event) {
    if ( event->button() == Qt::LeftButton && dragging_ ) {
        dragging_ = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    OpenGLWidget::mouseReleaseEvent(event);
}

void Window::wheelEvent(QWheelEvent* event) {
    const QPointF position = wheelPosition(event);
    const QPointF anchor = screenToWorld(position);
    viewHeight_ = viewHeight_ * std::pow(1.25, -event->angleDelta().y() / 80.0);
    const QPointF zoomedPoint = screenToWorld(position);
    centerX_ += anchor.x() - zoomedPoint.x();
    centerY_ += anchor.y() - zoomedPoint.y();

    update();
    event->accept();
}