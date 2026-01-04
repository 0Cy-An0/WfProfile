#include "dataReader.h"

#include <fstream>
#include <regex>
#include <vector>
#include <nlohmann/json.hpp>
#include <type_traits>
#include <unordered_set>

#include "FileAccess/FileAccess.h"



using json = nlohmann::json;

InventoryCategories operator|(InventoryCategories lhs, InventoryCategories rhs) {
    using U = std::underlying_type_t<InventoryCategories>;
    return static_cast<InventoryCategories>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

InventoryCategories operator&(InventoryCategories lhs, InventoryCategories rhs) {
    using U = std::underlying_type_t<InventoryCategories>;
    return static_cast<InventoryCategories>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

bool hasCategoryStrict(InventoryCategories value, InventoryCategories category) {
    using U = std::underlying_type_t<InventoryCategories>;
    return (static_cast<U>(value) & static_cast<U>(category)) == static_cast<U>(category);
}

bool hasCategory(InventoryCategories value, InventoryCategories category) {
    using U = std::underlying_type_t<InventoryCategories>;
    return (static_cast<U>(value) & static_cast<U>(category)) != 0;
}

bool hasNoCategory(InventoryCategories value, InventoryCategories category) {
    using U = std::underlying_type_t<InventoryCategories>;
    return (static_cast<U>(value) & static_cast<U>(category)) == 0;
}

InventoryCategories addCategory(InventoryCategories value, InventoryCategories category) {
    return value | category;
}

InventoryCategories unsetCategory(InventoryCategories value, InventoryCategories category) {
    using U = std::underlying_type_t<InventoryCategories>;
    return static_cast<InventoryCategories>(static_cast<U>(value) & ~static_cast<U>(category));
}

template<typename ReturnType, typename JsonType = nlohmann::json>
ReturnType getValueByKey(const JsonType& jsonData,
                         const std::string& key,
                         const ReturnType& default_value = ReturnType{},
                         bool suppressError = false)
{
    //LogThis("made it to getValueByKey");
    if (!jsonData.contains(key)) {
        if (!suppressError) LogThis("Key not found: " + key);
        return default_value;
    }
    //LogThis("if?");
    try {
        return jsonData.at(key).template get<ReturnType>();
    }
    catch (const std::exception& e) {
        LogThis("Exception getting key '" + key + "' : " + e.what());
        return default_value;
    }
}

std::string nameFromId(const std::string& id, bool supressError = false) {
    if (NameMap.empty()) {
        RefreshNameMap();
    }

    if (auto nameIt = NameMap.find(id); nameIt != NameMap.end()) {
        return nameIt->second;
    }
    if (!supressError) {
        LogThis("Name not found! check: " + id);
    }

    return id;
}

InventoryCategories GetItemCategoryFromId(const std::string& id) {
    InventoryCategories category = InventoryCategories::None;

    // Prime
    if (id.find("Prime") != std::string::npos) {
        category = addCategory(category, InventoryCategories::Prime);
        //special founder exclusives
        if (id.find("/Lotus/Powersuits/Excalibur/ExcaliburPrime") != std::string::npos ||
        id.find("/Lotus/Weapons/Tenno/Pistol/LatoPrime") != std::string::npos ||
        id.find("/Lotus/Weapons/Tenno/Melee/LongSword/SkanaPrime") != std::string::npos) {
            category = addCategory(category, InventoryCategories::FounderSpecial);
            return category;
        }
    }

    // Warframe & related
    if (id.find("/Lotus/Powersuits/") != std::string::npos) {
        category = addCategory(category, InventoryCategories::Warframe);
    }

    // Weapon & components
    if (id.find("Weapon") != std::string::npos && !hasCategory(category, InventoryCategories::Warframe)) {
        // Lich Adversary
        if (id.find("KuvaLich") != std::string::npos) {
            category = addCategory(category, InventoryCategories::Adversary);
        }
        //Kuva 2nd try
        else if (nameFromId(id, true).rfind("Kuva", 0) == 0) {
            category = addCategory(category, InventoryCategories::Adversary);
        }
        // Sister Adversary
        else if (id.find("BoardExec") != std::string::npos) {
            category = addCategory(category, InventoryCategories::Adversary);
        }
        //Tenet 2nd try
        else if (nameFromId(id, true).rfind("Tenet", 0) == 0) {
            category = addCategory(category, InventoryCategories::Adversary);
        }
        // Coda Adversary
        else if (id.find("InfestedLich") != std::string::npos) {
            category = addCategory(category, InventoryCategories::Adversary);
        }
        //Sold by Baro Ki'Teer
        else if (id.find("VoidTrader") != std::string::npos) {
            category = addCategory(category, InventoryCategories::DucatItem);
        }
        //just a weapon
        category = addCategory(category, InventoryCategories::Weapon);
    }

    // WeaponParts as Component
    if (id.find("WeaponParts") != std::string::npos) {
        category = addCategory(category, InventoryCategories::Component | InventoryCategories::Weapon | InventoryCategories::NoMastery);
    }

    //catch all for Resources
    if (id.find("Resources") != std::string::npos) {
        category = addCategory(category, InventoryCategories::Component | InventoryCategories::NoMastery);
    }

    // Companion
    if (id.find("Pet") != std::string::npos || id.find("Sentinel") != std::string::npos) {
        category = addCategory(category, InventoryCategories::Companion);
    }

    // Necramech
    if (id.find("Mech") != std::string::npos || id.find("ThanoTech") != std::string::npos || id.find("NechroTech") != std::string::npos) {
        category = addCategory(category, InventoryCategories::Necramech);
    }

    // Archwing and Archweapon
    if (id.find("Archwing") != std::string::npos) {
        if (hasCategory(category, InventoryCategories::Weapon)) {
            category = addCategory(category, InventoryCategories::Archweapon);
        } else {
            category = addCategory(category, InventoryCategories::Archwing);
        }
    }

    // DojoSpecial = Research
    if (id.find("DojoSpecial") != std::string::npos || id.find("Research") != std::string::npos) {
        category = addCategory(category, InventoryCategories::DojoSpecial | InventoryCategories::NoMastery);
    }

    // LandingCraft & Ships
    if (id.find("LandingCraft") != std::string::npos || id.find("Ships") != std::string::npos) {
        category = addCategory(category, InventoryCategories::LandingCraft | InventoryCategories::NoMastery);
    }

    // Modular (for some reason the spectra Vandal is under 'CorpusModularPistol')
    if (id.find("Hoverboard") != std::string::npos || (id.find("Modular") != std::string::npos && id.find("Vandal") == std::string::npos)) {
        category = addCategory(category, InventoryCategories::Modular);
    }

    return category;
}

std::unordered_map<std::string, std::string> extractMapFromArray(
    const json& j,
    const std::string& arrayKey,
    const std::string& valueKey = "name", const std::string& idKey = "uniqueName")
{
    std::unordered_map<std::string, std::string> result;

    // Check if arrayKey exists and is an array
    if (!j.contains(arrayKey) || !j[arrayKey].is_array()) return result;

    const auto& arr = j[arrayKey];

    for (const auto& entry : arr) {
        std::string id = entry.value(idKey, "");
        std::string val = entry.value(valueKey, "");
        if (!id.empty()) {
            result[id] = val;
        }
    }

    return result;
}

MasteryInfo GetMasteryLevelForItem(const std::string& itemId, int Affinity) {
    InventoryCategories  cat = GetItemCategoryFromId(itemId);
    bool isWeapon = hasCategory(cat, InventoryCategories::Weapon);
    bool isOver40Capable = itemId.find("BallasSwordWeapon") != std::string::npos || hasCategory(cat, InventoryCategories::Adversary) || (hasCategory(cat, InventoryCategories::Necramech) && !isWeapon);
    //BallasSwordWeapon = Paracesis

    int maxRank = isOver40Capable ? 40 : 30;

    auto xpForRank = [](int rank) -> int {
        return 1000 * rank * rank;
    };

    int level = 0;
    for (int rank = 1; rank <= maxRank; ++rank) {
        int threshold = xpForRank(rank);
        // Halve the threshold for weapons
        if (Affinity >= (isWeapon ? threshold / 2 : threshold)) {
            level = rank;

        } else {
            break;
        }
    }

    if (level > maxRank) level = maxRank;

    return MasteryInfo{level, maxRank, isWeapon};
}

std::unordered_map<std::string, std::string> extractWarframeAbilities(
    const json& j, const std::string& warframeArrayKey = "ExportWarframes")
{
    std::unordered_map<std::string, std::string> result;

    if (!j.contains(warframeArrayKey) || !j[warframeArrayKey].is_array()) {
        return result;
    }

    const auto& warframes = j[warframeArrayKey];

    for (const auto& warframe : warframes) {
        // Extract abilities from this warframe
        if (warframe.contains("abilities") && warframe["abilities"].is_array()) {
            const auto& abilities = warframe["abilities"];
            for (const auto& ability : abilities) {
                std::string id = ability.value("abilityUniqueName", "");
                std::string name = ability.value("abilityName", "");
                if (!id.empty() && !name.empty()) {
                    result[id] = name;
                }
            }
        }
    }

    return result;
}

void RefreshNameMap() {
    NameMap.clear();
    const nlohmann::json frames = ReadData(DataType::Warframes);
    const nlohmann::json weapons = ReadData(DataType::Weapons);
    const nlohmann::json companions = ReadData(DataType::Sentinels);
    const nlohmann::json resources = ReadData(DataType::Resources);
    const nlohmann::json custom = ReadData(DataType::Customs);
    const nlohmann::json regions = ReadData(DataType::Regions);
    const nlohmann::json enemies = ReadData(DataType::Enemies);

    std::unordered_map<std::string, std::string> idToWarframeName = extractMapFromArray(frames, "ExportWarframes");
    LogThis("Loaded " + std::to_string(idToWarframeName.size()) + " warframe entries.");
    NameMap.insert(idToWarframeName.begin(), idToWarframeName.end());

    std::unordered_map<std::string, std::string> idToAbilityName = extractMapFromArray(frames, "ExportAbilities", "abilityName", "abilityUniqueName");
    LogThis("Loaded " + std::to_string(idToAbilityName.size()) + " ability entries.");
    NameMap.insert(idToAbilityName.begin(), idToAbilityName.end());

    std::unordered_map<std::string, std::string> idToWarframeAbilityName = extractWarframeAbilities(frames);
    LogThis("Loaded " + std::to_string(idToWarframeAbilityName.size()) + " warframe ability entries.");
    NameMap.insert(idToWarframeAbilityName.begin(), idToWarframeAbilityName.end());

    std::unordered_map<std::string, std::string> idToWeaponName = extractMapFromArray(weapons, "ExportWeapons");
    LogThis("Loaded " + std::to_string(idToWeaponName.size()) + " weapon entries.");
    NameMap.insert(idToWeaponName.begin(), idToWeaponName.end());

    std::unordered_map<std::string, std::string> idToCompanionName = extractMapFromArray(companions, "ExportSentinels");
    LogThis("Loaded " + std::to_string(idToCompanionName.size()) + " companion entries.");
    NameMap.insert(idToCompanionName.begin(), idToCompanionName.end());

    std::unordered_map<std::string, std::string> idToComponentName = extractMapFromArray(resources, "ExportResources");
    LogThis("Loaded " + std::to_string(idToComponentName.size()) + " component entries.");
    NameMap.insert(idToComponentName.begin(), idToComponentName.end());

    std::unordered_map<std::string, std::string> idToCustomName = extractMapFromArray(custom, "ExportCustoms");
    LogThis("Loaded " + std::to_string(idToCustomName.size()) + " custom entries.");
    NameMap.insert(idToCustomName.begin(), idToCustomName.end());

    std::unordered_map<std::string, std::string> idToNodeName = extractMapFromArray(regions, "ExportRegions");
    LogThis("Loaded " + std::to_string(idToNodeName.size()) + " Node entries.");
    NameMap.insert(idToNodeName.begin(), idToNodeName.end());

    std::unordered_map<std::string, std::string> idToEnemyName = enemies;
    LogThis("Loaded " + std::to_string(idToEnemyName.size()) + " Enemy entries.");
    NameMap.insert(idToEnemyName.begin(), idToEnemyName.end());
}

void RefreshXPMap() {
    XPMap.clear();

    nlohmann::json player = ReadData(DataType::Player);
    auto result = getValueByKey<nlohmann::json>(player, "Results", nlohmann::json::array({{}}))[0];
    auto loadoutInventory = getValueByKey<nlohmann::json>(result, "LoadOutInventory", nlohmann::json::object());
    const auto& xpArray = getValueByKey<nlohmann::json>(loadoutInventory, "XPInfo", nlohmann::json::array());

    for (const auto& entry : xpArray) {
        std::string id = entry.value("ItemType", "");
        int xp = entry.value("XP", 0);  // default to 0 if XP is missing

        if (!id.empty()) {
            XPMap[id] = xp;
        }
    }

    LogThis("Refreshed XPMap with " + std::to_string(XPMap.size()) + " entries.");
}

int XPFromId(const std::string& id) {
    if (XPMap.empty()) {
        RefreshXPMap();
    }

    if (auto it = XPMap.find(id); it != XPMap.end()) {
        return it->second;
    }

    return 0;
}

std::string replaceLast(const std::string& str, const std::string& from, const std::string& to) {
    size_t pos = str.rfind(from);
    if (pos == std::string::npos) return str;
    return str.substr(0, pos) + to + str.substr(pos + from.length());
};

void UpdateMastery(ItemData& item) {
    int xpValue = XPFromId(item.getCraftedId());
    MasteryInfo info{};

    if (xpValue > 0) {
        info = GetMasteryLevelForItem(item.getCraftedId(), xpValue);
    } else {
        if (!hasCategory(item.getCategory(), InventoryCategories::NoMastery)) {
            info = GetMasteryLevelForItem(item.getCraftedId(), 0);
        }
    }

    item.setInfo(info);
}

const char* CategoryToString(InventoryCategories cat) {
    switch(cat) {
        case InventoryCategories::Prime: return "Prime";
        case InventoryCategories::Warframe: return "Warframe";
        case InventoryCategories::Weapon: return "Weapon";
        case InventoryCategories::Companion: return "Companion";
        case InventoryCategories::Necramech: return "Necramech";
        case InventoryCategories::Archwing: return "Archwing";
        case InventoryCategories::Archweapon: return "Archweapon";
        case InventoryCategories::Component: return "Component";
        case InventoryCategories::Adversary: return "Adversary";
        case InventoryCategories::DojoSpecial: return "DojoSpecial";
        case InventoryCategories::LandingCraft: return "LandingCraft";
        case InventoryCategories::Modular: return "Modular";
        case InventoryCategories::Relic: return "Relic";
        case InventoryCategories::NoMastery: return "NoMastery";
        case InventoryCategories::Arcane: return "Arcane";
        case InventoryCategories::Mod: return "Mod";
        case InventoryCategories::FounderSpecial: return "FounderSpecial";
        case InventoryCategories::DucatItem: return "DucatItem";
        default: return "Unknown";
    }
}

std::vector<ItemData> GetItems()
{
    std::vector<ItemData> items;

    LogThis("called GetItems()");

    const auto frames = getValueByKey<nlohmann::json>(ReadData(DataType::Warframes), "ExportWarframes");
    const auto weapons = getValueByKey<nlohmann::json>(ReadData(DataType::Weapons), "ExportWeapons");
    const auto companions = getValueByKey<nlohmann::json>(ReadData(DataType::Sentinels), "ExportSentinels");
    // Prepare a combined list of all items from the three datasets
    std::vector allItems = { &frames, &weapons, &companions };

    ItemData item{};
    std::unordered_set<std::string> seenIds;

    for (const auto* dataset : allItems)
    {
        for (const auto& itemFromAll : *dataset)
        {
            if (!itemFromAll.contains("uniqueName") || !itemFromAll["uniqueName"].is_string()) {
                LogThis("Skipping item with no valid uniqueName: " + itemFromAll.dump());
                continue;
            }

            std::string craftedId = itemFromAll["uniqueName"];

            if (itemFromAll["productCategory"] == "SpecialItems" && craftedId.find("Kavat") == std::string::npos) { //skips exalted weapons and such
                continue; //For some reason the khora kavat does count for mr
            }

            if (seenIds.count(craftedId) > 0) {
                LogThis("Duplicate id detected, skipping: " + craftedId);
                continue;
            }

            seenIds.insert(craftedId);

            if (craftedId.find("Pet") != std::string::npos &&
                craftedId.find("Head") == std::string::npos &&
                craftedId.find("Weapon") == std::string::npos &&
                craftedId.find("PowerSuit") == std::string::npos) { //for some reason antigens/mutagens are in here; making sure not to filter out moa head parts and only head
                continue;
            }

            //Vulpaphyla's and Predasite's are in here 2 times
            if (craftedId.find("WoundedInfested") != std::string::npos) {
                continue;
            }

            //filters out some zaw duplicates?
            if (craftedId.find("PvPVariant") != std::string::npos) {
                continue;
            }

            //filters out what i can only assume to be Noctua summoned via dante's ability
            if (craftedId.find("Doppelganger") != std::string::npos) {
                continue;
            }
            //filters out all amps (excluding the Mr relevant part)
            if (craftedId.find("OperatorAmplifiers") != std::string::npos && craftedId.find("Barrel") == std::string::npos) {
                continue;
            }

            // Filters out Zaws
            if (craftedId.find("ModularMelee") != std::string::npos && craftedId.find("Tip") == std::string::npos) {
                continue;
            }

            // Filters out kitguns
            if (craftedId.find("SUModular") != std::string::npos && craftedId.find("Barrel") == std::string::npos) {
                continue;
            }

            // Filters out infested Kitguns
            if (craftedId.find("InfKitGun") != std::string::npos && craftedId.find("Barrel") == std::string::npos) {
                continue;
            }

            // Filters out k-drives
            if (craftedId.find("HoverboardParts") != std::string::npos && craftedId.find("Deck") == std::string::npos) {
                continue;
            }

            // Setup main item
            item.craftedId = craftedId;

            item.name = nameFromId(craftedId, true);
            if (item.name.compare(0, 11, "<ARCHWING> ") == 0) {
                item.name.erase(0, 11);
            }
            // Skip if name is same as id (means no name found)
            if (item.name == craftedId) {
                LogThis("Did not find Name of Item: " + itemFromAll.dump());
                continue;
            }

            item.category = GetItemCategoryFromId(craftedId);

            UpdateMastery(item);

            items.push_back(item);
        }
    }

    LogThis("created " + std::to_string(items.size()) + " Items.");

    return items;
}

int GetMasteryRank() {
    LogThis("called GetName");
    nlohmann::json player = ReadData(DataType::Player);
    LogThis("Parsed Player Json successfully");
    auto result = getValueByKey<nlohmann::json>(player, "Results", nlohmann::json::array({{}}))[0];
    return getValueByKey<int>(result, "PlayerLevel", 0);
}

std::string toTitleCase(const std::string& str) {
    std::string out = str;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    if (!out.empty()) out[0] = static_cast<char>(::toupper(out[0]));
    return out;
}

std::vector<std::vector<ItemData>> SplitEquipment(std::vector<ItemData> items)
{
    LogThis("Called SplitEquipment");
    std::vector<ItemData> primeItems;
    std::vector<ItemData> normalItems;;

    for (const auto& item : items) {
        if (item.info.maxLevel == 0) {
            continue; // skip non mastery relevant items
        }

        // Split into prime vs normal; toTitleCase only capitalizes the very first character which is never part of 'prime'
        if (toTitleCase(item.craftedId).find("prime") != std::string::npos) {
            primeItems.push_back(item);
        } else {
            normalItems.push_back(item);
        }
    }

    return { normalItems, primeItems };
}

std::vector<MissionData> GetMissions(const json& playerJson, const json& NodesXp, const json& allNodes) {
    LogThis("called GetMissions");
    std::vector<MissionData> missions;
    MissionData data;

    // Index mission completion by tag
    std::unordered_map<std::string, json> missionDataMap;
    auto result = getValueByKey<nlohmann::json>(playerJson, "Results", nlohmann::json::array({{}}))[0];
    json missionsJson = getValueByKey<nlohmann::json>(result, "Missions");
    for (const auto& mission : missionsJson) {
        std::string tag = getValueByKey<std::string>(mission, "Tag");
        if (!tag.empty()) {
            missionDataMap[tag] = mission;
            if (tag.find("Junction") != std::string::npos) { //Junctions wont be found in public export so handle here
                data.tag = tag;
                data.name = tag; //there are names for the junctions
                size_t pos = tag.find("To", 1);
                if (pos != std::string::npos) {
                    data.region = tag.substr(0, pos);
                }
                else {
                    data.region = tag; //fallback just make it their own region
                }
                data.baseXp = 1000;
                int completes = getValueByKey<int>(mission, "Completes", 0, true);
                int tier = getValueByKey<int>(mission, "Tier", 0, true);
                data.isCompleted = completes > 0;
                data.sp = (tier == 1); // Steel Path completed
                missions.push_back(data);
            }
        }
    }

    // Process all nodes from allNodes["ExportRegions"]
    json exportRegions = getValueByKey<nlohmann::json>(allNodes, "ExportRegions");
    for (const auto& node : exportRegions) {

        data.tag = getValueByKey<std::string>(node, "uniqueName");
        data.name = getValueByKey<std::string>(node, "name");
        data.region = getValueByKey<std::string>(node, "systemName");

        // Completion logic
        auto it = missionDataMap.find(data.tag);
        if (it != missionDataMap.end()) {
            const json& m = it->second;
            int completes = getValueByKey<int>(m, "Completes", 0, true);
            int tier = getValueByKey<int>(m, "Tier", 0, true);
            //TODO: Verify if tier can be 1 and completes 0 or more concretely when it increases the tier to 1
            data.isCompleted = completes > 0;
            data.sp = (tier == 1); // Steel Path completed
        }

        // XP lookup
        if (NodesXp.contains(data.tag)) {
            data.baseXp = NodesXp[data.tag].get<int>();
        } else {
            data.baseXp = 0;
            LogThis("No Xp found for: " + data.tag);
        }

        missions.push_back(data);
    }

    return missions;
}

std::vector<MissionData> GetIncompleteMissions(const std::vector<MissionData>& missions) {
    std::vector<MissionData> result;
    for (const auto& mission : missions) {
        if (!mission.isCompleted && mission.baseXp > 0) {
            result.push_back(mission);
        }
    }
    return result;
}

int GetTotalMissionXp(const std::vector<MissionData>& missions) {
    int total = 0;
    for (const auto& mission : missions) {
        int xp = mission.baseXp;
        if (mission.isCompleted) {
            xp *= mission.sp ? 2 : 1;
            total += xp;
        }
    }
    return total;
}

MissionSummary GetMissionsSummary() {
    const json& playerJson = ReadData(DataType::Player);
    const json& NodesXp = ReadData(DataType::Nodes);
    const json& allNodes = ReadData(DataType::Regions);
    MissionSummary summary;
    summary.missions = GetMissions(playerJson, NodesXp, allNodes);
    summary.totalCount = static_cast<int>(summary.missions.size());
    summary.incompleteCount = static_cast<int>(std::count_if(
        summary.missions.begin(), summary.missions.end(),
        [](const MissionData& m) { return !m.isCompleted && m.baseXp > 0; }
    ));
    return summary;
}

int GetIntrinsicXp(const json& jsonData) {
    int totalXp = 0;

    auto result = getValueByKey<nlohmann::json>(jsonData, "Results", nlohmann::json::array({{}}))[0];
    json intrinsicJson = getValueByKey<nlohmann::json>(result, "PlayerSkills");
    for (auto& intrinsic : intrinsicJson.items()) {
        int intrinsicLevel = intrinsic.value().get<int>();
        totalXp += intrinsicLevel * 1500;
    }
    return totalXp;
}

int extraSpecialXp = 0;

int GetCurrentXP() {
    LogThis("called GetXP");
    nlohmann::json player = ReadData(DataType::Player);
    LogThis("Parsed Player Json successfully");
    int total = 0;
    MasteryInfo info;
    extraSpecialXp = 0;
    if (XPMap.empty()) {
        RefreshXPMap();
    }

    for (const auto& entry : XPMap) {
        std::string id = entry.first;
        int xp = entry.second;

        if (!id.empty()) {
            info = GetMasteryLevelForItem(id, xp);
            int toAdd = 0;
            if (info.usedHalfAffinity) {
                toAdd += info.level * 100;
            }
            else {
                toAdd += info.level * 200;
            }
            total += toAdd;
            //LogThis(id + " : " + std::to_string(toAdd));
            if (nameFromId(id, true) == id) {
                extraSpecialXp += toAdd;
                LogThis("special XP not found in export: " + id);
            }
        }
    }
    LogThis("parsed XPInfo for a total of " + std::to_string(total) + " Mastery Xp.");
    std::vector<MissionData> missiondata = GetMissions(player, ReadData(DataType::Nodes), ReadData(DataType::Regions)); //TODO: solve problem: how to get new values on updates; for DataType::Nodes
    int missionXp = GetTotalMissionXp(missiondata);
    total += missionXp;
    LogThis("got mission xp: " + std::to_string(missionXp));
    int intrinsicXp = GetIntrinsicXp(player);
    total += intrinsicXp;
    LogThis("got Intrinsic xp: " + std::to_string(intrinsicXp));

    return total;
}

std::string intrinsicNameFromKey(const std::string& key, const std::string& categoryRaw) {
    std::string name = key.substr(4);
    size_t pos = name.rfind('_');
    if (pos != std::string::npos) {
        name = name.substr(pos + 1);
    }
    return toTitleCase(name);
}

std::vector<IntrinsicCategory> GetIntrinsics() {
    LogThis("Getting ordered Intrinsics");
    const nlohmann::ordered_json data = ReadDataOrdered(DataType::Player);
    nlohmann::ordered_json intrinsics = nlohmann::ordered_json::array();

    try {
        const nlohmann::ordered_json& results = data.at("Results");
        if (!results.empty()) {
            intrinsics = results[0].at("PlayerSkills");

        } else {
            LogThis("Results was empty");
        }
    } catch (const nlohmann::json::out_of_range& e) {
        LogThis(".at failed: " + std::string(e.what()));
    }

    std::vector<IntrinsicCategory> categories;
    IntrinsicCategory* currentCategory = nullptr;
    std::string categoryRaw;

    for (auto& [key, value] : intrinsics.items()) {
        if (key.rfind("LPP_", 0) == 0) {
            // Found a new category
            categoryRaw = key.substr(4); // strip "LPP_"

            IntrinsicCategory cat;
            if (categoryNameMap.count(categoryRaw)) {
                cat.name = categoryNameMap[categoryRaw];
            } else {
                cat.name = toTitleCase(categoryRaw);
            }
            currentCategory = &categories.emplace_back(std::move(cat));
            //LogThis("CurrentCategory updated: " + currentCategory->name);
        }
        else if (key.rfind("LPS_", 0) == 0 && currentCategory) {
            Intrinsic s;
            s.name = intrinsicNameFromKey(key, categoryRaw);
            s.level = value.get<int>();
            //LogThis("   Added Intrinsic : " + s.name);
            currentCategory->skills.push_back(std::move(s));
        }
    }
    return categories;
}

std::string trim(const std::string& str) {
    std::string s = str;
    s.erase(0, s.find_first_not_of(" \t"));
    s.erase(s.find_last_not_of(" \t") + 1);
    return s;
}

std::string getHonoria() {
    auto result = getValueByKey<nlohmann::json>(ReadData(DataType::Player), "Results", nlohmann::json::array({{}}))[0];
    auto const HonoriaId = getValueByKey<std::string>(result, "TitleType");
    auto honoriaNames = getValueByKey<nlohmann::json>(ReadData(DataType::Flavour), "ExportFlavour");
    for (const auto& entry : honoriaNames) {
        if (entry.value("uniqueName", "") == HonoriaId) {
            std::string name = entry.value("name", "");
            // Remove "Honoria"
            size_t pos = name.find("Honoria");
            if (pos != std::string::npos) {
                name.erase(pos, 7);
            }
            return trim(name); //i am making extra sure that it still works even if the decide to name it "Honoria Something" in the future which i can totally see, shouldnt happen though
        }
    }
    return "";
}

std::string getClan() {
    auto result = getValueByKey<nlohmann::json>(ReadData(DataType::Player), "Results", nlohmann::json::array({{}}))[0];
    return getValueByKey<std::string>(result, "GuildName");
}

std::string getName() {
    auto result = getValueByKey<nlohmann::json>(ReadData(DataType::Player), "Results", nlohmann::json::array({{}}))[0];
    return getValueByKey<std::string>(result, "DisplayName");
}

void exportAsCsv(std::string fileName) {
    LogThis("Csv export called for: " + fileName);

    auto data = ReadData(DataType::Player);
    if (data.empty() || !data.is_object()) {
        LogThis("ERROR: No valid player data found");
        return;
    }

    // Helper function to format timestamp to CSV string (both UTC and local)
#ifdef _WIN32
    auto formatTimestamp = [](long long timestampMs) -> std::string {
        auto timestampSec = timestampMs / 1000;

        // Local time (Windows localtime_s)
        std::tm local{};
        localtime_s(&local, &timestampSec);
        std::ostringstream localOss;
        localOss << std::put_time(&local, "%m/%d/%Y %H:%M:%S");

        // UTC time (Windows gmtime_s)
        std::tm utc{};
        gmtime_s(&utc, &timestampSec);
        std::ostringstream utcOss;
        utcOss << std::put_time(&utc, "%m/%d/%Y %H:%M:%S");

        return localOss.str() + " (Local) / " + utcOss.str() + " (UTC)";
    };
#else
    // Non-Windows (Linux/Mac) - standard functions are fine
    auto formatTimestamp = [](long long timestampMs) -> std::string {
        std::time_t timestampSec = static_cast<std::time_t>(timestampMs / 1000);

        // Local time
        std::tm local = *std::localtime(&timestampSec);
        std::ostringstream localOss;
        localOss << std::put_time(&local, "%m/%d/%Y %H:%M:%S");

        // UTC time
        std::tm utc = *std::gmtime(&timestampSec);
        std::ostringstream utcOss;
        utcOss << std::put_time(&utc, "%m/%d/%Y %H:%M:%S");

        return localOss.str() + " (Local) / " + utcOss.str() + " (UTC)";
    };
#endif

    auto result = getValueByKey<nlohmann::json>(data, "Results", nlohmann::json::array({{}}))[0];
    auto stats = getValueByKey<nlohmann::json>(data, "Stats", nlohmann::json::object({}));

    // ===== BUILD MULTI-COLUMN CSV =====
    std::vector<std::vector<std::string>> columns;

    // Helper function to add a column
    auto addColumn = [&](const std::vector<std::string>& rows) {
        columns.emplace_back(rows);
    };

    auto sanitizeCsvField = [](std::string s) -> std::string {
        // Remove carriage returns and newlines
        s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
        s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());

        // If it contains commas or quotes, wrap in quotes and escape internal quotes
        if (s.find(',') != std::string::npos || s.find('"') != std::string::npos) {
            std::string out = "\"";
            for (char c : s) {
                if (c == '"') out += "\"\""; // escape quote
                else out += c;
            }
            out += "\"";
            return out;
        }
        return s;
    };

    // ===== PLAYER INFO =====
    {
        std::vector<std::string> leftColumn;
        std::vector<std::string> rightColumn;

        // --- Basic Player Info ---
        leftColumn.emplace_back("PLAYER INFO");
        rightColumn.emplace_back("");

        leftColumn.emplace_back("Name");
        rightColumn.emplace_back(getValueByKey<std::string>(result, "DisplayName", "Name Not Found"));

        // Created timestamp
        auto timestampStr = getValueByKey<std::string>(
            getValueByKey<nlohmann::json>(
                getValueByKey<nlohmann::json>(result, "Created"),
                "$date"
            ),
            "$numberLong", "0"
        );
        long long timestampMs = std::stoll(timestampStr);
        leftColumn.emplace_back("Account created");
        rightColumn.emplace_back(formatTimestamp(timestampMs));

        leftColumn.emplace_back("Mastery Rank");
        rightColumn.emplace_back(std::to_string(getValueByKey<int>(result, "PlayerLevel", -1)));

        leftColumn.emplace_back("Clan");
        rightColumn.emplace_back(getValueByKey<std::string>(result, "GuildName", ""));

        leftColumn.emplace_back("Ciphers solved");
        rightColumn.emplace_back(std::to_string(getValueByKey<int>(stats, "CiphersSolved", 0)));

        leftColumn.emplace_back("Ciphers failed");
        rightColumn.emplace_back(std::to_string(getValueByKey<int>(stats, "CiphersFailed", 0)));

        leftColumn.emplace_back("Cipher time (s)");
        rightColumn.emplace_back(std::to_string(getValueByKey<double>(stats, "CipherTime", 0.0)));

        leftColumn.emplace_back("Melee kills");
        rightColumn.emplace_back(std::to_string(getValueByKey<int>(stats, "MeleeKills", 0)));

        leftColumn.emplace_back("Missions completed");
        rightColumn.emplace_back(std::to_string(getValueByKey<int>(stats, "MissionsCompleted", 0)));

        leftColumn.emplace_back("Missions failed");
        rightColumn.emplace_back(std::to_string(getValueByKey<int>(stats, "MissionsFailed", 0)));

        leftColumn.emplace_back("Time played (hrs)");
        rightColumn.emplace_back(std::to_string(getValueByKey<double>(stats, "TimePlayedSec", 0.0) / 3600.0));

        leftColumn.emplace_back("Pickup count");
        rightColumn.emplace_back(std::to_string(getValueByKey<int>(stats, "PickupCount", 0)));

        leftColumn.emplace_back("Deaths");
        rightColumn.emplace_back(std::to_string(getValueByKey<int>(stats, "Deaths", 0)));

        leftColumn.emplace_back("Revives given");
        rightColumn.emplace_back(std::to_string(getValueByKey<int>(stats, "ReviveCount", 0)));

        leftColumn.emplace_back("Healing done");
        rightColumn.emplace_back(std::to_string(getValueByKey<int>(stats, "HealCount", 0)));

        leftColumn.emplace_back("Dojo obstacle score");
        rightColumn.emplace_back(std::to_string(getValueByKey<int>(stats, "DojoObstacleScore", 0)));

        leftColumn.emplace_back("Ollie's crash course score");
        rightColumn.emplace_back(std::to_string(getValueByKey<int>(stats, "OlliesCrashCourseScore", 0)));

        for (auto it = stats.begin(); it != stats.end(); ++it) {
            std::string key = it.key();
            if (key.find("EventScore") != std::string::npos) {
                leftColumn.emplace_back(key);
                rightColumn.emplace_back(it.value().is_number()
                    ? std::to_string(it.value().get<double>())
                    : it.value().dump());
            }
        }

        addColumn(leftColumn);
        addColumn(rightColumn);
    }

    //columns.emplace_back(); Blank column is added inside instead because it easier with the multiple split
    // ===== COMBINED ITEMS =====
    {
        auto weapons = getValueByKey<nlohmann::json>(stats, "Weapons", nlohmann::json::array({}));
        if (weapons.empty()) return;

        std::map<InventoryCategories, std::string> categoryNames = {
            {InventoryCategories::Prime, "Primes"},
            {InventoryCategories::None, "Normals"},
            {InventoryCategories::Adversary, "Adversarys"},
            {InventoryCategories::Warframe, "Warframes/Archwings"},
            {InventoryCategories::Weapon, "Weapons"},
            {InventoryCategories::Companion, "Companions"},
            {InventoryCategories::Necramech, "Necramechs"},
            {InventoryCategories::Archweapon, "Archweapons"},
            {InventoryCategories::FounderSpecial, "Founder exclusives"}
        };

        std::vector<InventoryCategories> mainCategories = {
            InventoryCategories::Prime,
            InventoryCategories::None,
            InventoryCategories::Adversary
        };

        std::vector<InventoryCategories> subCategories = {
            InventoryCategories::Warframe,
            InventoryCategories::Weapon,
            InventoryCategories::Companion,
            InventoryCategories::Necramech,
            InventoryCategories::Archweapon,
            InventoryCategories::FounderSpecial
        };

        // Collect items into category buckets
        std::map<InventoryCategories, std::map<InventoryCategories,
            std::vector<std::tuple<std::string, std::string,
                                   int,int,int,int,int,int,int>>>> groupedItems;

        for (const auto& w : weapons) {
            auto id = getValueByKey<std::string>(w, "type", "");
            auto itemCat = GetItemCategoryFromId(id);
            auto name = nameFromId(id, true);

            if (name.compare(0, 11, "<ARCHWING> ") == 0) {
                name.erase(0, 11);
            }

            int assists = getValueByKey<int>(w, "assists", 0, true);
            int kills   = getValueByKey<int>(w, "kills", 0, true);
            int fired   = getValueByKey<int>(w, "fired", 0, true);
            int hits    = getValueByKey<int>(w, "hits", 0, true);
            int headshots = getValueByKey<int>(w, "headshots", 0, true);
            int equipTime = getValueByKey<int>(w, "equipTime", 0, true);
            int xp      = getValueByKey<int>(w, "xp", 0, true);

            // Split primary vs secondary category
            InventoryCategories mainCat = InventoryCategories::None;
            if (hasCategory(itemCat, InventoryCategories::Prime))
                mainCat = InventoryCategories::Prime;
            else if (hasCategory(itemCat, InventoryCategories::Adversary))
                mainCat = InventoryCategories::Adversary;

            InventoryCategories subCat = InventoryCategories::Weapon; //default to weapon
            for (auto c : subCategories) {
                if (hasCategory(itemCat, c)) {
                    subCat = c; // keep updating — last one overrides
                }
            }

            groupedItems[mainCat][subCat].emplace_back(
                id, name, assists, kills, fired, hits, headshots, equipTime, xp
            );
        }

        // Sorting and output
        for (auto mainCat : mainCategories) {
            auto& subGroup = groupedItems[mainCat];
            if (subGroup.empty()) continue;
            addColumn(std::vector<std::string>(2, "")); // Adds a blank column

            std::vector<std::string> colHeader = {categoryNames[mainCat], "Name"};
            std::vector<std::string> colAssists = {"", "Assists"};
            std::vector<std::string> colKills = {"", "Kills"};
            std::vector<std::string> colFired = {"", "Fired"};
            std::vector<std::string> colHits = {"", "Hits"};
            std::vector<std::string> colHeadshots = {"", "Headshots"};
            std::vector<std::string> colEquipTime = {"", "EquipTime"};
            std::vector<std::string> colXP = {"", "Affinity"};
            std::vector<std::string> colMastery = {"", "Max Rank reached"};

            for (auto subCat : subCategories) {
                auto it = subGroup.find(subCat);
                if (it == subGroup.end() || it->second.empty()) continue;

                // Subheader
                colHeader.push_back("");
                colAssists.push_back("");
                colKills.push_back("");
                colFired.push_back("");
                colHits.push_back("");
                colHeadshots.push_back("");
                colEquipTime.push_back("");
                colXP.push_back("");
                colMastery.push_back("");

                colHeader.push_back(categoryNames[subCat]);
                colAssists.push_back("");
                colKills.push_back("");
                colFired.push_back("");
                colHits.push_back("");
                colHeadshots.push_back("");
                colEquipTime.push_back("");
                colXP.push_back("");
                colMastery.push_back("");

                auto& items = it->second;
                std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
                    const auto& [idA, nameA, assistsA, killsA, firedA, hitsA, headshotsA, equipA, xpA] = a;
                    const auto& [idB, nameB, assistsB, killsB, firedB, hitsB, headshotsB, equipB, xpB] = b;

                    const bool validA = (idA == nameA);
                    const bool validB = (idB == nameB);

                    if (validA != validB) {
                        return validA < validB;  //if we have no id's we just put that at the bottom; also i might be a bit tired i didnt consider that these are booleans but seems to work anyway
                    }

                    if (killsA != killsB) return killsA > killsB;
                    if (killsA != killsB) return assistsA > assistsB;
                    if (xpA != xpB) return xpA > xpB;
                    return nameA < nameB;
                });

                for (const auto& [id, name, assists, kills, fired, hits, headshots, equipTime, xp] : items) {
                    colHeader.push_back(name.empty() ? id : name);
                    colAssists.push_back(std::to_string(assists));
                    colKills.push_back(std::to_string(kills));
                    colFired.push_back(std::to_string(fired));
                    colHits.push_back(std::to_string(hits));
                    colHeadshots.push_back(std::to_string(headshots));
                    colEquipTime.push_back(std::to_string(equipTime));
                    colXP.push_back(std::to_string(xp));
                    colMastery.push_back(std::to_string(GetMasteryLevelForItem(id, xp).level));
                }
            }

            addColumn(colHeader);
            addColumn(colAssists);
            addColumn(colKills);
            addColumn(colFired);
            addColumn(colHits);
            addColumn(colHeadshots);
            addColumn(colEquipTime);
            addColumn(colXP);
            addColumn(colMastery);
        }
    }

    columns.emplace_back();
    // ===== COMBINED MISSIONS =====
    {
        auto playerMissions = getValueByKey<nlohmann::json>(result, "Missions", nlohmann::json::array({}));
        std::map<std::string, std::pair<int, int>> missionData;

        for (const auto& m : playerMissions) {
            auto tag = getValueByKey<std::string>(m, "Tag", "");
            missionData[tag] = {getValueByKey<int>(m, "Completes", 0),
                                getValueByKey<int>(m, "Tier", 0, true)};
        }

        auto statsMissions = getValueByKey<nlohmann::json>(stats, "Missions", nlohmann::json::array({}));
        std::map<std::string, int> missionHighScores;

        for (const auto& m : statsMissions) {
            auto type = getValueByKey<std::string>(m, "type", "");
            missionHighScores[type] = getValueByKey<int>(m, "highScore", 0);
            if (missionData.find(type) == missionData.end()) {
                missionData[type] = {0, 0};
            }
        }

        // Collect and sort missions
        std::vector<std::tuple<std::string, std::string, int, int, int>> missions;
        for (const auto& kv : missionData) {
            const auto& id = kv.first;
            auto name = nameFromId(id, true);
            int completes = kv.second.first;
            int tier = kv.second.second;
            int highScore = missionHighScores[id];
            missions.emplace_back(id, name, completes, tier, highScore);
        }

        std::sort(missions.begin(), missions.end(), [](const auto& a, const auto& b) {
            bool aHasName = std::get<1>(a) != std::get<0>(a);
            bool bHasName = std::get<1>(b) != std::get<0>(b);
            if (aHasName != bHasName) return aHasName > bHasName;
            if (std::get<2>(a) != std::get<2>(b)) return std::get<2>(a) > std::get<2>(b);
            if (std::get<4>(a) != std::get<4>(b)) return std::get<4>(a) > std::get<4>(b);
            if (std::get<3>(a) != std::get<3>(b)) return std::get<3>(a) > std::get<3>(b);
            return std::get<1>(a) < std::get<1>(b);
        });

        // Split missions into multiple column vectors
        std::vector<std::string> colName = {"MISSIONS", "Name"};
        std::vector<std::string> colCompletes = {"", "Completes"};
        std::vector<std::string> colTier = {"", "sp"};
        std::vector<std::string> colHighScore = {"", "HighScore"};

        for (const auto& [id, name, completes, tier, highScore] : missions) {
            colName.push_back(name.empty() ? id : name);
            colCompletes.push_back(std::to_string(completes));
            colTier.push_back(tier ? "y" : "n");
            colHighScore.push_back(std::to_string(highScore));
        }

        addColumn(colName);
        addColumn(colCompletes);
        addColumn(colTier);
        addColumn(colHighScore);
    }

    columns.emplace_back();
    // ===== ENEMIES =====
    {
        auto enemies = getValueByKey<nlohmann::json>(stats, "Enemies", nlohmann::json::array({}));

        std::vector<std::tuple<std::string, int, int, int, int, int, int>> enemyData;
        for (const auto& e : enemies) {
            auto type = getValueByKey<std::string>(e, "type", "");
            std::string name = nameFromId(type, true);

            enemyData.emplace_back(sanitizeCsvField(name),
                getValueByKey<int>(e, "kills", 0, true),
                getValueByKey<int>(e, "assists", 0, true),
                getValueByKey<int>(e, "headshots", 0, true),
                getValueByKey<int>(e, "executions", 0, true),
                getValueByKey<int>(e, "deaths", 0, true),
                getValueByKey<int>(e, "captures", 0, true));
        }

        std::sort(enemyData.begin(), enemyData.end(), [](const auto& a, const auto& b) {
            if (std::get<1>(a) != std::get<1>(b)) return std::get<1>(a) > std::get<1>(b);
            if (std::get<2>(a) != std::get<2>(b)) return std::get<2>(a) > std::get<2>(b);
            if (std::get<3>(a) != std::get<3>(b)) return std::get<3>(a) > std::get<3>(b);
            if (std::get<4>(a) != std::get<4>(b)) return std::get<4>(a) > std::get<4>(b);
            if (std::get<5>(a) != std::get<5>(b)) return std::get<5>(a) > std::get<5>(b);
            if (std::get<6>(a) != std::get<6>(b)) return std::get<6>(a) > std::get<6>(b);
            return std::get<0>(a) < std::get<0>(b);
        });

        // Separate into columns
        std::vector<std::string> colName = {"ENEMIES", "Name"};
        std::vector<std::string> colAssists = {"", "Assists"};
        std::vector<std::string> colDeaths = {"", "Deaths"};
        std::vector<std::string> colKills = {"", "Kills"};
        std::vector<std::string> colHeadshots = {"", "Headshots"};
        std::vector<std::string> colExecutions = {"", "Executions"};
        std::vector<std::string> colCaptures = {"", "Captures"};

        for (const auto& data : enemyData) {
            colName.push_back(std::get<0>(data));
            colAssists.push_back(std::to_string(std::get<2>(data)));
            colDeaths.push_back(std::to_string(std::get<5>(data)));
            colKills.push_back(std::to_string(std::get<1>(data)));
            colHeadshots.push_back(std::to_string(std::get<3>(data)));
            colExecutions.push_back(std::to_string(std::get<4>(data)));
            colCaptures.push_back(std::to_string(std::get<6>(data)));
        }

        addColumn(colName);
        addColumn(colAssists);
        addColumn(colDeaths);
        addColumn(colKills);
        addColumn(colHeadshots);
        addColumn(colExecutions);
        addColumn(colCaptures);
    }

    columns.emplace_back();
    // ===== ABILITIES =====
    {
        std::vector<std::string> abilityName;
        std::vector<std::string> abilityUsed;
        abilityName.emplace_back("ABILITIES");
        abilityName.emplace_back("Name");
        abilityUsed.emplace_back();
        abilityUsed.emplace_back("Used");

        auto abilities = getValueByKey<nlohmann::json>(stats, "Abilities", nlohmann::json::array({}));

        struct AbilityEntry {
            std::string name;
            int used;
        };

        std::vector<AbilityEntry> entries;
        for (const auto& a : abilities) {
            std::string type = getValueByKey<std::string>(a, "type", "");
            std::string name = nameFromId(type, true);
            int used = getValueByKey<int>(a, "used", 0);
            entries.push_back({name, used});
        }

        std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.used > rhs.used;
        });

        for (const auto& e : entries) {
            abilityName.emplace_back(e.name);
            abilityUsed.emplace_back(std::to_string(e.used));
        }

        addColumn(abilityName);
        addColumn(abilityUsed);
    }

    columns.emplace_back();
    // ===== PVP =====
    {
        std::vector<std::string> pvpName;
        std::vector<std::string> pvpSuitKills;
        std::vector<std::string> pvpSuitDeaths;
        std::vector<std::string> pvpWeaponKills;

        pvpName.emplace_back("PVP");
        pvpName.emplace_back("Type");
        pvpSuitKills.emplace_back();
        pvpSuitKills.emplace_back("Warframe Kills");
        pvpSuitDeaths.emplace_back();
        pvpSuitDeaths.emplace_back("Deaths");
        pvpWeaponKills.emplace_back();
        pvpWeaponKills.emplace_back("Weapon Kills");

        auto pvp = getValueByKey<nlohmann::json>(stats, "PVP", nlohmann::json::array({}));

        struct PvpEntry {
            std::string name;
            int suitKills;
            int suitDeaths;
            int weaponKills;
        };

        std::vector<PvpEntry> entries;
        for (const auto& p : pvp) {
            std::string type = getValueByKey<std::string>(p, "type", "");
            std::string name = nameFromId(type, true);
            int suitKills = getValueByKey<int>(p, "suitKills", 0, true);
            int suitDeaths = getValueByKey<int>(p, "suitDeaths", 0, true);
            int weaponKills = getValueByKey<int>(p, "weaponKills", 0, true);
            entries.push_back({name, suitKills, suitDeaths, weaponKills});
        }

        std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.weaponKills > rhs.weaponKills;
        });

        for (const auto& e : entries) {
            pvpName.emplace_back(e.name);
            pvpSuitKills.emplace_back(std::to_string(e.suitKills));
            pvpSuitDeaths.emplace_back(std::to_string(e.suitDeaths));
            pvpWeaponKills.emplace_back(std::to_string(e.weaponKills));
        }

        addColumn(pvpName);
        addColumn(pvpSuitKills);
        addColumn(pvpSuitDeaths);
        addColumn(pvpWeaponKills);
    }

    columns.emplace_back();
    // ===== SCANS =====
    {
        std::vector<std::string> scanName;
        std::vector<std::string> scanCount;

        scanName.emplace_back("SCANS");
        scanName.emplace_back("Type");
        scanCount.emplace_back();
        scanCount.emplace_back("Scans");

        auto scans = getValueByKey<nlohmann::json>(stats, "Scans", nlohmann::json::array({}));

        struct ScanEntry {
            std::string name;
            int scans;
        };

        std::vector<ScanEntry> entries;
        for (const auto& s : scans) {
            std::string type = getValueByKey<std::string>(s, "type", "");
            std::string name = nameFromId(type, true);
            entries.push_back({name, getValueByKey<int>(s, "scans", 0)});
        }

        // Sort descending by number of scans
        std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.scans > rhs.scans;
        });

        for (const auto& e : entries) {
            scanName.emplace_back(e.name);
            scanCount.emplace_back(std::to_string(e.scans));
        }

        addColumn(scanName);
        addColumn(scanCount);
    }

    // ===== COMBINE COLUMNS HORIZONTALLY =====
    // Find maximum row count
    size_t maxRows = 0;
    for (const auto& col : columns) {
        maxRows = std::max(maxRows, col.size());
    }

    // Build CSV with columns side-by-side
    std::string csvContent;
    for (size_t row = 0; row < maxRows; row++) {
        std::string line;
        for (size_t col = 0; col < columns.size(); col++) {
            if (row < columns[col].size()) {
                line += columns[col][row];
            }

            // Add comma separator between columns (except after last column)
            if (col < columns.size() - 1) {
                line += ",";
            }
        }
        csvContent += line + "\n";
    }

    // ===== SAVE FILE =====
    LogThis("Saving file...");
    if (SaveDataAt(csvContent, fileName, std::ios::out)) {
        LogThis("Csv export saved successfully: " + fileName);
    } else {
        LogThis("Csv export failed to save: " + fileName);
    }
}
