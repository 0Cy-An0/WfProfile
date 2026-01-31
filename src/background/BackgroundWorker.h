#pragma once

#include <QThread>
#include <QTimer>
#include <atomic>
#include <QObject>
#include <QString>

#include "window/CaptureOverlay.h"

class MainWindow;

class BackgroundWorker : public QObject {
    Q_OBJECT
public:
    explicit BackgroundWorker(MainWindow* mainWindow);

public slots:
    void startWork();
    void stopWork();

private slots:
    void backgroundWork();

private:
    struct LogEvents {
        bool missionFinished = false;
        bool rewardsShown = false;
    };

    LogEvents tailLogFile();

    void captureRewardScreen();

    QString performOCR(const QPixmap &pixmap);

    void parseItemsAndShowOverlay(const std::string &ocrText, int sectionIndex, QRect captureRect);

    void showSectionOverlay(const std::string &reward, int sectionIndex, QRect captureRect);

    static bool parseMissionFinish(const std::string& line);
    static bool parseRewardsTrigger(const std::string& line);

    void triggerUpdate() const;

    static void fetchGameData();

    std::atomic<bool> running;
    int timersElapsed = 0;
    std::streampos lastReadPos = 0;
    std::vector<CaptureOverlay*> b_overlays;

    MainWindow* mainWindow;
    QTimer* timer = nullptr;
};
