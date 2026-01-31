#include "CaptureOverlay.h"

#include <QTimer>
#include <utility>

#include "QPainter"
#include "FileAccess/FileAccess.h"

CaptureOverlay::CaptureOverlay(QWidget* parent)
    : QWidget(parent), m_bgColor(Qt::red), m_textColor(Qt::white), m_text("trying to read text in this Area") {
    setWindowFlags(Qt::WindowStaysOnTopHint |
                   Qt::FramelessWindowHint |
                   Qt::WindowTransparentForInput |
                   Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowOpacity(0.7);

    m_lifetimeTimer = new QTimer(this);
    m_lifeTime = 30000;
    m_brush = QBrush(QColor(255, 0, 0, 100));
    connect(m_lifetimeTimer, &QTimer::timeout, this, &CaptureOverlay::timeout);
}

CaptureOverlay::CaptureOverlay(const QColor& bgColor, const QColor& textColor,
                               QString  text, QWidget* parent)
    : QWidget(parent), m_bgColor(bgColor), m_textColor(textColor), m_text(std::move(text)) {
    setWindowFlags(Qt::WindowStaysOnTopHint |
                   Qt::FramelessWindowHint |
                   Qt::WindowTransparentForInput |
                   Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowOpacity(1);

    m_lifetimeTimer = new QTimer(this);
    m_lifeTime = 5000;
    m_brush = QBrush(QColor(125, 125, 125, 255));
    connect(m_lifetimeTimer, &QTimer::timeout, this, &CaptureOverlay::timeout);
}

CaptureOverlay::~CaptureOverlay() {
    m_lifetimeTimer->stop();
}

void CaptureOverlay::timeout() {
    hide();
    emit expired(this);
}

void CaptureOverlay::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setPen(QPen(m_bgColor, 3));
    painter.setBrush(m_brush);
    painter.drawRect(rect());
    painter.setPen(QPen(m_textColor, 2));
    painter.drawText(rect(), Qt::AlignCenter, m_text);
}

void CaptureOverlay::showEvent(QShowEvent*) {
    m_lifetimeTimer->setSingleShot(true);
    m_lifetimeTimer->start(m_lifeTime);
}

void CaptureOverlay::hideEvent(QHideEvent*) {
    m_lifetimeTimer->stop();
}

void CaptureOverlay::earlyTimeout() {
    m_lifetimeTimer->stop();
    m_lifetimeTimer->deleteLater();
    this->deleteLater();
    timeout();
}
