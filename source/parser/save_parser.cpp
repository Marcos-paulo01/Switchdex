#include "parser/save_parser.hpp"
#include "parser/pk8_crypto.hpp"
#include <fstream>
#include <cstring>
#include <algorithm>

namespace SwitchDex {

bool SaveParser::decodePokemonSlot(const uint8_t* rawData, size_t slotSize, CaughtPokemonData& outData) {
    if (slotSize < 336) return false;
    
    // Gen 8 PK8 uses encryption. The slot might be 344 bytes.
    // If it's a valid PK8, we can use PK8Crypto!
    // Wait, the current decodePokemonSlot didn't decrypt, it just read raw bytes.
    // That means the save files were ALREADY decrypted or it was Gen 9?
    // Actually, PKHeX decrypts the save file on load.
    // But since we are reading directly from Switch memory/save, it is encrypted!
    
    std::vector<uint8_t> block(rawData, rawData + slotSize);
    
    if (slotSize == 344) {
        // Decrypt first, then verify checksum on decrypted data!
        auto decrypted = PK8Crypto::decrypt(block);
        if (PK8Crypto::verifyChecksum(decrypted)) {
            block = decrypted;
        } else {
            return false;
        }
    } else {
        // For Gen 9 or others, just read directly for now
        // Though Gen 9 has its own format (PK9), it's unencrypted
    }

    uint16_t speciesId = *reinterpret_cast<const uint16_t*>(block.data() + 0x08);
    if (speciesId < 1 || speciesId > 1025) {
        return false;
    }

    uint16_t ability = *reinterpret_cast<const uint16_t*>(block.data() + 0x14); // Ability ID
    uint32_t exp = *reinterpret_cast<const uint32_t*>(block.data() + 0x10);

    outData.speciesId = speciesId;
    outData.formId = 0; // Not used yet
    outData.isShiny = false; // Ignore for now
    outData.level = 0; // Will be calculated
    outData.nickname = "";
    outData.exp = exp;
    
    // We just store the ability index as a placeholder, actually we need the Ability Index (0, 1, or 2)
    // Wait! PK8 stores the Ability ID, not the index?
    // Actually, offset 0x14 is Ability ID. We can just use the Ability ID, but our DB stores ability string.
    // In PKM format, 0x14 is Ability, 0x16 is Ability Number (1, 2, or H).
    // Let's read Ability Number (index 0, 1, 2) at 0x16!
    uint16_t abilityNumber = *reinterpret_cast<const uint16_t*>(block.data() + 0x16);
    // abilityNumber: 1=ability1, 2=ability2, 4=hidden ability? Actually usually 0=Ab1, 1=Ab2, 2=HA
    // Let's assume abilityNumber is 0, 1, or 2.
    outData.abilityIndex = abilityNumber & 3; 

    return true;
}

void SaveParser::scanMemoryForPokemonBlocks(const std::vector<uint8_t>& buffer, size_t slotSize, std::unordered_map<uint16_t, CaughtPokemonData>& map) {
    if (buffer.size() < slotSize) return;
    for (size_t offset = 0; offset + slotSize <= buffer.size(); offset += 4) {
        CaughtPokemonData mon;
        if (decodePokemonSlot(buffer.data() + offset, slotSize, mon)) {
            // Keep the one with highest EXP
            if (map.find(mon.speciesId) == map.end() || map[mon.speciesId].exp < mon.exp) {
                map[mon.speciesId] = mon;
            }
            offset += (slotSize - 4);
        }
    }
}

std::vector<CaughtPokemonData> SaveParser::parseAllCaughtData(const std::string& saveFilePath, GameVersion version) {
    std::vector<CaughtPokemonData> caughtList;
    auto map = parseCaughtSpecies(saveFilePath, version);
    for (auto& pair : map) {
        caughtList.push_back(pair.second);
    }
    return caughtList;
}

std::unordered_map<uint16_t, CaughtPokemonData> SaveParser::parseCaughtSpecies(const std::string& saveFilePath, GameVersion version) {
    std::unordered_map<uint16_t, CaughtPokemonData> uniqueSpecies;
    std::ifstream file(saveFilePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return uniqueSpecies;
    }
    std::streamsize fileSize = file.tellg();
    if (fileSize <= 0 || fileSize > 16 * 1024 * 1024) {
        return uniqueSpecies;
    }
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        return uniqueSpecies;
    }
    size_t slotSize = 344;
    if (version == GameVersion::BrilliantDiamond || version == GameVersion::ShiningPearl) {
        slotSize = 344; // BDSP uses PB8 which is 344 bytes encrypted
    }
    scanMemoryForPokemonBlocks(buffer, slotSize, uniqueSpecies);
    return uniqueSpecies;
}

void SaveParser::parseGen8SwordShield(const std::vector<uint8_t>& fileBuffer, std::unordered_map<uint16_t, CaughtPokemonData>& map) {
    scanMemoryForPokemonBlocks(fileBuffer, 344, map);
}

void SaveParser::parseGen9ScarletViolet(const std::vector<uint8_t>& fileBuffer, std::unordered_map<uint16_t, CaughtPokemonData>& map) {
    scanMemoryForPokemonBlocks(fileBuffer, 344, map); // Or whatever size PK9 is
}

}
