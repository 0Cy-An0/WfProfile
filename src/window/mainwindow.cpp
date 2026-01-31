#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QStandardPaths>
#include <QInputDialog>
#include <QGroupBox>
#include <QSpinBox>

#include "mainwindow.h"

#include <QDir>
#include <qguiapplication.h>
#include <qscreen.h>
#include <qscrollbar.h>

#include "CaptureOverlay.h"
#include "overviewPartWidget.h"
#include "apiParser/apiParser.h"
#include "autostart/autostart.h"
#include "dataReader/dataReader.h"
#include "EELogParser/logParser.h"
#include "FileAccess/FileAccess.h"

//TODO: Move somewhere better
bool caseInsensitiveFind(const std::string& str, const std::string& search) {
    if (search.empty()) return true; // empty filter matches anything

    std::string strLower = str;
    std::string searchLower = search;

    std::transform(strLower.begin(), strLower.end(), strLower.begin(), ::tolower);
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

    return strLower.find(searchLower) != std::string::npos;
}

Settings MainWindow::settings;
std::mutex MainWindow::settingsMutex;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumSize(800, 600);

    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    mainLayout = new QHBoxLayout(centralWidget);

    // Left panel: buttons
    auto* buttonPanel = new QWidget(centralWidget);
    auto* buttonLayout = new QVBoxLayout(buttonPanel);
    buttonLayout->setSpacing(0);
    buttonLayout->setContentsMargins(0,0,0,0);

    const QStringList labels = {
        "Profile", "Options"
    };

    for (int i = 0; i < labels.size(); ++i) {
        auto* btn = new QPushButton(labels[i].isEmpty() ? QString("Button %1").arg(i+1) : labels[i], buttonPanel);
        btn->setStyleSheet(btnStyle);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        buttonLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, i]() {
            showContentForIndex(i);
        });
    }

    // Right panel: content structure
    mainLayout->addWidget(buttonPanel, /*stretch*/ 1);

    contentArea = new QStackedWidget(this);

    // also adds overview at index 0
    updateOverview();

    QWidget* optionsPage = createOptionsPage();
    contentArea->addWidget(optionsPage);     // index 1

    backgroundThread = new QThread(this);  // MainWindow owns the thread

    backgroundWorker = new BackgroundWorker(this);

    backgroundWorker->moveToThread(backgroundThread);

    connect(backgroundThread, &QThread::started, backgroundWorker, &BackgroundWorker::startWork);
    connect(backgroundThread, &QThread::finished, backgroundWorker, &QObject::deleteLater);
    connect(backgroundThread, &QThread::finished, backgroundThread, &QObject::deleteLater);
    //i was told this should stop it with destruction of MainWindow but still should be done manually to be sure
    connect(this, &MainWindow::destroyed, backgroundWorker, &BackgroundWorker::stopWork);
    connect(this, &MainWindow::destroyed, backgroundThread, &QThread::quit);

    backgroundThread->start();

    mainLayout->addWidget(contentArea, /*stretch*/ 3);

    showContentForIndex(0);
}

void MainWindow::showContentForIndex(int index)
{
    currentIndex = index;
    switch (index)
    {
        case 0:
            contentArea->setCurrentIndex(0); // Overview
            break;
        case 1:
            contentArea->setCurrentIndex(1); // Options Page
        default:
            break;
    }

    mainLayout->invalidate();
}

int GetXPForRank(int rank) {
    if (rank <= 30) {
        return 2500 * rank * rank;
    }
    int legendaryRank = rank - 30;
    return 2250000 + (147500 * legendaryRank);
}

int GetCumulativeXPForRank(int rank) {
    if (rank <= 30) {
        // Use formula for sum of squares: sum(i^2) = n(n+1)(2n+1)/6
        return 2500 * (rank * (rank + 1) * (2 * rank + 1)) / 6;
    }

    // XP required to reach rank 30
    int xpAt30 = 2500 * 30 * 31 * 61 / 6;
    int legendaryRanks = rank - 30;
    return xpAt30 + legendaryRanks * 147500;
}

