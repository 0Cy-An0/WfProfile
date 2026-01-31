#include "BackgroundWorker.h"

#include <tesseract/baseapi.h>
#include <QTimer>
#include <QMetaObject>
#include <QBuffer>
#include <fstream>
#include <qguiapplication.h>
#include <qscreen.h>
#include <regex>

#include <EELogParser/logParser.h>
#include "window/mainwindow.h"
#include "apiParser/apiParser.h"
#include "FileAccess/FileAccess.h"
#include "window/CaptureOverlay.h"

BackgroundWorker::BackgroundWorker(MainWindow* mw)
    : QObject(nullptr), running(true), mainWindow(mw), timer(new QTimer(this)) {
    timer->setInterval(100);
    connect(timer, &QTimer::timeout, this, &BackgroundWorker::backgroundWork);
}

void BackgroundWorker::startWork() {
    running = true;
    timersElapsed = 0;
    timer->start();
    LogThis("called Backgroundworker startWork");
}

void BackgroundWorker::stopWork() {
    running = false;
    timer->stop();
    for (CaptureOverlay* overlay : b_overlays) {
        if (overlay) {
            overlay->earlyTimeout();
            overlay->setParent(mainWindow); //to be safe im setting parent to Main window check main window closeEvent for further reasoning
        }
    }
    qDeleteAll(b_overlays);
    b_overlays.clear();
}

void BackgroundWorker::backgroundWork() {
    if (!running) return;

    Settings settings = MainWindow::getWindowSettings();

    bool shouldUpdate = false;

    if (settings.autoSync) {
        ++timersElapsed;
        if (timersElapsed >= settings.syncTime) {
            LogThis("triggering Sync: timer");
            timersElapsed = 0;
            shouldUpdate = true;
        }
    }

    LogThis("", true); //I am so very confused. read comment for the same in tailLogFile too
    auto events = tailLogFile();

    if (settings.syncOnMissionFinish && events.missionFinished) {
        LogThis("triggering Sync: mission finish");
        shouldUpdate = true;
    }

    if (settings.relicOverlay && events.rewardsShown) {
        LogThis("InvokingRelic");
        QTimer::singleShot(500, [this]() {
            captureRewardScreen();
        });
    }

    if (shouldUpdate) {
        triggerUpdate();
    }
}

static bool initialized = false;
BackgroundWorker::LogEvents BackgroundWorker::tailLogFile() {
    LogEvents events;

    const std::string EElogPath = findLog();
    if (EElogPath.empty()) return events;

    std::ifstream file(EElogPath, std::ios::in | std::ios::binary);
    if (!file.is_open()) return events;

    // Truncation check and init (Game restart/start)
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    if (lastReadPos > fileSize) {
        LogThis("EELog was cleared");
        lastReadPos = 0;
        initialized = false;
    }

    // First time setup: Scan whole file to prime position
    if (!initialized) {
        //jump to end
        int pos = fileSize;

        while (pos > 0) {
            file.seekg(--pos);
            if (file.peek() == '\n') {
                file.get();
                break;
            }
        }
        lastReadPos = pos;
        initialized = true;
        return events; //its fine if this misses stuff
    }

    //tailing EE.log now
    file.clear();
    file.seekg(lastReadPos);
    std::string line;
    std::streampos currentPos = lastReadPos;

    while (std::getline(file, line)) {
        std::streampos afterLinePos = file.tellg();


        // Line complete check
        if (file.eof() && (line.empty() || (line.back() != '\n' && line.back() != '\r'))) {
            break;
        }

        //I implore you to not touch this. at all. i cant tell you what i tried instead of exactly this. and its alot. Nothing works, it doesnt make sense; if you are smarter then me and see this pls tell me what the fuck is going on
        LogThis("", true);
        if (parseMissionFinish(line)) {
            events.missionFinished = true;
        }
        if (parseRewardsTrigger(line)) {
            events.rewardsShown = true;
        }

        currentPos = afterLinePos;
    }

    lastReadPos = currentPos;
    return events;
}

