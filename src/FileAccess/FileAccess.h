#ifndef FILEACCESS_H
#define FILEACCESS_H
#include <ios>
#include <qpoint.h>
#include <string>
#include <nlohmann/json.hpp>

enum class DataType {
    Warframes,
    Blueprints,
    Customs,
    Drones,
    Flavour,
    FusionsBundles,
    Gear,
    Keys,
    Images,
    Regions,
    Relics,
    Resources,
    Sentinels,
    SortieRewards,
    Mods,
    Weapons,
    Player,
    Nodes,
    Enemies
};

struct Settings {
    bool hideFounder = true;
    bool startWithSystem = false;
    bool autoSync = false;
    int syncTime = 10;
    bool syncOnMissionFinish = false;
    QPoint captureTopLeft{475, 400};
    QPoint captureBottomRight{1440, 460};
    int sections = 4;
    bool relicOverlay = false;
    std::string id{};
};

static const std::string logPath = "../data/Log/Log.txt";
static const std::string settingPath = "../data/Settings/Settings.cfg";
static const std::string downloadPath = "../data/Download/";
void LogThis(const std::string& str, bool ThisIsNecessaryDontTouchThis = false);
bool SaveDataAt(const std::string& data, const std::string& path, std::ios_base::openmode mode = std::ios::out);
std::shared_ptr<const std::vector<uint8_t>> GetImageByFileOrDownload(const std::string& image);
Settings LoadSettings();
void WriteSettings(const Settings& settings);
bool SettingsFileExists();

nlohmann::json ReadData(DataType type);
nlohmann::ordered_json ReadDataOrdered(DataType type);

void exportAsJson(std::string fileName);

#endif //FILEACCESS_H