QWidget *MainWindow::createOverview(bool noUpdate) {
    auto *mainWidget = new QWidget(this);
    auto *OverviewLayout = new QVBoxLayout(mainWidget);
    OverviewLayout->setContentsMargins(0, 0, 0, 0);
    OverviewLayout->setSpacing(6);

    // main horizontal layout for the header
    auto *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    // header widget
    auto *headerWidget = new QWidget();
    headerWidget->setLayout(headerLayout);

    // ================= LEFT SIDE: HONORIA =================
    QString honoria = getHonoria().data();
    QWidget *honoriaWidget = nullptr;
    if (!honoria.isEmpty()) {
        auto *honoriaLayout = new QHBoxLayout();
        honoriaLayout->setContentsMargins(0, 0, 0, 0);
        honoriaLayout->setSpacing(4);

        auto *honoriaLabel = new QLabel(honoria);
        QFont honFont = honoriaLabel->font();
        honFont.setPointSize(10);
        honoriaLabel->setFont(honFont);

        auto *honoriaImageLabel = new QLabel();
        auto imageData = GetImageByFileOrDownload("/Lotus/Interface/Icons/StoreIcons/MiscItems/GenericHonoriaIcon.png!00_OlNqflNKQ94jQVJJdLI54Q");
        if (imageData && !imageData->empty()) {
            QPixmap pixmap;
            pixmap.loadFromData(imageData->data(), imageData->size());
            pixmap = pixmap.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            honoriaImageLabel->setPixmap(pixmap);
        }
        honoriaImageLabel->setFixedSize(50, 50);

        honoriaLayout->addWidget(honoriaImageLabel);
        honoriaLayout->addWidget(honoriaLabel);

        honoriaWidget = new QWidget();
        honoriaWidget->setLayout(honoriaLayout);
    }

    // ================= CENTER: PLAYER NAME =================
    auto *centerContainer = new QWidget();
    auto *centerVLayout = new QVBoxLayout(centerContainer);
    centerVLayout->setContentsMargins(0, 0, 0, 0);
    centerVLayout->setSpacing(6);

    auto *nameLabel = new QLabel(getName().data());
    nameLabel->setAlignment(Qt::AlignCenter);
    QFont nameFont = nameLabel->font();
    nameFont.setPointSize(24);
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);

    centerVLayout->addWidget(nameLabel, 0, Qt::AlignHCenter);

    // ================= RIGHT SIDE: CLAN =================
    QString clan = getClan().data();
    QWidget *rightContainer = nullptr;

    if (!clan.isEmpty()) {
        rightContainer = new QWidget();
        auto *rightHLayout = new QHBoxLayout(rightContainer);
        rightHLayout->setContentsMargins(0, 0, 0, 0);
        rightHLayout->setSpacing(4);

        auto *ClanImageLabel = new QLabel();
        auto emblemData = GetImageByFileOrDownload("/Lotus/Interface/Icons/StoreIcons/Emblems/QTCC2024Emblem.png!00_UdM+apgPG3poYtNyw2SniQ");
        if (emblemData && !emblemData->empty()) {
            QPixmap emblemPixmap;
            emblemPixmap.loadFromData(emblemData->data(), emblemData->size());
            emblemPixmap = emblemPixmap.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            ClanImageLabel->setPixmap(emblemPixmap);
        }
        ClanImageLabel->setFixedSize(50, 50);

        // text stacked vertically
        auto *rightTextLayout = new QVBoxLayout();
        rightTextLayout->setContentsMargins(0, 0, 0, 0);
        rightTextLayout->setSpacing(2);

        QFont rightFont = nameLabel->font();
        rightFont.setPointSize(10);
        rightFont.setBold(false);

        auto *clanLabel = new QLabel(clan);
        clanLabel->setFont(rightFont);

        rightTextLayout->addWidget(clanLabel, 0, Qt::AlignLeft);

        rightHLayout->addLayout(rightTextLayout);
        rightHLayout->addWidget(ClanImageLabel); // image on outside
    }

    // Measure natural widths
    int leftWidth = honoriaWidget ? honoriaWidget->sizeHint().width() : 0;
    int rightWidth = rightContainer ? rightContainer->sizeHint().width() : 0;
    int widthDiff = qAbs(leftWidth - rightWidth);

    // Left spacer (compensates if right is larger)
    auto *leftSpacerWidget = new QWidget();
    leftSpacerWidget->setStyleSheet("background: transparent;");
    auto *leftSpacerLayout = new QHBoxLayout(leftSpacerWidget);
    leftSpacerLayout->setContentsMargins(0,0,0,0);
    if (rightWidth > leftWidth) {
        leftSpacerLayout->addSpacing(widthDiff);
    }
    leftSpacerWidget->setFixedHeight(50);  // Match side heights

    // Right spacer (compensates if left is larger)
    auto *rightSpacerWidget = new QWidget();
    rightSpacerWidget->setStyleSheet("background: transparent;");
    auto *rightSpacerLayout = new QHBoxLayout(rightSpacerWidget);
    rightSpacerLayout->setContentsMargins(0,0,0,0);
    if (leftWidth > rightWidth) {
        rightSpacerLayout->addSpacing(widthDiff);
    }
    rightSpacerWidget->setFixedHeight(50);

    // ================= ASSEMBLE HEADER LAYOUT =================
    if (honoriaWidget) headerLayout->addWidget(honoriaWidget);
    headerLayout->addWidget(leftSpacerWidget);  // Invisible left compensator
    headerLayout->addStretch();
    headerLayout->addWidget(centerContainer);
    headerLayout->addStretch();
    headerLayout->addWidget(rightSpacerWidget);  // Invisible right compensator
    if (rightContainer) headerLayout->addWidget(rightContainer);



    // Progress bars container: left and right under the name
    auto *progressBarsContainer = new QWidget(mainWidget);
    auto *progressBarsLayout = new QHBoxLayout(progressBarsContainer);
    progressBarsLayout->setContentsMargins(0, 0, 0, 0);
    progressBarsLayout->setSpacing(10);

    // Left progress bar + label: Xp To Mastery Rank {X}
    auto *leftProgressWidget = new QWidget(progressBarsContainer);
    auto *leftProgressLayout = new QVBoxLayout(leftProgressWidget);
    leftProgressLayout->setContentsMargins(0, 0, 0, 0);
    leftProgressLayout->setSpacing(2);

    // Create label
    xpToMasteryLabel = new QLabel(progressBarsContainer);

    // Create progress bar
    xpToMasteryBar = new QProgressBar(mainWidget);
    xpToMasteryBar->setTextVisible(true);
    xpToMasteryBar->setAlignment(Qt::AlignCenter);

    leftProgressLayout->addWidget(xpToMasteryLabel);
    leftProgressLayout->addWidget(xpToMasteryBar);

    // Right progress bar + label: Xp To Max
    auto *rightProgressWidget = new QWidget(progressBarsContainer);
    auto *rightProgressLayout = new QVBoxLayout(rightProgressWidget);
    rightProgressLayout->setContentsMargins(0, 0, 0, 0);
    rightProgressLayout->setSpacing(2);

    auto *xpToMaxLabel = new QLabel("Xp To Max", progressBarsContainer);
    xpToMaxBar = new QProgressBar(progressBarsContainer);
    xpToMaxBar->setTextVisible(true);
    xpToMaxBar->setAlignment(Qt::AlignCenter);

    rightProgressLayout->addWidget(xpToMaxLabel);
    rightProgressLayout->addWidget(xpToMaxBar);

    progressBarsLayout->addWidget(leftProgressWidget);
    progressBarsLayout->addWidget(rightProgressWidget);

    // Three rectangular fields container
    auto* fieldsWidget = new QWidget(mainWidget);
    auto *fieldsLayout = new QHBoxLayout(fieldsWidget);
    fieldsLayout->setContentsMargins(0, 0, 0, 0);
    fieldsLayout->setSpacing(10);

    equipmentField = new OverviewPartWidget(fieldsWidget);
    starChartField = new OverviewPartWidget(fieldsWidget);

    // 1) Collectables: X%
    QVector<QString> sectionHeaders = { "NonPrime", "Prime" };
    QVector<QString> listHeaders   = { "Todo", "Todo" };

    equipmentField->initialize(sectionHeaders, listHeaders);

    // 2) Star Chart: X%
    sectionHeaders   = { "Normal", "Steel Path" };
    listHeaders = { "Todo", "Todo" };

    starChartField->initialize(sectionHeaders, listHeaders);

    // 3) Intrinsics: X%
    // Retrieve the categories
    std::vector<IntrinsicCategory> overviewCategories = GetIntrinsics();

    // clear old data
    sectionHeaders = {};
    listHeaders = {};

    for (const auto &cat : overviewCategories) {
        sectionHeaders.append(QString::fromStdString(cat.name));
        listHeaders.append("Levels");
    };


    fieldsLayout->addWidget(equipmentField);
    fieldsLayout->addWidget(starChartField);

    fieldsLayout->setStretchFactor(equipmentField, 1);
    fieldsLayout->setStretchFactor(starChartField, 1);
    //intrinsics part 2. kinda destroys the order i did think which i a bit but its easier this way
    if (!overviewCategories.empty()) {
        intrinsicsField = new OverviewPartWidget(fieldsWidget);
        intrinsicsField->initialize(sectionHeaders, listHeaders);
        fieldsLayout->addWidget(intrinsicsField);
        fieldsLayout->setStretchFactor(intrinsicsField, 1);
    }

    auto *emptyContent = new QWidget(mainWidget);
    auto *emptyLayout = new QVBoxLayout(emptyContent);
    emptyContent->setLayout(emptyLayout);


    // Create a container widget for the first three widgets
    auto *topContainer = new QWidget(mainWidget); //maybe ill rename it but bottom isnt needed anymore so that was removed
    auto *topLayout = new QVBoxLayout(topContainer);

    topLayout->addWidget(headerWidget);
    topLayout->addWidget(progressBarsContainer);
    topLayout->addWidget(fieldsWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);

    OverviewLayout->addWidget(topContainer);

    //this sets all the numbers
    if (!noUpdate) updateOverview();

    return mainWidget;
}