void BackgroundWorker::captureRewardScreen() {
    LogThis("trying to capture screen");
    Settings s = MainWindow::getWindowSettings();

    QRect rect(s.captureTopLeft, s.captureBottomRight);
    if (rect.isEmpty() || rect.isNull()) {
        LogThis("Invalid capture rect");
        return;
    }

    QScreen* screen = QGuiApplication::screenAt(rect.center());
    if (!screen) screen = QGuiApplication::primaryScreen();

    int sections = s.sections;
    if (sections < 1) {
        LogThis("number of sections below 1");
        return;
    }

    int sectionWidth = rect.width() / sections;
    int sectionHeight = rect.height();

    // Capture each section separately
    for (int i = 0; i < sections; ++i) {
        QRect sectionRect(
            rect.x() + i * sectionWidth,
            rect.y(),
            sectionWidth,
            sectionHeight
        );

        QPixmap sectionShot = screen->grabWindow(0,
            sectionRect.x(), sectionRect.y(),
            sectionRect.width(), sectionRect.height());

        if (sectionShot.isNull()) { LogThis("Failed to capture section " + std::to_string(i+1)); continue; }

        LogThis("Captured section " + std::to_string(i+1) +
                ": x=" + std::to_string(sectionRect.x()) +
                ", w=" + std::to_string(sectionRect.width()));

        //ocr has trouble reading the rewards so im trying to make the text more readable first
        QImage img = sectionShot.toImage().convertToFormat(QImage::Format_ARGB32);
        if (img.isNull()) {
            LogThis("Failed to convert section " + std::to_string(i+1) + " to QImage");
            continue;
        }

        //some kind of contrast boosting magic, which i hope works; if you read this it did
        double factor = 3.0;
        for (int y = 0; y < img.height(); ++y) {
            for (int x = 0; x < img.width(); ++x) {
                QRgb pixel = img.pixel(x, y);
                int r = qRed(pixel), g = qGreen(pixel), b = qBlue(pixel);

                double r_n = r / 255.0;
                r_n = (r_n - 0.5) * factor + 0.5;
                r_n = qBound(0.0, r_n, 1.0);
                r = static_cast<int>(r_n * 255);

                double g_n = g / 255.0;
                g_n = (g_n - 0.5) * factor + 0.5;
                g_n = qBound(0.0, g_n, 1.0);
                g = static_cast<int>(g_n * 255);

                double b_n = b / 255.0;
                b_n = (b_n - 0.5) * factor + 0.5;
                b_n = qBound(0.0, b_n, 1.0);
                b = static_cast<int>(b_n * 255);

                img.setPixel(x, y, qRgb(r, g, b));
            }
        }

        QPixmap enhancedShot = QPixmap::fromImage(img);

        QString sectionOcr = performOCR(enhancedShot);
        LogThis("Section " + std::to_string(i+1) + " OCR: " + sectionOcr.toStdString());

        parseItemsAndShowOverlay(sectionOcr.toStdString(), i, sectionRect);
    }

    LogThis("Processed " + std::to_string(sections) + " reward sections");
}

QString BackgroundWorker::performOCR(const QPixmap& pixmap) {
    auto* api = new tesseract::TessBaseAPI();

    // Use tessdata next to exe
    QString tessdataPath = QCoreApplication::applicationDirPath() + "/tessdata";
    if (api->Init(tessdataPath.toStdString().c_str(), "eng") != 0) {
        LogThis("Tesseract Init failed: " + tessdataPath.toStdString());
        api->End();
        delete api;
        return "OCR Init failed";
    }
    api->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
    // Restrict to these chars only
    api->SetVariable("tessedit_char_whitelist", " -0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

    QImage img = pixmap.toImage();
    api->SetImage(img.bits(), img.width(), img.height(), 4, 4 * img.width());

    char* outText = api->GetUTF8Text();
    QString result = QString::fromUtf8(outText);

    api->End();
    delete[] outText;
    return result.trimmed();
}

double levenshtein(const std::string& s1, const std::string& s2) {
    int m = s1.size(), n = s2.size();
    std::vector<std::vector<int>> d(m + 1, std::vector<int>(n + 1));
    for (int i = 0; i <= m; ++i) d[i][0] = i;
    for (int j = 0; j <= n; ++j) d[0][j] = j;
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            d[i][j] = min(d[i - 1][j] + 1, min(d[i][j - 1] + 1, d[i - 1][j - 1] + cost));
        }
    }
    return 1.0 - static_cast<double>(d[m][n]) / max(m, n);  // Normalized similarity
}

