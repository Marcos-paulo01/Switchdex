#pragma once
#include <cstdint>
#include <vector>

namespace SwitchDex {

#pragma pack(push, 1)
struct PK8Struct {
    uint32_t EC;            // 0x00
    uint16_t Checksum;      // 0x04
    uint16_t Unused;        // 0x06
    
    // Block A
    uint16_t Species;       // 0x08
    uint16_t HeldItem;      // 0x0A
    uint16_t Token;         // 0x0C
    uint16_t TID;           // 0x0E
    uint16_t SID;           // 0x10
    uint32_t EXP;           // 0x12 (Wait, is it 0x12? Actually, let's just make it a raw byte buffer)
};
#pragma pack(pop)

class PK8Crypto {
public:
    // Receives a 344-byte encrypted PK8 buffer and returns a decrypted 344-byte buffer
    static std::vector<uint8_t> decrypt(const std::vector<uint8_t>& encryptedData);
    
    // Extract Species ID from decrypted buffer
    static uint16_t getSpecies(const std::vector<uint8_t>& decryptedData);
    
    // Extract EXP from decrypted buffer
    static uint32_t getEXP(const std::vector<uint8_t>& decryptedData);
    
    // Extract Ability from decrypted buffer
    static uint16_t getAbility(const std::vector<uint8_t>& decryptedData);
    
    // Extract IVs (32-bit integer containing all 6 IVs)
    static uint32_t getIVs(const std::vector<uint8_t>& decryptedData);
    
    // Check if the PK8 checksum is valid
    static bool verifyChecksum(const std::vector<uint8_t>& decryptedData);
};

} // namespace SwitchDex
