#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_set>
#include "mount/mount_engine.hpp"
namespace SwitchDex {
struct CaughtPokemonData {
    uint16_t speciesId;    
    uint16_t formId;       
    bool isShiny;          
    uint8_t level;         
    std::string nickname;  
};
class SaveParser {
public:
    SaveParser() = default;
    std::unordered_set<uint16_t> parseCaughtSpecies(const std::string& saveFilePath, GameVersion version);
    std::vector<CaughtPokemonData> parseAllCaughtData(const std::string& saveFilePath, GameVersion version);
private:
    bool decodePokemonSlot(const uint8_t* rawData, size_t slotSize, CaughtPokemonData& outData);
    static void decryptLCRNG(uint8_t* data, size_t size, uint32_t seed);
    void parseGen8SwordShield(const std::vector<uint8_t>& fileBuffer, std::vector<CaughtPokemonData>& list);
    void parseGen9ScarletViolet(const std::vector<uint8_t>& fileBuffer, std::vector<CaughtPokemonData>& list);
    void scanMemoryForPokemonBlocks(const std::vector<uint8_t>& buffer, size_t slotSize, std::vector<CaughtPokemonData>& list);
};
}
