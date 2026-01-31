#ifndef DATAREADER_H
#define DATAREADER_H
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json_fwd.hpp>

enum class JsonType {
    Int,
    String,
    Bool,
    Array,
    Object,
    Double,
    Null
};

enum class InventoryCategories : uint32_t {
    None            = 0,
    Prime           = 1 << 0,
    Warframe        = 1 << 1,
    Weapon          = 1 << 2,
    Companion       = 1 << 3,
    Necramech       = 1 << 4,
    Archwing        = 1 << 5,
    Archweapon      = 1 << 6,
    Component       = 1 << 7,
    Adversary         = 1 << 8,
    DojoSpecial     = 1 << 9,
    LandingCraft    = 1 << 10,
    Modular         = 1 << 11,
    Relic           = 1 << 12,
    NoMastery       = 1 << 13,
    Arcane          = 1 << 14,
    Mod             = 1 << 15,
    FounderSpecial  = 1 << 16,
    DucatItem       = 1 << 17
};

struct RankCount {
    int rank = 0;
    int count = 0;
};

struct MasteryInfo {
    int level;
    int maxLevel;
    bool usedHalfAffinity;
};

struct ItemData {
    std::string craftedId{};
    std::string name{};
    MasteryInfo info = {};
    bool mastered = false;
    InventoryCategories category{};

    [[nodiscard]] const std::string& getCraftedId() const { return craftedId; }
    [[nodiscard]] const std::string& getName() const { return name; }
    [[nodiscard]] InventoryCategories getCategory() const { return category; }
    [[nodiscard]] const bool& getMastered() const { return mastered; }

    bool setInfo(const MasteryInfo newInfo) {
        info = newInfo;
        mastered = (newInfo.level == newInfo.maxLevel);
        return true;
    }
};

struct MissionData {
    std::string name;
    std::string tag;
    std::string region;
    int baseXp = 0;
    bool sp = false;
    bool isCompleted = false;
};

struct MissionSummary {
    int totalCount = 0;
    int incompleteCount = 0;
    std::vector<MissionData> missions;
};

struct Intrinsic {
    std::string name;
    int level;
};

struct IntrinsicCategory {
    std::string name;
    std::vector<Intrinsic> skills;
};

static std::map<std::string, std::string> categoryNameMap = {
    {"SPACE", "Railjack"},
    {"DRIFTER", "Duviri"}
};

extern std::unordered_map<std::string, std::string> BpToResultMap;
extern std::unordered_map<std::string, std::string> ResultToBpMap;
static std::unordered_map<std::string, std::vector<std::string>> IngredientToResultMap;
extern std::unordered_map<std::string, std::string> NameMap;
static std::unordered_map<std::string, int> XPMap;
extern int extraSpecialXp;
extern std::vector<ItemData> items;

// Bitwise OR
InventoryCategories operator|(InventoryCategories lhs, InventoryCategories rhs);

// Bitwise AND
InventoryCategories operator&(InventoryCategories lhs, InventoryCategories rhs);

// Check if 'value' has 'category' (strict)
bool hasCategoryStrict(InventoryCategories value, InventoryCategories category);

// Include check: returns true if ANY bits in 'category' are set in 'value'
bool hasCategory(InventoryCategories value, InventoryCategories category);

// Exclude check: returns true if NONE of the bits in 'category' are set in 'value'
bool hasNoCategory(InventoryCategories value, InventoryCategories category);

// Return 'value' with 'category' set
InventoryCategories addCategory(InventoryCategories value, InventoryCategories category);

// Return 'value' with 'category' unset
InventoryCategories unsetCategory(InventoryCategories value, InventoryCategories category);

//game relevant
void RefreshNameMap();
void RefreshResultBpMap();
std::string nameFromId(const std::string& id, bool supressError = false);

//player relevant
void RefreshXPMap();
int GetMasteryRank();
int GetCurrentXP();
MissionSummary GetMissionsSummary();
std::vector<IntrinsicCategory> GetIntrinsics();
std::vector<std::vector<ItemData>> SplitEquipment(std::vector<ItemData> items);
std::vector<ItemData> GetItems();
std::string getHonoria();
std::string getClan();
std::string getName();
std::tuple<std::vector<ItemData>, std::vector<int>> getMainCraftedIdsWithQuantities(const std::string& bpId, int depth = 0);

void exportAsCsv(std::string fileName);

#endif //DATAREADER_H