void MainWindow::updateOverview() {
    //had some trouble with this updating incorrectly before. now it just does a whole new page instead. any create call should now call this instead!
    // create new overview page instead
    QWidget* oldOverview = contentArea->widget(0);
    if (oldOverview) {
        contentArea->removeWidget(oldOverview);
        oldOverview->deleteLater();  // Safe cleanup of old page
    }
    QWidget *newOverview = createOverview(true); //this should be only create call other than maybe the very first im not quite sure about that yet
    contentArea->insertWidget(0, newOverview);  // Replace at index 0
    int currentRank = GetMasteryRank();
    int currentXP = GetCurrentXP();
    int totalXP = extraSpecialXp;
    int xpForNextRank = GetXPForRank(currentRank + 1);
    int xpForCurrentRank = GetXPForRank(currentRank);

    //Update Mastery Text
    xpToMasteryLabel->setText(("XP To Mastery Rank " + std::to_string(currentRank + 1)).c_str());

    // Set the range from 0 to XP needed to next rank
    xpToMasteryBar->setRange(0, xpForNextRank - xpForCurrentRank);

    // Set the current value to the XP progress
    xpToMasteryBar->setValue(currentXP - xpForCurrentRank);

    // Set the text over the bar to show "x / y"
    xpToMasteryBar->setFormat(QString("%1 / %2").arg(currentXP).arg(GetCumulativeXPForRank(currentRank + 1))); //Xp starts from 0 each new mastery rank

    // 1) Collectables: X%
    if (items.empty()) items = GetItems();
    std::vector<std::vector<ItemData>> collectables = SplitEquipment(items);
    QVector<QString> sectionHeaders = { "NonPrime", "Prime" };
    QVector completedValues   = { 0, 0 };
    QVector totalValues       = { 0, 0 };
    QVector<QStringList> sectionLists = { {}, {} };
    QVector<QString> listHeaders   = { "Todo", "Todo" };
    for (int sectionIndex = 0; sectionIndex < collectables.size(); ++sectionIndex) {
        auto &section = collectables[sectionIndex];
        // Stable sort items in this section by info.level
        std::stable_sort(section.begin(), section.end(),
            [](const ItemData &a, const ItemData &b) {
                return a.info.level > b.info.level;
            });
        LogThis("section " + std::to_string(sectionIndex) +  " with " + std::to_string(collectables[sectionIndex].size()) + " items");
        for (const ItemData &item : collectables[sectionIndex]) {

            if (settings.hideFounder && hasCategory(item.category, InventoryCategories::FounderSpecial)) {
                LogThis("Hiding: " + item.name);
                continue;
            }

            totalValues[sectionIndex]++;

            const auto &info = item.info;
            int toAdd = info.maxLevel * (info.usedHalfAffinity ? 100 : 200);
            totalXP += toAdd;
            //LogThis(item.craftedId + " : " + std::to_string(toAdd));
            if (info.level >= info.maxLevel) {
                // Completed
                completedValues[sectionIndex]++;
            } else {
                QString entry = QString::fromStdString(item.name) +
                                " " + QString::number(info.level) +
                                "/" + QString::number(info.maxLevel);
                sectionLists[sectionIndex].append(entry);
            }
        }
        //LogThis("sub total: " + std::to_string(totalXP));
    }
    int totalCompleted = 0;
    int totalOverall = 0;
    for (int c : completedValues) totalCompleted += c;
    for (int t : totalValues) totalOverall += t;

    float combinedCompletion = (totalOverall == 0) ? 0.f : (static_cast<float>(totalCompleted) / static_cast<float>(totalOverall) * 100.f);

    //updateField
    equipmentField->updateData(completedValues, totalValues, sectionLists);
    equipmentField->updateTitle(QString("Equipment: %1%").arg(combinedCompletion, 0, 'f', 1));

    // 2) Star Chart: X%
    MissionSummary summary = GetMissionsSummary();
    // Prepare Normal & Steel Path variables
    int normalCompleted = 0;
    int normalTotal = 0;
    QStringList normalIncomplete;

    int steelCompleted = 0;
    int steelTotal = 0;
    QStringList steelIncomplete;

    for (const auto& m : summary.missions) {
        // Always count for both totals
        normalTotal++;
        steelTotal++;
        totalXP += m.baseXp * 2;

        // Normal path
        if (m.isCompleted) {
            normalCompleted++;
        } else {
            normalIncomplete << QString::fromStdString(m.name);
        }

        // Steel Path
        if (m.sp && m.isCompleted) {
            steelCompleted++;
        }
        if (!m.sp) {
            steelIncomplete << QString::fromStdString(m.name);
        }
    }

    // Combined statistics for the title
    totalCompleted = normalCompleted + steelCompleted;
    totalOverall = normalTotal + steelTotal;
    combinedCompletion = (totalOverall == 0)
        ? 0.f
        : (static_cast<float>(totalCompleted) / static_cast<float>(totalOverall) * 100.f);

    // TODO: maybe split this up further into each region?
    sectionHeaders   = { "Normal", "Steel Path" };
    completedValues  = { normalCompleted, steelCompleted };
    totalValues      = { normalTotal, steelTotal };
    sectionLists = { normalIncomplete, steelIncomplete };
    listHeaders = { "Todo", "Todo" };

    //updateField
    starChartField->updateData(completedValues, totalValues, sectionLists);
    starChartField->updateTitle(QString("Star Chart: %1%").arg(combinedCompletion, 0, 'f', 1));

    // 3) Intrinsics: X%
    // Retrieve the categories
    std::vector<IntrinsicCategory> overviewCategories = GetIntrinsics();

    // clear old data
    sectionHeaders = {};
    completedValues = {};
    totalValues = {};
    sectionLists = {};
    listHeaders= {};

    // Helper lambda to populate one category's section data
    for (const auto &cat : overviewCategories) {
        int completed = 0;
        int total = 0;
        QStringList incomplete;

        for (const auto &skill : cat.skills) {
            incomplete.append(
                QString::fromStdString(skill.name + " " + std::to_string(skill.level) + "/10")
            );
            completed += skill.level;
            total += 10; // each skill is out of 10
            totalXP +=  15000;
        }

        sectionHeaders.append(QString::fromStdString(cat.name));
        completedValues.append(completed);
        totalValues.append(total);
        sectionLists.append(incomplete);
        listHeaders.append("Levels");
    };

    // Calculate combined completion percentage for title
    totalCompleted = 0;
    totalOverall = 0;
    for (int c : completedValues) totalCompleted += c;
    for (int t : totalValues) totalOverall += t;

    combinedCompletion = (totalOverall == 0) ? 0.f : (static_cast<float>(totalCompleted) / static_cast<float>(totalOverall) * 100.f);

    if (intrinsicsField) {
        intrinsicsField->updateData(completedValues, totalValues, sectionLists);
        intrinsicsField->updateTitle(QString("Intrinsics: %1%").arg(combinedCompletion, 0, 'f', 1));
    }


    xpToMaxBar->setRange(0, totalXP);
    LogThis("Total XP possible: " + std::to_string(totalXP));
    xpToMaxBar->setValue(currentXP);
}

