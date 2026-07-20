#include "CaptureOverlay.h"

#include <QTimer>
#include <QPainter>
#include <QScreen>
#include <QGuiApplication>

CaptureOverlay::CaptureOverlay(QWidget* parent)
    : QWidget(parent),
      m_bgColor(Qt::red),
      m_textColor(Qt::white),
      m_text("trying to read text in this Area"),
      m_brush(QColor(255, 0, 0, 100)),
      m_lifetimeTimer(new QTimer(this)),
      m_lifeTime(30000)
{
    setWindowFlags(Qt::WindowStaysOnTopHint |
                   Qt::Tool |
                   Qt::FramelessWindowHint |
                   Qt::WindowTransparentForInput |
                   Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowOpacity(0.7);

    connect(m_lifetimeTimer, &QTimer::timeout,
            this, &CaptureOverlay::timeout);
}

CaptureOverlay::CaptureOverlay(const QColor& bgColor, const QColor& textColor, QString text, QWidget* parent)
    : QWidget(parent),
      m_bgColor(bgColor),
      m_textColor(textColor),
      m_text(std::move(text)),
      m_brush(QColor(125, 125, 125, 255)),
      m_lifetimeTimer(new QTimer(this)),
      m_lifeTime(5000) {
    setWindowFlags(Qt::WindowStaysOnTopHint |
                   Qt::Tool |
                   Qt::FramelessWindowHint |
                   Qt::WindowTransparentForInput |
                   Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowOpacity(1.0);

    connect(m_lifetimeTimer, &QTimer::timeout,
            this, &CaptureOverlay::timeout);
}

CaptureOverlay::~CaptureOverlay() {
    m_lifetimeTimer->stop();
}

void CaptureOverlay::showOnScreenWithGlobalRect(QScreen* screen, const QRect& globalRect) {
    if (!screen) screen = QGuiApplication::primaryScreen();

    const QRect screenGeom = screen->geometry();

    //window covers the full screen
    resize(screenGeom.size());
    move(screenGeom.topLeft());
    show();
    raise();

    //global capture rect into local coordinates of this window
    const QPoint offset = globalRect.topLeft() - screenGeom.topLeft();
    m_localPaintRect = QRect(offset, globalRect.size());

    update();
}

void CaptureOverlay::showTextOverlayOnScreen(QScreen* screen, const QRect& globalRect) {
    if (!screen) screen = QGuiApplication::primaryScreen();

    const QRect screenGeom = screen->geometry();

    resize(screenGeom.size());
    move(screenGeom.topLeft());
    show();
    raise();

    const QPoint offset = globalRect.topLeft() - screenGeom.topLeft();
    m_localPaintRect = QRect(offset, globalRect.size());

    update();
}

void CaptureOverlay::timeout() {
    hide();
    emit expired(this);
}

void CaptureOverlay::paintEvent(QPaintEvent*) {
    if (!m_localPaintRect.isValid())
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(QPen(m_bgColor, 3));
    painter.setBrush(m_brush);
    painter.drawRect(m_localPaintRect);

    if (!m_text.isEmpty()) {
        painter.setPen(QPen(m_textColor, 2));
        painter.drawText(m_localPaintRect, Qt::AlignCenter, m_text);
    }
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
    hide();
}
