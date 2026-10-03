#include "parser/save_parser.hpp"
#include <fstream>
#include <cstring>
#include <algorithm>
namespace SwitchDex {
static constexpr uint8_t BLOCK_SHUFFLE_TABLE[24][4] = {
    {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 2, 3, 1}, {0, 3, 1, 2}, {0, 3, 2, 1},
    {1, 0, 2, 3}, {1, 0, 3, 2}, {1, 2, 0, 3}, {1, 2, 3, 0}, {1, 3, 0, 2}, {1, 3, 2, 0},
    {2, 0, 1, 3}, {2, 0, 3, 1}, {2, 1, 0, 3}, {2, 1, 3, 0}, {2, 3, 0, 1}, {2, 3, 1, 0},
    {3, 0, 1, 2}, {3, 0, 2, 1}, {3, 1, 0, 2}, {3, 1, 2, 0}, {3, 2, 0, 1}, {3, 2, 1, 0}
};
void SaveParser::decryptLCRNG(uint8_t* data, size_t size, uint32_t seed) {
    uint16_t* words = reinterpret_cast<uint16_t*>(data);
    size_t wordCount = size / sizeof(uint16_t);
    for (size_t i = 0; i < wordCount; ++i) {
        seed = seed * 0x41C64E6D + 0x6073;
        uint16_t xorVal = static_cast<uint16_t>(seed >> 16);
        words[i] ^= xorVal;
    }
}
bool SaveParser::decodePokemonSlot(const uint8_t* rawData, size_t slotSize, CaughtPokemonData& outData) {
    if (slotSize < 336) return false;
    uint32_t encryptionConstant = *reinterpret_cast<const uint32_t*>(rawData + 0x00);
    uint16_t sanity = *reinterpret_cast<const uint16_t*>(rawData + 0x04);
    uint16_t checksum = *reinterpret_cast<const uint16_t*>(rawData + 0x06);
    if (encryptionConstant == 0 || sanity != 0) {
        return false;
    }
    size_t blockSize = (slotSize >= 344) ? 72 : 56;
    size_t totalEncryptedSize = blockSize * 4;
    if (8 + totalEncryptedSize > slotSize) return false;
    std::vector<uint8_t> decryptedPayload(totalEncryptedSize);
    std::memcpy(decryptedPayload.data(), rawData + 8, totalEncryptedSize);
    decryptLCRNG(decryptedPayload.data(), totalEncryptedSize, encryptionConstant);
    uint32_t shiftIndex = ((encryptionConstant >> 13) & 0x1F) % 24;
    const uint8_t* order = BLOCK_SHUFFLE_TABLE[shiftIndex];
    int blockAPosition = -1;
    for (int i = 0; i < 4; ++i) {
        if (order[i] == 0) {
            blockAPosition = i;
            break;
        }
    }
    if (blockAPosition == -1) return false;
    const uint8_t* blockAData = decryptedPayload.data() + (blockAPosition * blockSize);
    uint16_t speciesId = *reinterpret_cast<const uint16_t*>(blockAData + 0x00);
    if (speciesId < 1 || speciesId > 1025) {
        return false;
    }
    uint8_t formFlags = blockAData[0x1D];
    uint16_t formId = static_cast<uint16_t>(formFlags >> 3);
    uint16_t tid = *reinterpret_cast<const uint16_t*>(blockAData + 0x04);
    uint16_t sid = *reinterpret_cast<const uint16_t*>(blockAData + 0x06);
    uint32_t pid = *reinterpret_cast<const uint32_t*>(blockAData + 0x18);
    uint16_t shinyVal = tid ^ sid ^ static_cast<uint16_t>(pid >> 16) ^ static_cast<uint16_t>(pid & 0xFFFF);
    bool isShiny = (shinyVal < 16);
    outData.speciesId = speciesId;
    outData.formId = formId;
    outData.isShiny = isShiny;
    outData.level = 50;
    outData.nickname = "";
    return true;
}
void SaveParser::scanMemoryForPokemonBlocks(const std::vector<uint8_t>& buffer, size_t slotSize, std::vector<CaughtPokemonData>& list) {
    if (buffer.size() < slotSize) return;
    for (size_t offset = 0; offset + slotSize <= buffer.size(); offset += 4) {
        CaughtPokemonData mon;
        if (decodePokemonSlot(buffer.data() + offset, slotSize, mon)) {
            list.push_back(mon);
            offset += (slotSize - 4);
        }
    }
}
std::vector<CaughtPokemonData> SaveParser::parseAllCaughtData(const std::string& saveFilePath, GameVersion version) {
    std::vector<CaughtPokemonData> caughtList;
    std::ifstream file(saveFilePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return caughtList;
    }
    std::streamsize fileSize = file.tellg();
    if (fileSize <= 0 || fileSize > 16 * 1024 * 1024) {
        return caughtList;
    }
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        return caughtList;
    }
    size_t slotSize = 344;
    if (version == GameVersion::BrilliantDiamond || version == GameVersion::ShiningPearl) {
        slotSize = 336;
    }
    scanMemoryForPokemonBlocks(buffer, slotSize, caughtList);
    return caughtList;
}
std::unordered_set<uint16_t> SaveParser::parseCaughtSpecies(const std::string& saveFilePath, GameVersion version) {
    std::unordered_set<uint16_t> uniqueSpecies;
    auto allMons = parseAllCaughtData(saveFilePath, version);
    for (const auto& mon : allMons) {
        uniqueSpecies.insert(mon.speciesId);
    }
    return uniqueSpecies;
}
}
