#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include "db/pokedex_db.hpp"
namespace SwitchDex {
enum class AppView : int {
    TitleScreen,   
    HomeHub,       
    GamePokedex,
    PokemonDetail
};
enum class AppLanguage : int {
    English,
    Portuguese
};
class PokedexUI {
public:
    PokedexUI();
    ~PokedexUI();
    bool initialize();
    void shutdown();
    void setGameCards(const std::vector<GameCardInfo>& gameCards);
    void setPokemonList(const std::vector<PokemonEntry>& list);
    void setCurrentProfileName(const std::string& name);
    void toggleLanguage();
    void handleInput(
        uint64_t keysDown,
        uint64_t keysHeld,
        std::function<void(DexGame, CatchFilterMode, const std::string&)> onFilterChanged
    );
    void handleTouch(
        const HidTouchScreenState& state,
        std::function<void(DexGame, CatchFilterMode, const std::string&)> onFilterChanged
    );
    void render();
    [[nodiscard]] bool shouldClose() const { return m_shouldClose; }
    [[nodiscard]] AppView getCurrentView() const { return m_currentView; }
    [[nodiscard]] DexGame getSelectedGame() const { return m_selectedGame; }
    [[nodiscard]] CatchFilterMode getCatchFilter() const { return m_catchFilter; }
    [[nodiscard]] const std::string& getTypeFilter() const { return m_currentTypeFilter; }
private:
    void renderTitleScreen();
    void renderHomeHub();
    void renderGamePokedex();
    void renderFilterHeader();
    void renderGrid();
    void renderDetailPanel();
    void renderPokemonDetail();
    void renderFooterHints();
    void drawText(const std::string& text, int x, int y, SDL_Color color, TTF_Font* font, bool center = false);
    void drawTextWrapped(const std::string& text, int x, int y, Uint32 wrapLength, SDL_Color color, TTF_Font* font);
    void drawProgressBar(int x, int y, int width, int height, int value, int maxValue, SDL_Color color);
    SDL_Texture* loadTexture(const std::string& path);
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    TTF_Font* m_fontSmall = nullptr;
    TTF_Font* m_fontMedium = nullptr;
    TTF_Font* m_fontLarge = nullptr;
    TTF_Font* m_fontHuge = nullptr;
    std::unordered_map<std::string, SDL_Texture*> m_textureCache;
    AppView m_currentView = AppView::TitleScreen;
    std::vector<GameCardInfo> m_gameCards;
    std::vector<PokemonEntry> m_entries;
    int m_selectedGameIndex = 0;
    DexGame m_selectedGame = DexGame::National;
    CatchFilterMode m_catchFilter = CatchFilterMode::All;
    int m_typeFilterIndex = 0;
    std::string m_currentTypeFilter = "ALL";
    std::string m_profileName = "Trainer";
    AppLanguage m_language = AppLanguage::English;
    std::string t(const std::string& key) const;
    int m_selectedIndex = 0;
    int m_scrollOffset = 0;
    bool m_shouldClose = false;
    bool m_touchDown = false;
    bool m_isAnimating = true;
    int m_animFrame = 0;
    AppView m_targetView = AppView::TitleScreen;
    static constexpr int SCREEN_WIDTH = 1280;
    static constexpr int SCREEN_HEIGHT = 720;
    static constexpr int GRID_COLS = 5;
    static constexpr int GRID_ROWS = 4;
    static constexpr int PAGE_SIZE = GRID_COLS * GRID_ROWS;
};
}
