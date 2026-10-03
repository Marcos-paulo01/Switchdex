#pragma once
#include <switch.h>
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
namespace SwitchDex {
enum class GameVersion : uint32_t {
    Sword,
    Shield,
    BrilliantDiamond,
    ShiningPearl,
    LegendsArceus,
    Scarlet,
    Violet,
    LegendsZA,
    Unknown
};
struct GameTitleInfo {
    GameVersion version;
    uint64_t titleId;
    const char* name;
    const char* mountPoint;     
    const char* saveRelativePath;
};
inline constexpr GameTitleInfo SUPPORTED_GAMES[] = {
    { GameVersion::Sword,            0x0100ABF008968000ULL, "Pokémon Sword",            "save_sw",  "main" },
    { GameVersion::Shield,           0x01008DB008C2C000ULL, "Pokémon Shield",           "save_sh",  "main" },
    { GameVersion::BrilliantDiamond, 0x0100000011D44000ULL, "Pokémon Brilliant Diamond","save_bd",  "savedata.bin" },
    { GameVersion::ShiningPearl,     0x010018E011D92000ULL, "Pokémon Shining Pearl",    "save_sp",  "savedata.bin" },
    { GameVersion::LegendsArceus,    0x01001F5010DFA000ULL, "Pokémon Legends: Arceus",  "save_pla", "main" },
    { GameVersion::Scarlet,          0x0100A3D008F5C000ULL, "Pokémon Scarlet",          "save_sc",  "main" },
    { GameVersion::Violet,           0x01008F6008C5E000ULL, "Pokémon Violet",           "save_vi",  "main" }
};
class ScopedSaveMount {
public:
    ScopedSaveMount(const std::string& mountPoint, uint64_t titleId, AccountUid userId);
    ~ScopedSaveMount();
    ScopedSaveMount(const ScopedSaveMount&) = delete;
    ScopedSaveMount& operator=(const ScopedSaveMount&) = delete;
    ScopedSaveMount(ScopedSaveMount&& other) noexcept;
    ScopedSaveMount& operator=(ScopedSaveMount&& other) noexcept;
    [[nodiscard]] bool isMounted() const { return m_mounted; }
    [[nodiscard]] const std::string& getMountPoint() const { return m_mountPoint; }
    [[nodiscard]] std::string getFullPath(const std::string& relativeFile) const;
    [[nodiscard]] Result getLastError() const { return m_lastResult; }
    void unmount();
private:
    std::string m_mountPoint;
    bool m_mounted = false;
    Result m_lastResult = 0;
};
class MountEngine {
public:
    MountEngine();
    ~MountEngine();
    bool initialize();
    void shutdown();
    std::optional<AccountUid> getActiveUser();
    std::vector<AccountUid> listAvailableUsers();
    std::string getUserNickname(AccountUid uid);
    std::vector<GameTitleInfo> scanAvailableSaves(AccountUid uid);
    ScopedSaveMount mountGameSave(const GameTitleInfo& game, AccountUid uid);
private:
    bool m_accountInitialized = false;
    bool m_fsInitialized = false;
};
}
