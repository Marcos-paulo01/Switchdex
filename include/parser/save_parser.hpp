#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "mount/mount_engine.hpp"
namespace SwitchDex {
struct CaughtPokemonData {
    uint16_t speciesId;    
    uint16_t formId;       
    bool isShiny;          
    uint8_t level;         
    std::string nickname;  
    uint32_t exp;
    uint16_t abilityIndex; // 0=ability1, 1=ability2, 2=hidden
};
class SaveParser {
public:
    SaveParser() = default;
    std::unordered_map<uint16_t, CaughtPokemonData> parseCaughtSpecies(const std::string& saveFilePath, GameVersion version);
    std::vector<CaughtPokemonData> parseAllCaughtData(const std::string& saveFilePath, GameVersion version);
private:
    bool decodePokemonSlot(const uint8_t* rawData, size_t slotSize, CaughtPokemonData& outData);
    void parseGen8SwordShield(const std::vector<uint8_t>& fileBuffer, std::unordered_map<uint16_t, CaughtPokemonData>& map);
    void parseGen9ScarletViolet(const std::vector<uint8_t>& fileBuffer, std::unordered_map<uint16_t, CaughtPokemonData>& map);
    void scanMemoryForPokemonBlocks(const std::vector<uint8_t>& buffer, size_t slotSize, std::unordered_map<uint16_t, CaughtPokemonData>& map);
};
}