double ngramScore(const std::string& a, const std::string& b, int n = 3) {
    if (a.size() < n || b.size() < n) return 0.0;
    std::vector<std::string> gramsA, gramsB;
    for (size_t i = 0; i <= a.size() - n; ++i) gramsA.push_back(a.substr(i, n));
    for (size_t i = 0; i <= b.size() - n; ++i) gramsB.push_back(b.substr(i, n));
    std::ranges::sort(gramsA); std::ranges::sort(gramsB);
    int common = 0, j = 0;
    for (const auto& ga : gramsA) {
        while (j < gramsB.size() && gramsB[j] < ga) ++j;
        if (j < gramsB.size() && ga == gramsB[j]) { ++common; ++j; }
    }
    return static_cast<double>(common) / max(gramsA.size(), gramsB.size());
}

//im gonna be honest. i stole this from se webs; i have no idea and dont really care that much to do a good matching algo myself
double fuzzyMatchScore(const std::string& a, const std::string& b) {
    if (a.empty() || b.empty()) return 0.0;

    auto normalize = [](std::string s) {
        std::erase_if(s, ::ispunct);
        std::ranges::transform(s, s.begin(), ::tolower);
        return s;
    };

    std::string na = normalize(a), nb = normalize(b);

    double gramBonus = ngramScore(na, nb);

    auto tokens = [&](const std::string& s) {
        std::vector<std::string> res;
        std::stringstream ss(s);
        std::string word;
        while (ss >> word) {
            std::erase_if(word, ::ispunct);
            std::ranges::transform(word, word.begin(), ::tolower);
            if (word.length() >= 2) res.push_back(word);
        }
        return res;
    };

    auto ta = tokens(na), tb = tokens(nb);
    if (ta.empty() && tb.empty()) return gramBonus;

    std::ranges::sort(ta); std::ranges::sort(tb);

    int matches = 0, total = 0;
    size_t i = 0, j = 0;
    while (i < ta.size() && j < tb.size()) {
        double sim = levenshtein(ta[i], tb[j]);
        if (sim >= 0.6) {  // Fuzzy threshold
            matches++; ++i; ++j;
        } else if (ta[i] < tb[j]) {
            ++i;
        } else {
            ++j;
        }
        total = max(static_cast<int>(i), static_cast<int>(j));
    }

    double tokenScore = ta.empty() || tb.empty() ? 0.0 : static_cast<double>(matches) / total;
    double lenBonus = 1.0 + min(static_cast<double>(na.length()), static_cast<double>(nb.length())) / 100.0;
    double score = (tokenScore * 0.7 + gramBonus * 0.3) * lenBonus;  // Blend scores

    return std::clamp(score, 0.0, 1.0);
}

std::string extractOriginalCase(const std::string& ocrText, const std::string& cleanMatch) {
    size_t pos = ocrText.find(cleanMatch);
    if (pos != std::string::npos) {
        size_t start = pos > 5 ? pos - 5 : 0;
        size_t end = qMin(ocrText.size(), pos + cleanMatch.length() + 5);
        return ocrText.substr(start, end - start);
    }
    return cleanMatch;
}