//Mutex to be extra safe; i think its actually needed, given the user can modify them at any time
Settings MainWindow::getWindowSettings() {
    std::lock_guard lock(settingsMutex);
    return settings;
}

QWidget* MainWindow::createOptionsPage() {
    auto* page = new QWidget(this);
    auto* gridLayout = new QGridLayout(page);

    // Load settings
    settings = LoadSettings();

    // ---------- Top-Left: Visual Settings ----------
    auto* visualGroup = new QGroupBox("UI / Appearance", page);
    auto* visualLayout = new QVBoxLayout(visualGroup);

    auto founderCheckbox = new QCheckBox("Hide Founder Items", visualGroup);
    founderCheckbox->setChecked(settings.hideFounder);
    visualLayout->addWidget(founderCheckbox);

    connect(founderCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
        settings.hideFounder = checked;
        WriteSettings(settings);
        updateOverview();
    });

    // ---------- Bottom-Left: Data Input ----------
    auto* dataGroup = new QGroupBox("Warframe Profile Data", page);
    auto* dataLayout = new QVBoxLayout(dataGroup);

    auto IdInput = new QLineEdit(dataGroup);
    IdInput->setPlaceholderText("enter profile id...");
    if (!settings.id.empty()) {
        IdInput->setText(settings.id.data());
    }


    auto autoFillBtn = new QPushButton("Auto", dataGroup); //use getLastPlayerId()
    autoFillBtn->setStyleSheet(btnStyle);

    auto gameUpdateBtn = new QPushButton("Update Game Data", dataGroup);
    gameUpdateBtn->setStyleSheet(btnStyle);

    auto* inputLayout = new QHBoxLayout();
    inputLayout->addWidget(IdInput);
    inputLayout->addWidget(autoFillBtn);
    dataLayout->addLayout(inputLayout);
    dataLayout->addWidget(gameUpdateBtn);

    connect(gameUpdateBtn, &QPushButton::clicked, [this]() {
        updateGameData();
    });

    connect(IdInput, &QLineEdit::textChanged, this, [=](const QString& val) {
        settings.id = val.toStdString();
        WriteSettings(settings);
    });

    connect(autoFillBtn, &QPushButton::clicked, this, [=] {
        settings.id = getLastPlayerId(true);
        IdInput->setText(settings.id.data());
    });

    // ---------- Bottom-Right: Automation ----------
    auto* automationGroup = new QGroupBox("Automation / Startup", page);
    auto* automationLayout = new QVBoxLayout(automationGroup);

    auto autoStartCheckbox = new QCheckBox("Start with System", automationGroup);
    autoStartCheckbox->setChecked(settings.startWithSystem);

    auto autoSyncCheckbox = new QCheckBox("Enable Auto Inventory Sync", automationGroup);
    autoSyncCheckbox->setChecked(settings.autoSync);

    auto syncOnMissionFinishCheckbox = new QCheckBox("Sync Inventory on mission finish", automationGroup);
    syncOnMissionFinishCheckbox->setChecked(settings.syncOnMissionFinish);

    auto PrimeMasteryOverlay = new QCheckBox("Show stats for Main prime part above relic rewards", automationGroup);
    PrimeMasteryOverlay->setChecked(settings.relicOverlay);

    auto manualSyncBtn = new QPushButton("Sync inventory", dataGroup);

    auto intervalLabel = new QLabel("Auto-Sync Interval (minutes):", automationGroup);
    auto intervalSpinBox = new QSpinBox(automationGroup);
    intervalSpinBox->setRange(1, 120);
    intervalSpinBox->setValue(settings.syncTime);

    auto captureLayout = new QHBoxLayout();
    auto topLeftLabel = new QLabel("Top Left (x,y):");
    auto topLeftEdit = new QLineEdit(QString::number(settings.captureTopLeft.x()) + "," + QString::number(settings.captureTopLeft.y()));


    auto bottomRightLabel = new QLabel("Bottom Right (x,y):");
    auto bottomRightEdit = new QLineEdit(QString::number(settings.captureBottomRight.x()) + "," + QString::number(settings.captureBottomRight.y()));



    auto sections = new QLabel("Number of rewards expected:", automationGroup);
    auto sectionsBox = new QSpinBox(automationGroup);
    sectionsBox->setRange(1, 4);
    sectionsBox->setValue(settings.sections);

    auto testCaptureBtn = new QPushButton("Press to Show Capture screen (30s overlay)");
    captureLayout->addWidget(topLeftLabel);
    captureLayout->addWidget(topLeftEdit);
    captureLayout->addWidget(bottomRightLabel);
    captureLayout->addWidget(bottomRightEdit);

    automationLayout->addWidget(autoStartCheckbox);
    automationLayout->addWidget(autoSyncCheckbox);
    automationLayout->addWidget(syncOnMissionFinishCheckbox);
    automationLayout->addWidget(manualSyncBtn);
    automationLayout->addWidget(PrimeMasteryOverlay);
    automationLayout->addLayout(captureLayout);
    automationLayout->addWidget(sections);
    automationLayout->addWidget(sectionsBox);
    automationLayout->addWidget(testCaptureBtn);
    automationLayout->addWidget(intervalLabel);
    automationLayout->addWidget(intervalSpinBox);

    connect(autoStartCheckbox, &QCheckBox::toggled, this, [=](bool checked) {
        settings.startWithSystem = checked;
        WriteSettings(settings);
        bool success = setAutoStart(checked);
        if (success) {
            LogThis(std::string("Successfully ") + (checked ? "enabled" : "disabled") + " autostart");
        }
        else {
            LogThis(std::string("Tried to ") + (checked ? "enable" : "disable") + " autostart and failed");
            // Revert the checkbox state without triggering the toggled signal again
            QSignalBlocker blocker(autoStartCheckbox);
            autoStartCheckbox->setChecked(!checked);
        }
    });

    connect(autoSyncCheckbox, &QCheckBox::toggled, this, [=](bool checked) {
        settings.autoSync = checked;
        WriteSettings(settings);
    });

    connect(syncOnMissionFinishCheckbox, &QCheckBox::toggled, this, [=](bool checked) {
        settings.syncOnMissionFinish = checked;
        WriteSettings(settings);
    });

    connect(intervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int val) {
        settings.syncTime = val;
        WriteSettings(settings);
    });

    connect(PrimeMasteryOverlay, &QCheckBox::toggled, this, [=](bool checked) {
        settings.relicOverlay = checked;
        WriteSettings(settings);
    });

    connect(topLeftEdit, &QLineEdit::textChanged, this, [=](const QString& text) {
        QStringList parts = text.split(",");
        if (parts.size() == 2) {
            bool xOk, yOk;
            int x = parts[0].trimmed().toInt(&xOk);
            int y = parts[1].trimmed().toInt(&yOk);
            if (xOk && yOk) {
                settings.captureTopLeft = QPoint(x, y);
                WriteSettings(settings);
                LogThis("Updated bottom-left: " + std::to_string(x) + "," + std::to_string(y));
            }
        }
    });

    connect(bottomRightEdit, &QLineEdit::textChanged, this, [=](const QString& text) {
        QStringList parts = text.split(",");
        if (parts.size() == 2) {
            bool xOk, yOk;
            int x = parts[0].trimmed().toInt(&xOk);
            int y = parts[1].trimmed().toInt(&yOk);
            if (xOk && yOk) {
                settings.captureBottomRight = QPoint(x, y);
                WriteSettings(settings);
                LogThis("Updated top-right: " + std::to_string(x) + "," + std::to_string(y));
            }
        }
    });

    connect(sectionsBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int val) {
        settings.sections = val;
        WriteSettings(settings);
    });

    connect(testCaptureBtn, &QPushButton::pressed, this, [this]() {
        LogThis("TestButtonPressed");
        //TODO: remember to put background workers overlays in an array too (with early exit)

        QRect rect(settings.captureTopLeft, settings.captureBottomRight);

        for (CaptureOverlay* overlay : m_overlays) {
            if (overlay) overlay->deleteLater();
        }
        m_overlays.clear();

        int sections = settings.sections;
        if (sections < 1) {
            LogThis("number of sections below 1");
            return;
        }

        int sectionWidth = rect.width() / sections;
        int sectionHeight = rect.height();

        for (int i = 0; i < sections; ++i) {
            QRect sectionRect(
                rect.x() + i * sectionWidth,
                rect.y(),
                sectionWidth,
                sectionHeight
            );

            auto* overlay = new CaptureOverlay(nullptr);
            connect(overlay, &CaptureOverlay::expired,
            this, [this](CaptureOverlay* o) {
                m_overlays.erase(std::ranges::remove(m_overlays, o).begin(), m_overlays.end());
                o->deleteLater();
            });
            overlay->setGeometry(sectionRect);
            overlay->raise();
            overlay->show();

            m_overlays.push_back(overlay);
            LogThis("Created overlay " + std::to_string(i+1) +
                    " at x=" + std::to_string(sectionRect.x()));
        }
    });

    connect(manualSyncBtn, &QPushButton::clicked, this, &MainWindow::updatePlayerData);

    // ---------- Top-Right: Export ----------
    auto* exportGroup = new QGroupBox("Export", page);
    auto* exportLayout = new QVBoxLayout(exportGroup);

    auto* exportBtn = new QPushButton("Export...", exportGroup);
    exportBtn->setStyleSheet(btnStyle);
    exportLayout->addWidget(exportBtn);
    exportLayout->addStretch();

    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::exportPlayer);

    // ---------- Final Layout Assembly ----------
    gridLayout->addWidget(visualGroup, 0, 0);
    gridLayout->addWidget(exportGroup, 0, 1);
    gridLayout->addWidget(dataGroup, 1, 0);
    gridLayout->addWidget(automationGroup, 1, 1);

    return page;
}

