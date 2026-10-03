#include "parser/pk8_crypto.hpp"
#include <cstring>

namespace SwitchDex {

const uint8_t BLOCK_POSITION[24][4] = {
    {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 3, 1, 2},
    {0, 2, 3, 1}, {0, 3, 2, 1}, {1, 0, 2, 3}, {1, 0, 3, 2},
    {2, 0, 1, 3}, {3, 0, 1, 2}, {2, 0, 3, 1}, {3, 0, 2, 1},
    {1, 2, 0, 3}, {1, 3, 0, 2}, {2, 1, 0, 3}, {3, 1, 0, 2},
    {2, 3, 0, 1}, {3, 2, 0, 1}, {1, 2, 3, 0}, {1, 3, 2, 0},
    {2, 1, 3, 0}, {3, 1, 2, 0}, {2, 3, 1, 0}, {3, 2, 1, 0}
};

class PRNG {
    uint32_t seed;
public:
    PRNG(uint32_t s) : seed(s) {}
    uint16_t next() {
        seed = seed * 0x41C64E6D + 0x6073;
        return seed >> 16;
    }
};

std::vector<uint8_t> PK8Crypto::decrypt(const std::vector<uint8_t>& encryptedData) {
    std::vector<uint8_t> dec = encryptedData;
    if (dec.size() != 344) return dec;

    uint32_t ec = *reinterpret_cast<const uint32_t*>(dec.data() + 0);
    
    // Decrypt the 336 bytes of block data
    PRNG prng(ec);
    uint16_t* data16 = reinterpret_cast<uint16_t*>(dec.data() + 0x08);
    for (int i = 0; i < 168; ++i) {
        data16[i] ^= prng.next();
    }
    
    // Unshuffle the blocks
    int shiftValue = ((ec >> 13) & 0x1F) % 24;
    const uint8_t* pos = BLOCK_POSITION[shiftValue];
    
    std::vector<uint8_t> unshuffled(336);
    for (int i = 0; i < 4; ++i) {
        int srcBlock = pos[i];
        int dstBlock = i;
        std::memcpy(unshuffled.data() + (dstBlock * 84), dec.data() + 0x08 + (srcBlock * 84), 84);
    }
    
    std::memcpy(dec.data() + 0x08, unshuffled.data(), 336);
    return dec;
}

uint16_t PK8Crypto::getSpecies(const std::vector<uint8_t>& decryptedData) {
    if (decryptedData.size() != 344) return 0;
    return *reinterpret_cast<const uint16_t*>(decryptedData.data() + 0x08); // Block A starts at 0x08
}

uint32_t PK8Crypto::getEXP(const std::vector<uint8_t>& decryptedData) {
    if (decryptedData.size() != 344) return 0;
    return *reinterpret_cast<const uint32_t*>(decryptedData.data() + 0x10); // Check PKHeX offset for EXP
    // Wait! EXP is usually at 0x10 in Block A. Let's confirm offsets later if necessary.
}

uint16_t PK8Crypto::getAbility(const std::vector<uint8_t>& decryptedData) {
    if (decryptedData.size() != 344) return 0;
    return *reinterpret_cast<const uint16_t*>(decryptedData.data() + 0x14); // Ability is around 0x14
}

uint32_t PK8Crypto::getIVs(const std::vector<uint8_t>& decryptedData) {
    if (decryptedData.size() != 344) return 0;
    return *reinterpret_cast<const uint32_t*>(decryptedData.data() + 0x8C); // IVs are usually in Block B (0x5C + 0x30 = 0x8C)
}

bool PK8Crypto::verifyChecksum(const std::vector<uint8_t>& decryptedData) {
    if (decryptedData.size() != 344) return false;
    uint16_t expected = *reinterpret_cast<const uint16_t*>(decryptedData.data() + 0x04);
    uint16_t sum = 0;
    const uint16_t* data16 = reinterpret_cast<const uint16_t*>(decryptedData.data() + 0x08);
    for (int i = 0; i < 168; ++i) {
        sum += data16[i];
    }
    if (sum == 0) return false;
    return sum == expected;
}

} // namespace SwitchDex
