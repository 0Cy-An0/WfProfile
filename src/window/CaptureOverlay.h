#pragma once

#include <QWidget>
#include <QTimer>
#include <QBrush>
#include <QRect>
#include <QString>

#include "FileAccess/FileAccess.h"

struct CaptureSection {
    QRect rect;
    int index;
};

static std::vector<CaptureSection> makeCaptureSections(const Settings& s) {
    QRect rect(s.captureTopLeft, s.captureBottomRight);
    std::vector<CaptureSection> result;

    if (rect.isEmpty() || rect.isNull()) return result;

    int sections = s.sections;
    if (sections < 1) return result;

    int sectionWidth = rect.width() / sections;
    int sectionHeight = rect.height();

    for (int i = 0; i < sections; ++i) {
        QRect sectionRect(
            rect.x() + i * sectionWidth,
            rect.y(),
            sectionWidth,
            sectionHeight
        );
        result.push_back({sectionRect, i});
    }
    return result;
}


class QScreen;

class CaptureOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit CaptureOverlay(QWidget* parent = nullptr);
    CaptureOverlay(const QColor& bgColor,
                   const QColor& textColor,
                   QString text,
                   QWidget* parent = nullptr);
    ~CaptureOverlay() override;

    // Show as a fullscreen overlay on the given screen and highlight a global rect.
    void showOnScreenWithGlobalRect(QScreen* screen, const QRect& globalRect);

    // For text overlays that just use the given global overlay rect
    void showTextOverlayOnScreen(QScreen* screen, const QRect& globalRect);

    void earlyTimeout();

    signals:
        void expired(CaptureOverlay* self);

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void timeout();

private:
    QColor m_bgColor;
    QColor m_textColor;
    QString m_text;
    QBrush m_brush;

    QTimer* m_lifetimeTimer;
    int m_lifeTime;

    // Rectangle to paint inside this fullscreen window, in *local* coordinates
    QRect m_localPaintRect;
};