std::string findBestNameMapMatch(const std::string& candidate) {
    std::string candidateClean = candidate;
    std::ranges::replace_if(candidateClean, [](char c){ return !isalnum(c) && c != ' ' && c != '-'; }, ' ');
    std::ranges::replace(candidateClean, ' ', ' ');

    std::string candidateLower = candidateClean;
    std::ranges::transform(candidateLower, candidateLower.begin(), ::tolower);

    std::string bestId;
    std::string bestName;
    double bestScore = 0.0;

    if (NameMap.empty()) {
        RefreshNameMap();
    }

    for (const auto& [buildId, buildName] : NameMap) {
        std::string name = buildName;
        std::string id = buildId;
        if (id.find("Prime") == std::string::npos) continue;

        std::string nameLower = name;
        std::ranges::transform(nameLower, nameLower.begin(), ::tolower);

        double score = fuzzyMatchScore(candidateLower, nameLower);
        if (score > bestScore) {
            bestScore = score;
            bestId = id;
            bestName = nameLower;
        }
    }
    LogThis("best score is " + std::to_string(bestScore) + " with " + bestName + "(" + bestId + ")");
    return bestId;
}

void BackgroundWorker::parseItemsAndShowOverlay(const std::string &ocrText, int sectionIndex, QRect captureRect) {
    if (ocrText.empty()) {
        LogThis("OCR empty, skipping");
        return;
    }

    std::string cleanText = ocrText;
    std::ranges::transform(cleanText, cleanText.begin(), ::tolower);

    std::string bestMatchId = findBestNameMapMatch(cleanText);

    if (!bestMatchId.empty()) {
        showSectionOverlay(bestMatchId, sectionIndex, captureRect);
    } else {
        LogThis("No primes found in this section");
    }
}

void BackgroundWorker::showSectionOverlay(const std::string &reward, int sectionIndex, QRect captureRect) {
    if (!mainWindow) return;

    LogThis("Showing section overlay " + std::to_string(sectionIndex+1));

    QMetaObject::invokeMethod(mainWindow, [this, reward, captureRect]() {
        const int sectionHeight = captureRect.height();

        const QRect overlayRect(captureRect.x(),
                         captureRect.y() - (sectionHeight * 4),
                         captureRect.width(),
                         sectionHeight);

        QString text;
        auto [items, quantities] = getMainCraftedIdsWithQuantities(reward);
        if (items.empty()) {
            text = QString::fromStdString(nameFromId(reward) + ": nothing found");
        }
        else {
            std::string result = nameFromId(reward) + ":\n";
            for (size_t i = 0; i < items.size(); ++i) {
                const auto& item = items[i];
                const std::string name = item.name;
                const int qty = (i < quantities.size()) ? quantities[i] : 1;
                const int current = item.info.level;
                const int max = item.info.maxLevel;

                result += name + "  (x" + std::to_string(qty) + "): " +
                          std::to_string(current) + "/" + std::to_string(max);

                if (i + 1 < items.size())
                    result += "\n";
            }

            text = QString::fromStdString(result);
        }

        auto* overlay = new CaptureOverlay(QColor(0, 0, 0), QColor(255, 105, 180), text, nullptr);
        connect(overlay, &CaptureOverlay::expired,
            this, [this](CaptureOverlay* o) {
                b_overlays.erase(std::ranges::remove(b_overlays, o).begin(), b_overlays.end());
                o->deleteLater();
            });
        b_overlays.push_back(overlay);
        overlay->setGeometry(overlayRect);
        overlay->raise();
        overlay->show();
    }, Qt::QueuedConnection);
}

bool BackgroundWorker::parseMissionFinish(const std::string& line) {
    return line.find("EndOfMatch.lua: ReturnedToShip") != std::string::npos;
}

bool BackgroundWorker::parseRewardsTrigger(const std::string& line) {
    if (line.find("Created /Lotus/Interface/ProjectionRewardChoice.swf") != std::string::npos) {
        return true;
    }
    return false;
}

void BackgroundWorker::fetchGameData() {
    FetchGameUpdate();
    LogThis("Background game data fetch completed");
}

void BackgroundWorker::triggerUpdate() const {
    if (mainWindow) {
        QMetaObject::invokeMethod(mainWindow, [this]() {
            mainWindow->updatePlayerData();
        }, Qt::QueuedConnection);
    }
}
