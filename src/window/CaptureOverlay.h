#ifndef CAPTUREOVERLAY_H
#define CAPTUREOVERLAY_H

#include <QWidget>
#include <QScreen>

class CaptureOverlay : public QWidget {
    Q_OBJECT
public:
    explicit CaptureOverlay(QWidget* parent = nullptr);

    CaptureOverlay(const QColor &bgColor, const QColor &textColor, QString text, QWidget *parent);

    ~CaptureOverlay() override;

    void earlyTimeout();

signals:
    void expired(CaptureOverlay *overlay);

private slots:
    void timeout();

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    QTimer* m_lifetimeTimer;
    QColor m_bgColor;
    QColor m_textColor;
    QString m_text;
    QBrush m_brush;
    int m_lifeTime;
};

#endif