void MainWindow::exportPlayer() {
    QString filter = tr("JSON Files (*.json);;"
                        "csv (*.csv);;"
                        "All Files (*.*)");

    QString selectedFilter;
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Export Data"),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        filter,
        &selectedFilter,
        QFileDialog::Options()
    );

    if (fileName.isEmpty())
        return;

    // Determine format from selected filter
    enum class ExportFormat { Json, Csv, Unknown };
    ExportFormat format = ExportFormat::Unknown;

    if (selectedFilter.startsWith("JSON"))
        format = ExportFormat::Json;
    else if (selectedFilter.startsWith("csv"))
        format = ExportFormat::Csv;

    // Ensure proper extension if user omitted it
    auto ensureSuffix = [](QString& name, const QString& ext) {
        if (!name.endsWith(ext, Qt::CaseInsensitive))
            name += ext;
    };

    switch (format) {
        case ExportFormat::Json:
            ensureSuffix(fileName, ".json");
            exportAsJson(fileName.toStdString());
            break;
        case ExportFormat::Csv:
            ensureSuffix(fileName, ".csv");
            exportAsCsv(fileName.toStdString());
            break;
        default:
            ensureSuffix(fileName, ".json");
            exportAsJson(fileName.toStdString());
            break;
    }
}

void MainWindow::updateGameData() {
    if (backgroundWorker) {
        QMetaObject::invokeMethod(backgroundWorker, [this]() {
            FetchGameUpdate();
            // Notify main thread when background work is done
            QMetaObject::invokeMethod(this, [this]() {
                RefreshNameMap();
            }, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
    } else {
        // Fallback if no background worker
        FetchGameUpdate();
        RefreshNameMap();
    }
}

void MainWindow::updatePlayerData(bool skipFetch) {
    LogThis("Updating data");
    if (!skipFetch) {
        bool success = UpdatePlayerData(settings.id); //TODO: maybe do something on fail idk put a warning banner up
        if (!success) {
            LogThis("player data failed to download; check if id is set correctly");
        }
    }
    RefreshXPMap();

    updateOverview();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    for (CaptureOverlay* overlay : m_overlays) {
        if (overlay) {
            overlay->earlyTimeout();
            overlay->setParent(this); //for some reason not doing this and qDelete will leave exactly 1 of the potential 4 overlays just hanging there. I assume i could skip early timeout with this as QT should handle the deletion now, but maybe not.
        }
    }
    qDeleteAll(m_overlays);
    m_overlays.clear();

    if (backgroundWorker) {
        backgroundWorker->stopWork();
    }

    event->accept();
}

MainWindow::~MainWindow() {
    qDeleteAll(m_overlays);
    m_overlays.clear();
    if (backgroundWorker) {
        backgroundWorker->stopWork();
    }
    if (backgroundThread) {
        backgroundThread->quit();
        backgroundThread->wait();
    }
}
