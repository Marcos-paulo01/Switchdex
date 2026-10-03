#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <cstdint>
#include "mount/mount_engine.hpp"
#include "db/sqlite3.h"
namespace SwitchDex {
enum class DexGame : int {
    National = 0,
    SwordShield = 1,
    ScarletViolet = 2,
    LegendsArceus = 3,
    BrilliantDiamondShiningPearl = 4,
    Count = 5
};
struct GameCardInfo {
    DexGame gameId;
    std::string title;
    std::string subtitle;
    std::string region;
    std::string releaseYear;
    std::string boxArtColor;
    bool hasSaveData = false;
    int caughtCount = 0;
    int totalCount = 0;
    std::string coverPath;
};
enum class CatchFilterMode : int {
    All = 0,
    CaughtOnly = 1,
    UncaughtOnly = 2,
    Count = 3
};
struct PokemonEntry {
    uint16_t speciesId;
    std::string name;
    std::string type1;
    std::string type2;
    std::string region;
    std::string gameCode;
    std::string description;
    std::string spritePath;
    int hp;
    int attack;
    int defense;
    int spAttack;
    int spDefense;
    int speed;
    bool isUnlocked = false;
};
class PokedexDB {
public:
    PokedexDB();
    ~PokedexDB();
    bool open(const std::string& dbPath);
    void close();
    std::vector<GameCardInfo> getGameCardsInfo(
        const std::unordered_map<GameVersion, std::unordered_set<uint16_t>>& perGameCaught,
        const std::unordered_set<uint16_t>& allCaught
    );
    std::vector<PokemonEntry> loadFilteredPokemon(
        DexGame game,
        CatchFilterMode filterMode,
        const std::string& typeFilter,
        const std::unordered_map<GameVersion, std::unordered_set<uint16_t>>& perGameCaught,
        const std::unordered_set<uint16_t>& allCaught
    );
    bool createTablesIfNotExist();
    bool seedSampleData();
private:
    sqlite3* m_db = nullptr;
};
}
