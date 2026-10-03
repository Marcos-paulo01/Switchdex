#include "db/pokedex_db.hpp"
#include <cstdio>
#include <sstream>
#include <fstream>
namespace SwitchDex {
PokedexDB::PokedexDB() = default;
PokedexDB::~PokedexDB() {
    close();
}
bool PokedexDB::open(const std::string& dbPath) {
    close();
    int rc = sqlite3_open(dbPath.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        if (m_db) {
            sqlite3_close(m_db);
            m_db = nullptr;
        }
        return false;
    }
    createTablesIfNotExist();
    return true;
}
void PokedexDB::close() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}
bool PokedexDB::createTablesIfNotExist() {
    if (!m_db) return false;
    const char* sql = 
        "CREATE TABLE IF NOT EXISTS pokemon ("
        "  species_id INTEGER PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  type1 TEXT NOT NULL,"
        "  type2 TEXT,"
        "  region TEXT NOT NULL,"
        "  game_code TEXT NOT NULL,"
        "  description TEXT,"
        "  sprite_path TEXT,"
        "  hp INTEGER DEFAULT 0,"
        "  attack INTEGER DEFAULT 0,"
        "  defense INTEGER DEFAULT 0,"
        "  sp_attack INTEGER DEFAULT 0,"
        "  sp_defense INTEGER DEFAULT 0,"
        "  speed INTEGER DEFAULT 0,"
        "  growth_rate TEXT,"
        "  ability1 TEXT,"
        "  ability2 TEXT,"
        "  hidden_ability TEXT,"
        "  evolve_level INTEGER"
        ");";
    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }
    return true;
}
bool PokedexDB::seedSampleData() {
    if (!m_db) return false;
    const char* checkSql = "SELECT COUNT(*) FROM pokemon;";
    sqlite3_stmt* checkStmt = nullptr;
    if (sqlite3_prepare_v2(m_db, checkSql, -1, &checkStmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(checkStmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(checkStmt, 0);
            sqlite3_finalize(checkStmt);
            if (count > 0) {
                // If the old table doesn't have growth_rate, this will crash the new queries or just not load.
                // We should probably just recreate it by dropping it.
                // But wait, the app deletes pokedex.db if it's old? Let's just drop it.
                sqlite3_exec(m_db, "DROP TABLE pokemon;", nullptr, nullptr, nullptr);
                createTablesIfNotExist();
            }
        } else {
            sqlite3_finalize(checkStmt);
        }
    }
    std::ifstream file("romfs:/db/pokemon_seed.sql");
    if (!file.is_open()) return false;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string seedSql = buffer.str();
    
    char* errMsg = nullptr;
    sqlite3_exec(m_db, seedSql.c_str(), nullptr, nullptr, &errMsg);
    if (errMsg) {
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}
std::vector<GameCardInfo> PokedexDB::getGameCardsInfo(
    const std::unordered_map<GameVersion, std::unordered_map<uint16_t, CaughtPokemonData>>& perGameCaught,
    const std::unordered_map<uint16_t, CaughtPokemonData>& allCaught
) {
    std::vector<GameCardInfo> cards;
    auto getGameCount = [this](const std::string& code) -> int {
        if (!m_db) return 0;
        std::string sql = code.empty() 
            ? "SELECT COUNT(*) FROM pokemon;" 
            : "SELECT COUNT(*) FROM pokemon WHERE game_code = '" + code + "';";
        sqlite3_stmt* stmt = nullptr;
        int count = 0;
        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                count = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
        return count;
    };
    GameCardInfo nat;
    nat.gameId = DexGame::National;
    nat.title = "NATIONAL POKÉDEX";
    nat.subtitle = "All Nintendo Switch Generations";
    nat.region = "Universal";
    nat.releaseYear = "Comprehensive";
    nat.hasSaveData = !allCaught.empty();
    nat.caughtCount = static_cast<int>(allCaught.size());
    nat.totalCount = getGameCount("");
    nat.coverPath = "romfs:/covers/national.jpg";
    cards.push_back(nat);
    bool hasSwSh = perGameCaught.count(GameVersion::Sword) || perGameCaught.count(GameVersion::Shield);
    size_t swshCaught = 0;
    if (perGameCaught.count(GameVersion::Sword)) swshCaught += perGameCaught.at(GameVersion::Sword).size();
    if (perGameCaught.count(GameVersion::Shield)) swshCaught += perGameCaught.at(GameVersion::Shield).size();
    GameCardInfo swsh;
    swsh.gameId = DexGame::SwordShield;
    swsh.title = "SWORD & SHIELD";
    swsh.subtitle = "Galar Region";
    swsh.region = "Galar";
    swsh.releaseYear = "2019";
    swsh.hasSaveData = hasSwSh;
    swsh.caughtCount = static_cast<int>(swshCaught);
    swsh.totalCount = getGameCount("");
    swsh.coverPath = "romfs:/covers/swsh.jpg";
    cards.push_back(swsh);
    bool hasSV = perGameCaught.count(GameVersion::Scarlet) || perGameCaught.count(GameVersion::Violet);
    size_t svCaught = 0;
    if (perGameCaught.count(GameVersion::Scarlet)) svCaught += perGameCaught.at(GameVersion::Scarlet).size();
    if (perGameCaught.count(GameVersion::Violet)) svCaught += perGameCaught.at(GameVersion::Violet).size();
    GameCardInfo sv;
    sv.gameId = DexGame::ScarletViolet;
    sv.title = "SCARLET & VIOLET";
    sv.subtitle = "Paldea Region";
    sv.region = "Paldea";
    sv.releaseYear = "2022";
    sv.hasSaveData = hasSV;
    sv.caughtCount = static_cast<int>(svCaught);
    sv.totalCount = getGameCount("");
    sv.coverPath = "romfs:/covers/sv.jpg";
    cards.push_back(sv);
    bool hasPLA = perGameCaught.count(GameVersion::LegendsArceus);
    size_t plaCaught = hasPLA ? perGameCaught.at(GameVersion::LegendsArceus).size() : 0;
    GameCardInfo pla;
    pla.gameId = DexGame::LegendsArceus;
    pla.title = "LEGENDS: ARCEUS";
    pla.subtitle = "Ancient Hisui";
    pla.region = "Hisui";
    pla.releaseYear = "2022";
    pla.hasSaveData = hasPLA;
    pla.caughtCount = static_cast<int>(plaCaught);
    pla.totalCount = getGameCount("");
    pla.coverPath = "romfs:/covers/pla.jpg";
    cards.push_back(pla);
    bool hasBDSP = perGameCaught.count(GameVersion::BrilliantDiamond) || perGameCaught.count(GameVersion::ShiningPearl);
    size_t bdspCaught = 0;
    if (perGameCaught.count(GameVersion::BrilliantDiamond)) bdspCaught += perGameCaught.at(GameVersion::BrilliantDiamond).size();
    if (perGameCaught.count(GameVersion::ShiningPearl)) bdspCaught += perGameCaught.at(GameVersion::ShiningPearl).size();
    GameCardInfo bdsp;
    bdsp.gameId = DexGame::BrilliantDiamondShiningPearl;
    bdsp.title = "BD & SP";
    bdsp.subtitle = "Sinnoh Region";
    bdsp.region = "Sinnoh";
    bdsp.releaseYear = "2021";
    bdsp.hasSaveData = hasBDSP;
    bdsp.caughtCount = static_cast<int>(bdspCaught);
    bdsp.totalCount = getGameCount("");
    bdsp.coverPath = "romfs:/covers/bdsp.jpg";
    cards.push_back(bdsp);
    
    // Add Legends Z-A (PLZA)
    GameCardInfo plza;
    plza.gameId = DexGame::LegendsZA;
    plza.title = "LEGENDS: Z-A";
    plza.subtitle = "Lumiose City";
    plza.region = "Kalos";
    plza.releaseYear = "2025";
    plza.hasSaveData = false; // Add save data check when available
    plza.caughtCount = 0;
    plza.totalCount = getGameCount("");
    plza.coverPath = "romfs:/covers/national.jpg"; // Placeholder cover
    cards.push_back(plza);
    
    return cards;
}
std::vector<PokemonEntry> PokedexDB::loadFilteredPokemon(
    DexGame game,
    CatchFilterMode filterMode,
    const std::string& typeFilter,
    const std::unordered_map<GameVersion, std::unordered_map<uint16_t, CaughtPokemonData>>& perGameCaught,
    const std::unordered_map<uint16_t, CaughtPokemonData>& allCaught
) {
    std::vector<PokemonEntry> list;
    if (!m_db) return list;
    std::ostringstream sql;
    sql << "SELECT species_id, name, type1, type2, region, game_code, description, sprite_path, hp, attack, defense, sp_attack, sp_defense, speed, growth_rate, ability1, ability2, hidden_ability, evolve_level FROM pokemon WHERE 1=1 ";
    if (!typeFilter.empty() && typeFilter != "ALL") {
        sql << "AND (type1 = '" << typeFilter << "' OR type2 = '" << typeFilter << "') ";
    }
    sql << "ORDER BY species_id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return list;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PokemonEntry entry;
        entry.speciesId = static_cast<uint16_t>(sqlite3_column_int(stmt, 0));
        entry.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        entry.type1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const unsigned char* type2Text = sqlite3_column_text(stmt, 3);
        entry.type2 = type2Text ? reinterpret_cast<const char*>(type2Text) : "";
        entry.region = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        entry.gameCode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const unsigned char* descText = sqlite3_column_text(stmt, 6);
        entry.description = descText ? reinterpret_cast<const char*>(descText) : "";
        const unsigned char* spriteText = sqlite3_column_text(stmt, 7);
        entry.spritePath = spriteText ? reinterpret_cast<const char*>(spriteText) : "";
        entry.hp = sqlite3_column_int(stmt, 8);
        entry.attack = sqlite3_column_int(stmt, 9);
        entry.defense = sqlite3_column_int(stmt, 10);
        entry.spAttack = sqlite3_column_int(stmt, 11);
        entry.spDefense = sqlite3_column_int(stmt, 12);
        entry.speed = sqlite3_column_int(stmt, 13);
        const unsigned char* grText = sqlite3_column_text(stmt, 14);
        entry.growthRate = grText ? reinterpret_cast<const char*>(grText) : "";
        const unsigned char* ab1Text = sqlite3_column_text(stmt, 15);
        entry.ability1 = ab1Text ? reinterpret_cast<const char*>(ab1Text) : "";
        const unsigned char* ab2Text = sqlite3_column_text(stmt, 16);
        entry.ability2 = ab2Text ? reinterpret_cast<const char*>(ab2Text) : "";
        const unsigned char* hAbText = sqlite3_column_text(stmt, 17);
        entry.hiddenAbility = hAbText ? reinterpret_cast<const char*>(hAbText) : "";
        entry.evolveLevel = sqlite3_column_type(stmt, 18) == SQLITE_NULL ? -1 : sqlite3_column_int(stmt, 18);
        bool isCaught = false;
        const CaughtPokemonData* cData = nullptr;
        if (game == DexGame::National) {
            auto it = allCaught.find(entry.speciesId);
            if (it != allCaught.end()) { isCaught = true; cData = &it->second; }
        } else if (game == DexGame::SwordShield) {
            if (perGameCaught.count(GameVersion::Sword)) { auto it = perGameCaught.at(GameVersion::Sword).find(entry.speciesId); if (it != perGameCaught.at(GameVersion::Sword).end()) { isCaught = true; cData = &it->second; } }
            if (perGameCaught.count(GameVersion::Shield)) { auto it = perGameCaught.at(GameVersion::Shield).find(entry.speciesId); if (it != perGameCaught.at(GameVersion::Shield).end()) { isCaught = true; cData = &it->second; } }
        } else if (game == DexGame::ScarletViolet) {
            if (perGameCaught.count(GameVersion::Scarlet)) { auto it = perGameCaught.at(GameVersion::Scarlet).find(entry.speciesId); if (it != perGameCaught.at(GameVersion::Scarlet).end()) { isCaught = true; cData = &it->second; } }
            if (perGameCaught.count(GameVersion::Violet)) { auto it = perGameCaught.at(GameVersion::Violet).find(entry.speciesId); if (it != perGameCaught.at(GameVersion::Violet).end()) { isCaught = true; cData = &it->second; } }
        } else if (game == DexGame::LegendsArceus) {
            if (perGameCaught.count(GameVersion::LegendsArceus)) { auto it = perGameCaught.at(GameVersion::LegendsArceus).find(entry.speciesId); if (it != perGameCaught.at(GameVersion::LegendsArceus).end()) { isCaught = true; cData = &it->second; } }
        } else if (game == DexGame::BrilliantDiamondShiningPearl) {
            if (perGameCaught.count(GameVersion::BrilliantDiamond)) { auto it = perGameCaught.at(GameVersion::BrilliantDiamond).find(entry.speciesId); if (it != perGameCaught.at(GameVersion::BrilliantDiamond).end()) { isCaught = true; cData = &it->second; } }
            if (perGameCaught.count(GameVersion::ShiningPearl)) { auto it = perGameCaught.at(GameVersion::ShiningPearl).find(entry.speciesId); if (it != perGameCaught.at(GameVersion::ShiningPearl).end()) { isCaught = true; cData = &it->second; } }
        } else if (game == DexGame::LegendsZA) {
            isCaught = false; 
        }
        entry.isUnlocked = isCaught;
        if (cData) {
            entry.saveExp = cData->exp;
            entry.saveAbilityIndex = cData->abilityIndex;
        }
        if (filterMode == CatchFilterMode::CaughtOnly && !entry.isUnlocked) {
            continue;
        }
        if (filterMode == CatchFilterMode::UncaughtOnly && entry.isUnlocked) {
            continue;
        }
        list.push_back(entry);
    }
    sqlite3_finalize(stmt);
    return list;
}
}
