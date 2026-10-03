#include <switch.h>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "mount/mount_engine.hpp"
#include "parser/save_parser.hpp"
#include "db/pokedex_db.hpp"
#include "ui/pokedex_ui.hpp"
struct ProfileData {
    AccountUid uid;
    std::string nickname;
    std::unordered_map<SwitchDex::GameVersion, std::unordered_set<uint16_t>> perGameCaught;
    std::unordered_set<uint16_t> aggregatedCaughtSpecies;
};
void populateProfiles(std::vector<ProfileData>& profiles, SwitchDex::MountEngine& mountEngine, SwitchDex::SaveParser& saveParser) {
    auto users = mountEngine.listAvailableUsers();
    for (auto user : users) {
        ProfileData pd;
        pd.uid = user;
        pd.nickname = mountEngine.getUserNickname(user);
        auto availableGames = mountEngine.scanAvailableSaves(user);
        for (const auto& game : availableGames) {
            auto saveMount = mountEngine.mountGameSave(game, user);
            if (!saveMount.isMounted()) continue;
            std::string fullSavePath = saveMount.getFullPath(game.saveRelativePath);
            auto speciesFound = saveParser.parseCaughtSpecies(fullSavePath, game.version);
            pd.perGameCaught[game.version].insert(speciesFound.begin(), speciesFound.end());
            pd.aggregatedCaughtSpecies.insert(speciesFound.begin(), speciesFound.end());
        }
        profiles.push_back(pd);
    }
    if (profiles.empty()) {
        profiles.push_back({ {}, "Trainer", {}, {} });
    }
}
int main(int argc, char* argv[]) {
    Result rc = romfsInit();
    bool romfsMounted = R_SUCCEEDED(rc);
    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);
    hidInitializeTouchScreen();
    SwitchDex::MountEngine mountEngine;
    SwitchDex::SaveParser saveParser;
    SwitchDex::PokedexDB pokedexDB;
    SwitchDex::PokedexUI pokedexUI;
    if (!pokedexUI.initialize()) {
        if (romfsMounted) romfsExit();
        return 1;
    }
    std::vector<ProfileData> profiles;
    populateProfiles(profiles, mountEngine, saveParser);
    int currentProfileIndex = 0;
    if (!pokedexDB.open(":memory:")) {
        pokedexDB.open("sdmc:/switch/switchdex/pokedex.db");
    }
    pokedexDB.seedSampleData();
    auto updateProfileUI = [&]() {
        const auto& pd = profiles[currentProfileIndex];
        pokedexUI.setCurrentProfileName(pd.nickname);
        auto gameCards = pokedexDB.getGameCardsInfo(pd.perGameCaught, pd.aggregatedCaughtSpecies);
        pokedexUI.setGameCards(gameCards);
    };
    updateProfileUI();
    auto updatePokedexList = [&](SwitchDex::DexGame game, SwitchDex::CatchFilterMode filterMode, const std::string& typeFilter) {
        const auto& pd = profiles[currentProfileIndex];
        auto list = pokedexDB.loadFilteredPokemon(game, filterMode, typeFilter, pd.perGameCaught, pd.aggregatedCaughtSpecies);
        pokedexUI.setPokemonList(list);
    };
    updatePokedexList(SwitchDex::DexGame::National, SwitchDex::CatchFilterMode::All, "ALL");
    while (appletMainLoop() && !pokedexUI.shouldClose()) {
        padUpdate(&pad);
        uint64_t kDown = padGetButtonsDown(&pad);
        if (pokedexUI.getCurrentView() == SwitchDex::AppView::HomeHub) {
            if (kDown & HidNpadButton_L) {
                currentProfileIndex = (currentProfileIndex == 0) ? (profiles.size() - 1) : (currentProfileIndex - 1);
                updateProfileUI();
            } else if (kDown & HidNpadButton_R) {
                currentProfileIndex = (currentProfileIndex + 1) % profiles.size();
                updateProfileUI();
            }
        }
        HidTouchScreenState touchState = {0};
        hidGetTouchScreenStates(&touchState, 1);
        pokedexUI.handleInput(kDown, [&](SwitchDex::DexGame game, SwitchDex::CatchFilterMode filterMode, const std::string& typeFilter) {
            updatePokedexList(game, filterMode, typeFilter);
        });
        pokedexUI.handleTouch(touchState, [&](SwitchDex::DexGame game, SwitchDex::CatchFilterMode filterMode, const std::string& typeFilter) {
            updatePokedexList(game, filterMode, typeFilter);
        });
        pokedexUI.render();
    }
    pokedexUI.shutdown();
    pokedexDB.close();
    mountEngine.shutdown();
    if (romfsMounted) {
        romfsExit();
    }
    return 0;
}
