#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScrollArea>
#include <QStackedWidget>
#include <QProgressBar> //this is used even if the IDE tells you its not

#include "overviewPartWidget.h"
#include "background/BackgroundWorker.h"
#include "dataReader/dataReader.h"
#include "FileAccess/FileAccess.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    static Settings getWindowSettings();
    void updatePlayerData(bool skipFetch = false);

    ~MainWindow() override;

private:
    BackgroundWorker* backgroundWorker = nullptr;
    QThread* backgroundThread = nullptr;

    static std::mutex settingsMutex;
    static Settings settings;

    QWidget* contentField = nullptr;
    QStackedWidget *contentArea = nullptr;
    QHBoxLayout *mainLayout = nullptr;

    QLabel* xpToMasteryLabel = nullptr;
    QProgressBar* xpToMasteryBar = nullptr;
    QProgressBar* xpToMaxBar = nullptr;
    OverviewPartWidget* equipmentField = nullptr;
    OverviewPartWidget* starChartField = nullptr;
    OverviewPartWidget* intrinsicsField = nullptr;

    int currentIndex = 0;

    std::vector<ItemData> items; //this is important otherwise these get deleted after creation and all filtering etc. causes invalid access violation
    //the above comment might he invalid now that filtering doesnt exist anymore; but i wouldnt touch it

    QString btnStyle = R"(
        QPushButton {
            background: #333333;
            color: white;
            border: none;
            border-radius: 0px;
            padding: 0px;
            min-height: 48px;
        }
        QPushButton:hover { background: #222222; }
    )";

    /*static QWidget* createField(const QString& title,
                                const QVector<QString>& sectionHeaders,
                                const QVector<int>& completedValues,
                                const QVector<int>& totalValues,
                                const QVector<QStringList>& incompleteLists,
                                const QVector<QString>& incompleteHeaders, QWidget *parent);

    static QWidget* createSectionWidget(const QString& sectionHeader,
                                        int completed,
                                        int total,
                                        const QStringList& list,
                                        const QString& listHeader, QWidget* parent);*/

    QWidget *createOverview(bool noUpdate = false);

    void updateOverview();

    void showContentForIndex(int index);

    QWidget *createOptionsPage();

    void exportPlayer();

protected:
    void updateGameData();

};
#endif // MAINWINDOW_H
