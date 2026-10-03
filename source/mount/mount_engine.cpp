#include "mount/mount_engine.hpp"
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace SwitchDex {

ScopedSaveMount::ScopedSaveMount(const std::string& mountPoint, uint64_t titleId, AccountUid userId)
    : m_mountPoint(mountPoint), m_mounted(false), m_lastResult(0) {
    m_lastResult = fsdevMountSaveData(m_mountPoint.c_str(), titleId, userId);
    if (R_SUCCEEDED(m_lastResult)) {
        m_mounted = true;
    }
}

ScopedSaveMount::~ScopedSaveMount() {
    unmount();
}

ScopedSaveMount::ScopedSaveMount(ScopedSaveMount&& other) noexcept
    : m_mountPoint(std::move(other.m_mountPoint)),
      m_mounted(other.m_mounted),
      m_lastResult(other.m_lastResult) {
    other.m_mounted = false;
}

ScopedSaveMount& ScopedSaveMount::operator=(ScopedSaveMount&& other) noexcept {
    if (this == &other) return *this;

    unmount();
    m_mountPoint = std::move(other.m_mountPoint);
    m_mounted = other.m_mounted;
    m_lastResult = other.m_lastResult;
    other.m_mounted = false;

    return *this;
}

void ScopedSaveMount::unmount() {
    if (!m_mounted) return;
    
    fsdevUnmountDevice(m_mountPoint.c_str());
    m_mounted = false;
}

std::string ScopedSaveMount::getFullPath(const std::string& relativeFile) const {
    if (!m_mounted) return "";
    return m_mountPoint + ":/" + relativeFile;
}

MountEngine::MountEngine() {
    initialize();
}

MountEngine::~MountEngine() {
    shutdown();
}

bool MountEngine::initialize() {
    if (!m_accountInitialized) {
        Result rc = accountInitialize(AccountServiceType_Administrator);
        if (R_FAILED(rc)) {
            rc = accountInitialize(AccountServiceType_Application);
        }
        m_accountInitialized = R_SUCCEEDED(rc);
    }
    
    if (!m_fsInitialized) {
        Result rc = fsInitialize();
        m_fsInitialized = R_SUCCEEDED(rc);
    }
    
    return m_accountInitialized && m_fsInitialized;
}

void MountEngine::shutdown() {
    if (m_accountInitialized) {
        accountExit();
        m_accountInitialized = false;
    }
    
    if (m_fsInitialized) {
        fsExit();
        m_fsInitialized = false;
    }
}

std::optional<AccountUid> MountEngine::getActiveUser() {
    if (!m_accountInitialized) return std::nullopt;
    
    AccountUid uid;
    Result rc = accountGetPreselectedUser(&uid);
    if (R_SUCCEEDED(rc) && accountUidIsValid(&uid)) return uid;
    
    auto users = listAvailableUsers();
    if (!users.empty()) return users.front();
    
    return std::nullopt;
}

std::vector<AccountUid> MountEngine::listAvailableUsers() {
    std::vector<AccountUid> userList;
    if (!m_accountInitialized) return userList;
    
    AccountUid uids[ACC_USER_LIST_SIZE];
    s32 actualTotal = 0;
    Result rc = accountListAllUsers(uids, ACC_USER_LIST_SIZE, &actualTotal);
    
    if (R_FAILED(rc) || actualTotal <= 0) return userList;
    
    for (s32 i = 0; i < actualTotal; ++i) {
        userList.push_back(uids[i]);
    }
    return userList;
}

std::string MountEngine::getUserNickname(AccountUid uid) {
    if (!m_accountInitialized) return "Unknown";
    
    AccountProfile profile;
    AccountProfileBase profileBase;
    std::memset(&profile, 0, sizeof(profile));
    std::memset(&profileBase, 0, sizeof(profileBase));
    
    Result rc = accountGetProfile(&profile, uid);
    if (R_FAILED(rc)) return "Trainer";
    
    rc = accountProfileGet(&profile, nullptr, &profileBase);
    accountProfileClose(&profile);
    
    if (R_FAILED(rc)) return "Trainer";
    
    return std::string(profileBase.nickname);
}

std::vector<GameTitleInfo> MountEngine::scanAvailableSaves(AccountUid uid) {
    std::vector<GameTitleInfo> availableGames;
    
    for (const auto& game : SUPPORTED_GAMES) {
        ScopedSaveMount mount(game.mountPoint, game.titleId, uid);
        if (!mount.isMounted()) continue;
        
        std::string saveFile = mount.getFullPath(game.saveRelativePath);
        struct stat st;
        if (stat(saveFile.c_str(), &st) != 0 || st.st_size <= 0) continue;
        
        availableGames.push_back(game);
    }
    
    return availableGames;
}

ScopedSaveMount MountEngine::mountGameSave(const GameTitleInfo& game, AccountUid uid) {
    return ScopedSaveMount(game.mountPoint, game.titleId, uid);
}

}
