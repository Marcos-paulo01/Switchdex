#include "db/pokedex_db.hpp"
#include <cstdio>
#include <sstream>
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
        "  speed INTEGER DEFAULT 0"
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
            if (count > 0) return true;
        } else {
            sqlite3_finalize(checkStmt);
        }
    }
    const char* seedSql = 
        "INSERT OR IGNORE INTO pokemon VALUES "
        "(1, 'Bulbasaur', 'Grass', 'Poison', 'Kanto', 'KANTO', 'A strange seed was planted on its back at birth. The plant sprouts and grows with this Pokémon.', 'romfs:/sprites/001.png', 45, 49, 49, 65, 65, 45),"
        "(4, 'Charmander', 'Fire', NULL, 'Kanto', 'KANTO', 'It has a preference for hot things. When it rains, steam is said to spout from the tip of its tail.', 'romfs:/sprites/004.png', 39, 52, 43, 60, 50, 65),"
        "(7, 'Squirtle', 'Water', NULL, 'Kanto', 'KANTO', 'When it retracts its long neck into its shell, it squirts water with vigorous force.', 'romfs:/sprites/007.png', 44, 48, 65, 50, 64, 43),"
        "(25, 'Pikachu', 'Electric', NULL, 'Kanto', 'KANTO', 'Pikachu that can generate powerful electricity have cheek sacs that are extra soft and stretchy.', 'romfs:/sprites/025.png', 35, 55, 40, 50, 50, 90),"
        "(133, 'Eevee', 'Normal', NULL, 'Kanto', 'KANTO', 'Its genetic code is irregular. It may mutate if it is exposed to radiation from element stones.', 'romfs:/sprites/133.png', 55, 55, 50, 45, 65, 55),"
        "(143, 'Snorlax', 'Normal', NULL, 'Kanto', 'KANTO', 'Its stomach can digest any kind of food, even if it happens to be moldy or rotten.', 'romfs:/sprites/143.png', 160, 110, 65, 65, 110, 30),"
        "(150, 'Mewtwo', 'Psychic', NULL, 'Kanto', 'KANTO', 'It was created by a scientist after years of horrific gene-splicing and DNA engineering experiments.', 'romfs:/sprites/150.png', 106, 110, 90, 154, 90, 130),"
        "(387, 'Turtwig', 'Grass', NULL, 'Sinnoh', 'BDSP', 'Made from soil, the shell on its back hardens when it drinks water. It lives along lakes.', 'romfs:/sprites/387.png', 55, 68, 64, 45, 55, 31),"
        "(390, 'Chimchar', 'Fire', NULL, 'Sinnoh', 'BDSP', 'It agilely scales sheer cliffs to live atop mountains. Its fire goes out when it goes to sleep.', 'romfs:/sprites/390.png', 44, 58, 44, 58, 44, 61),"
        "(393, 'Piplup', 'Water', NULL, 'Sinnoh', 'BDSP', 'Because it is very proud, it hates accepting food from people. Its thick down guards it from cold.', 'romfs:/sprites/393.png', 53, 51, 53, 61, 56, 40),"
        "(448, 'Lucario', 'Fighting', 'Steel', 'Sinnoh', 'BDSP', 'It can read the thoughts and movements of its opponents in battle by sensing their auras.', 'romfs:/sprites/448.png', 70, 110, 70, 115, 70, 90),"
        "(483, 'Dialga', 'Steel', 'Dragon', 'Sinnoh', 'BDSP', 'A legendary Pokémon of Sinnoh. It is said that time flowed when Dialga was born.', 'romfs:/sprites/483.png', 100, 120, 120, 150, 100, 90),"
        "(484, 'Palkia', 'Water', 'Dragon', 'Sinnoh', 'BDSP', 'It has the ability to distort space. It is described as a deity in Sinnoh-region mythology.', 'romfs:/sprites/484.png', 90, 120, 100, 150, 120, 100),"
        "(487, 'Giratina', 'Ghost', 'Dragon', 'Sinnoh', 'BDSP', 'This Pokémon is said to live in a world on the reverse side of our own.', 'romfs:/sprites/487.png', 150, 100, 120, 100, 120, 90),"
        "(810, 'Grookey', 'Grass', NULL, 'Galar', 'SWSH', 'When it uses its stick to strike a beat, the sound waves carry revitalizing energy to plants.', 'romfs:/sprites/810.png', 50, 65, 50, 40, 40, 65),"
        "(813, 'Scorbunny', 'Fire', NULL, 'Galar', 'SWSH', 'A warm-up of running around gets fire energy coursing through this Pokémon body.', 'romfs:/sprites/813.png', 50, 71, 40, 40, 40, 69),"
        "(816, 'Sobble', 'Water', NULL, 'Galar', 'SWSH', 'When it gets wet, its skin changes color, and this Pokémon becomes invisible in water.', 'romfs:/sprites/816.png', 50, 40, 40, 70, 40, 70),"
        "(861, 'Grimmsnarl', 'Dark', 'Fairy', 'Galar', 'SWSH', 'Its hairs act like muscle fibers, giving this Pokémon enormous physical power.', 'romfs:/sprites/861.png', 95, 120, 65, 95, 75, 60),"
        "(887, 'Dragapult', 'Dragon', 'Ghost', 'Galar', 'SWSH', 'When it isn’t in battle, it keeps Dreepy in the holes on its horns and launches them like missiles.', 'romfs:/sprites/887.png', 88, 120, 75, 100, 75, 142),"
        "(888, 'Zacian', 'Fairy', 'Steel', 'Galar', 'SWSH', 'Known as a legendary hero, this Pokémon absorbs metal that it then uses to create a sword to battle.', 'romfs:/sprites/888.png', 92, 130, 115, 80, 115, 138),"
        "(889, 'Zamazenta', 'Fighting', 'Steel', 'Galar', 'SWSH', 'In times past, it worked together with a king of the people to save the Galar region.', 'romfs:/sprites/889.png', 92, 130, 115, 80, 115, 138),"
        "(493, 'Arceus', 'Normal', NULL, 'Hisui', 'PLA', 'It is told in mythology that this Pokémon was the first to emerge from an egg in a universe with nothing.', 'romfs:/sprites/493.png', 120, 120, 120, 120, 120, 120),"
        "(899, 'Wyrdeer', 'Normal', 'Psychic', 'Hisui', 'PLA', 'The black orbs in its antlers shine with an uncanny light when this Pokémon is erecting invisible barriers.', 'romfs:/sprites/899.png', 103, 105, 72, 105, 75, 65),"
        "(900, 'Kleavor', 'Bug', 'Rock', 'Hisui', 'PLA', 'A violent creature that fells towering trees with its crude axe arms and chops down anything in its way.', 'romfs:/sprites/900.png', 70, 135, 95, 45, 70, 85),"
        "(901, 'Ursaluna', 'Ground', 'Normal', 'Hisui', 'PLA', 'It crossed the seas to settle in the ancient land of Hisui. Its bulk allows it to excavate peat.', 'romfs:/sprites/901.png', 130, 140, 105, 45, 80, 50),"
        "(905, 'Enamorus', 'Fairy', 'Flying', 'Hisui', 'PLA', 'When it flies across the sky, spring arrives in Hisui. It is known as the Herald of Spring.', 'romfs:/sprites/905.png', 74, 115, 70, 135, 80, 106),"
        "(906, 'Sprigatito', 'Grass', NULL, 'Paldea', 'SV', 'The sweet scent its body gives off captivates those around it, causing enemies to lose the will to fight.', 'romfs:/sprites/906.png', 40, 61, 54, 45, 45, 65),"
        "(909, 'Fuecoco', 'Fire', NULL, 'Paldea', 'SV', 'It lies on warm rocks and uses the heat absorbed by its diamond-shaped scales to create fire energy.', 'romfs:/sprites/909.png', 67, 45, 59, 63, 40, 36),"
        "(912, 'Quaxly', 'Water', NULL, 'Paldea', 'SV', 'This Pokémon migrated to Paldea from distant lands long ago. The gel on its feathers repels water.', 'romfs:/sprites/912.png', 55, 65, 45, 50, 45, 50),"
        "(937, 'Ceruledge', 'Fire', 'Ghost', 'Paldea', 'SV', 'An old set of armor steeped in grudges provided the medium for Ceruledge to emerge into life.', 'romfs:/sprites/937.png', 75, 125, 80, 60, 100, 85),"
        "(999, 'Gimmighoul', 'Ghost', NULL, 'Paldea', 'SV', 'This Pokémon was born inside a treasure chest about 1,500 years ago and leeches vitality from humans.', 'romfs:/sprites/999.png', 45, 30, 70, 75, 70, 10),"
        "(1007, 'Koraidon', 'Fighting', 'Dragon', 'Paldea', 'SV', 'Known as the Winged King. It is capable of splitting the land with its bare fists.', 'romfs:/sprites/1007.png', 100, 135, 115, 85, 100, 135),"
        "(1008, 'Miraidon', 'Electric', 'Dragon', 'Paldea', 'SV', 'Much remains unknown about this creature. It hovers swiftly using its jet engine organs.', 'romfs:/sprites/1008.png', 100, 85, 100, 135, 115, 135);";
    char* errMsg = nullptr;
    sqlite3_exec(m_db, seedSql, nullptr, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);
    return true;
}
std::vector<GameCardInfo> PokedexDB::getGameCardsInfo(
    const std::unordered_map<GameVersion, std::unordered_set<uint16_t>>& perGameCaught,
    const std::unordered_set<uint16_t>& allCaught
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
    swsh.totalCount = getGameCount("SWSH");
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
    sv.totalCount = getGameCount("SV");
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
    pla.totalCount = getGameCount("PLA");
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
    bdsp.totalCount = getGameCount("BDSP");
    bdsp.coverPath = "romfs:/covers/bdsp.jpg";
    cards.push_back(bdsp);
    return cards;
}
std::vector<PokemonEntry> PokedexDB::loadFilteredPokemon(
    DexGame game,
    CatchFilterMode filterMode,
    const std::string& typeFilter,
    const std::unordered_map<GameVersion, std::unordered_set<uint16_t>>& perGameCaught,
    const std::unordered_set<uint16_t>& allCaught
) {
    std::vector<PokemonEntry> list;
    if (!m_db) return list;
    std::ostringstream sql;
    sql << "SELECT species_id, name, type1, type2, region, game_code, description, sprite_path, hp, attack, defense, sp_attack, sp_defense, speed FROM pokemon WHERE 1=1 ";
    if (game != DexGame::National) {
        const char* code = "SWSH";
        if (game == DexGame::ScarletViolet) code = "SV";
        else if (game == DexGame::LegendsArceus) code = "PLA";
        else if (game == DexGame::BrilliantDiamondShiningPearl) code = "BDSP";
        sql << "AND game_code = '" << code << "' ";
    }
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
        bool isCaught = false;
        if (game == DexGame::National) {
            isCaught = (allCaught.find(entry.speciesId) != allCaught.end());
        } else if (game == DexGame::SwordShield) {
            if (perGameCaught.count(GameVersion::Sword) && perGameCaught.at(GameVersion::Sword).count(entry.speciesId)) isCaught = true;
            if (perGameCaught.count(GameVersion::Shield) && perGameCaught.at(GameVersion::Shield).count(entry.speciesId)) isCaught = true;
        } else if (game == DexGame::ScarletViolet) {
            if (perGameCaught.count(GameVersion::Scarlet) && perGameCaught.at(GameVersion::Scarlet).count(entry.speciesId)) isCaught = true;
            if (perGameCaught.count(GameVersion::Violet) && perGameCaught.at(GameVersion::Violet).count(entry.speciesId)) isCaught = true;
        } else if (game == DexGame::LegendsArceus) {
            if (perGameCaught.count(GameVersion::LegendsArceus) && perGameCaught.at(GameVersion::LegendsArceus).count(entry.speciesId)) isCaught = true;
        } else if (game == DexGame::BrilliantDiamondShiningPearl) {
            if (perGameCaught.count(GameVersion::BrilliantDiamond) && perGameCaught.at(GameVersion::BrilliantDiamond).count(entry.speciesId)) isCaught = true;
            if (perGameCaught.count(GameVersion::ShiningPearl) && perGameCaught.at(GameVersion::ShiningPearl).count(entry.speciesId)) isCaught = true;
        }
        entry.isUnlocked = isCaught;
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
