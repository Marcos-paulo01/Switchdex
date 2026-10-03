#include "ui/pokedex_ui.hpp"
#include <switch.h>
#include <algorithm>
#include <iomanip>
#include <sstream>
namespace SwitchDex {
static const std::vector<std::string> ELEMENT_TYPES = {
    "ALL", "Grass", "Fire", "Water", "Electric", "Normal", "Fighting",
    "Flying", "Poison", "Ground", "Rock", "Bug", "Ghost", "Steel",
    "Psychic", "Ice", "Dragon", "Dark", "Fairy"
};
PokedexUI::PokedexUI() = default;
PokedexUI::~PokedexUI() {
    shutdown();
}
bool PokedexUI::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        return false;
    }
    if (TTF_Init() < 0) {
        return false;
    }
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        return false;
    }
    m_window = SDL_CreateWindow(
        "SwitchDex - Nintendo Switch Pokémon Tracker",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!m_window) return false;
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) return false;
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    const char* fontPath = "romfs:/fonts/Inter-SemiBold.ttf";
    m_fontSmall = TTF_OpenFont(fontPath, 15);
    m_fontMedium = TTF_OpenFont(fontPath, 19);
    m_fontLarge = TTF_OpenFont(fontPath, 26);
    m_fontHuge = TTF_OpenFont(fontPath, 34);
    return true;
}
void PokedexUI::shutdown() {
    for (auto& pair : m_textureCache) {
        if (pair.second) {
            SDL_DestroyTexture(pair.second);
        }
    }
    m_textureCache.clear();
    if (m_fontSmall) TTF_CloseFont(m_fontSmall);
    if (m_fontMedium) TTF_CloseFont(m_fontMedium);
    if (m_fontLarge) TTF_CloseFont(m_fontLarge);
    if (m_fontHuge) TTF_CloseFont(m_fontHuge);
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}
void PokedexUI::setGameCards(const std::vector<GameCardInfo>& gameCards) {
    m_gameCards = gameCards;
}
void PokedexUI::setPokemonList(const std::vector<PokemonEntry>& list) {
    m_entries = list;
    m_selectedIndex = 0;
    m_scrollOffset = 0;
}
void PokedexUI::setCurrentProfileName(const std::string& name) {
    m_profileName = name;
}
void PokedexUI::toggleLanguage() {
    m_language = (m_language == AppLanguage::English) ? AppLanguage::Portuguese : AppLanguage::English;
}
std::string PokedexUI::t(const std::string& key) const {
    if (m_language == AppLanguage::English) return key;
    static const std::unordered_map<std::string, std::string> ptBR = {
        {"Press Any Button to Start", "Pressione Qualquer Botão para Iniciar"},
        {"Select a Game Pokédex Module", "Selecione um Módulo de Pokédex"},
        {"Profile: ", "Perfil: "},
        {"NO IMAGE", "SEM IMAGEM"},
        {"Save Detected", "Save Encontrado"},
        {"No Save Found", "Nenhum Save Encontrado"},
        {"BOOT DEX", "INICIAR DEX"},
        {"National Pokédex", "Pokédex Nacional"},
        {"Status: ", "Status: "},
        {"ALL", "TODOS"},
        {"CAUGHT ONLY", "CAPTURADOS"},
        {"UNCAUGHT ONLY", "NÃO CAPTURADOS"},
        {"Type: ", "Tipo: "},
        {"[B] Game Menu", "[B] Menu de Jogos"},
        {"Registered: ", "Registrados: "},
        {"Region: ", "Região: "},
        {"Pokédex Entry:", "Registro da Pokédex:"},
        {"BASE STATS", "ATRIBUTOS BASE"},
        {"Attack", "Ataque"},
        {"Defense", "Defesa"},
        {"Sp. Atk", "Atq. Esp"},
        {"Sp. Def", "Def. Esp"},
        {"Speed", "Velocidade"},
        {"STATUS: NOT REGISTERED", "STATUS: NÃO REGISTRADO"},
        {"Capture this Pokémon in your save files", "Capture este Pokémon no seu save"},
        {"to unlock full registration & stats.", "para liberar as estatísticas e registro."},
        {"[A] Boot Dex   [◄ / ►] Select Game   [L / R] Change Profile   [-] Language   [+] Exit", "[A] Iniciar Dex   [◄ / ►] Escolher Jogo   [L / R] Mudar Perfil   [-] Idioma   [+] Sair"},
        {"[A] View Detail   [ZL / ZR] Select Game   [L / R] Page Jump   [-] Language   [+] Exit", "[A] Detalhes   [ZL / ZR] Escolher Jogo   [L / R] Pular Página   [-] Idioma   [+] Sair"}
    };
    auto it = ptBR.find(key);
    return (it != ptBR.end()) ? it->second : key;
}
SDL_Texture* PokedexUI::loadTexture(const std::string& path) {
    if (path.empty()) return nullptr;
    auto it = m_textureCache.find(path);
    if (it != m_textureCache.end()) {
        return it->second;
    }
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) return nullptr;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FreeSurface(surface);
    if (texture) {
        m_textureCache[path] = texture;
    }
    return texture;
}
void PokedexUI::handleInput(
    uint64_t keysDown,
    uint64_t keysHeld,
    std::function<void(DexGame, CatchFilterMode, const std::string&)> onFilterChanged
) {
    if ((keysHeld & HidNpadButton_L) && (keysHeld & HidNpadButton_R) && (keysDown & HidNpadButton_Plus)) {
        for (auto& entry : m_entries) {
            entry.isUnlocked = true;
        }
        return;
    }
    if (keysDown & HidNpadButton_Plus) {
        m_shouldClose = true;
        return;
    }
    if (keysDown & HidNpadButton_Minus) {
        toggleLanguage();
    }
    if (m_isAnimating) return;
    if (m_currentView == AppView::TitleScreen) {
        if (keysDown & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_X | HidNpadButton_Y | HidNpadButton_Plus | HidNpadButton_L | HidNpadButton_R | HidNpadButton_ZL | HidNpadButton_ZR)) {
            m_targetView = AppView::HomeHub;
            m_isAnimating = true;
            m_animFrame = 0;
        }
    } else if (m_currentView == AppView::HomeHub) {
        int totalGames = static_cast<int>(m_gameCards.size());
        if (totalGames == 0) return;
        if (keysDown & HidNpadButton_Right || keysDown & HidNpadButton_StickLRight) {
            m_selectedGameIndex = (m_selectedGameIndex + 1) % totalGames;
        } else if (keysDown & HidNpadButton_Left || keysDown & HidNpadButton_StickLLeft) {
            m_selectedGameIndex = (m_selectedGameIndex == 0) ? (totalGames - 1) : (m_selectedGameIndex - 1);
        } else if (keysDown & HidNpadButton_A) {
            m_selectedGame = m_gameCards[m_selectedGameIndex].gameId;
            m_targetView = AppView::GamePokedex;
            m_isAnimating = true;
            m_animFrame = 0;
            if (onFilterChanged) {
                onFilterChanged(m_selectedGame, m_catchFilter, m_currentTypeFilter);
            }
        }
    } else if (m_currentView == AppView::PokemonDetail) {
        if (keysDown & HidNpadButton_B) {
            m_targetView = AppView::GamePokedex;
            m_isAnimating = true;
            m_animFrame = 0;
            return;
        }
    } else {
        if (keysDown & HidNpadButton_B) {
            m_targetView = AppView::HomeHub;
            m_isAnimating = true;
            m_animFrame = 0;
            return;
        }
        if (keysDown & HidNpadButton_Y) {
            int nextFilter = (static_cast<int>(m_catchFilter) + 1) % static_cast<int>(CatchFilterMode::Count);
            m_catchFilter = static_cast<CatchFilterMode>(nextFilter);
            if (onFilterChanged) onFilterChanged(m_selectedGame, m_catchFilter, m_currentTypeFilter);
            return;
        }
        if (keysDown & HidNpadButton_X) {
            m_typeFilterIndex = (m_typeFilterIndex + 1) % static_cast<int>(ELEMENT_TYPES.size());
            m_currentTypeFilter = ELEMENT_TYPES[m_typeFilterIndex];
            if (onFilterChanged) onFilterChanged(m_selectedGame, m_catchFilter, m_currentTypeFilter);
            return;
        }
        if (keysDown & HidNpadButton_ZL) {
            int totalGames = static_cast<int>(m_gameCards.size());
            m_selectedGameIndex = (m_selectedGameIndex == 0) ? (totalGames - 1) : (m_selectedGameIndex - 1);
            m_selectedGame = m_gameCards[m_selectedGameIndex].gameId;
            m_isAnimating = true; m_animFrame = 0; m_targetView = AppView::GamePokedex;
            if (onFilterChanged) onFilterChanged(m_selectedGame, m_catchFilter, m_currentTypeFilter);
            return;
        } else if (keysDown & HidNpadButton_ZR) {
            int totalGames = static_cast<int>(m_gameCards.size());
            m_selectedGameIndex = (m_selectedGameIndex + 1) % totalGames;
            m_selectedGame = m_gameCards[m_selectedGameIndex].gameId;
            m_isAnimating = true; m_animFrame = 0; m_targetView = AppView::GamePokedex;
            if (onFilterChanged) onFilterChanged(m_selectedGame, m_catchFilter, m_currentTypeFilter);
            return;
        }
        if (m_entries.empty()) return;
        int total = static_cast<int>(m_entries.size());
        if (keysDown & HidNpadButton_Right || keysDown & HidNpadButton_StickLRight) {
            if (m_selectedIndex + 1 < total) m_selectedIndex++;
        } else if (keysDown & HidNpadButton_Left || keysDown & HidNpadButton_StickLLeft) {
            if (m_selectedIndex - 1 >= 0) m_selectedIndex--;
        } else if (keysDown & HidNpadButton_Down || keysDown & HidNpadButton_StickLDown) {
            if (m_selectedIndex + GRID_COLS < total) m_selectedIndex += GRID_COLS;
        } else if (keysDown & HidNpadButton_Up || keysDown & HidNpadButton_StickLUp) {
            if (m_selectedIndex - GRID_COLS >= 0) m_selectedIndex -= GRID_COLS;
        } else if (keysDown & HidNpadButton_L) {
            m_selectedIndex = std::max(0, m_selectedIndex - PAGE_SIZE);
        } else if (keysDown & HidNpadButton_R) {
            m_selectedIndex = std::min(total - 1, m_selectedIndex + PAGE_SIZE);
        } else if (keysDown & HidNpadButton_A) {
            m_targetView = AppView::PokemonDetail;
            m_isAnimating = true;
            m_animFrame = 0;
            return;
        }
        int selectedRow = m_selectedIndex / GRID_COLS;
        int visibleStartRow = m_scrollOffset / GRID_COLS;
        int visibleEndRow = visibleStartRow + GRID_ROWS - 1;
        if (selectedRow < visibleStartRow) {
            m_scrollOffset = selectedRow * GRID_COLS;
        } else if (selectedRow > visibleEndRow) {
            m_scrollOffset = (selectedRow - GRID_ROWS + 1) * GRID_COLS;
        }
    }
}
void PokedexUI::handleTouch(
    const HidTouchScreenState& state,
    std::function<void(DexGame, CatchFilterMode, const std::string&)> onFilterChanged
) {
    if (state.count == 0) {
        m_touchDown = false;
        return;
    }
    if (m_touchDown) return;
    m_touchDown = true;
    if (m_isAnimating) return;
    int touchX = state.touches[0].x;
    int touchY = state.touches[0].y;
    if (m_currentView == AppView::TitleScreen) {
        m_targetView = AppView::HomeHub;
        m_isAnimating = true;
        m_animFrame = 0;
        return;
    } else if (m_currentView == AppView::HomeHub) {
        int startX = 60;
        int cardY = 160;
        int cardW = 216;
        int cardH = 480;
        int gap = 20;
        for (size_t i = 0; i < m_gameCards.size(); ++i) {
            int cardX = startX + static_cast<int>(i) * (cardW + gap);
            if (touchX >= cardX && touchX <= cardX + cardW && touchY >= cardY && touchY <= cardY + cardH) {
                m_selectedGameIndex = static_cast<int>(i);
                m_selectedGame = m_gameCards[m_selectedGameIndex].gameId;
                m_targetView = AppView::GamePokedex;
                m_isAnimating = true;
                m_animFrame = 0;
                if (onFilterChanged) {
                    onFilterChanged(m_selectedGame, m_catchFilter, m_currentTypeFilter);
                }
                return;
            }
        }
    } else {
        if (touchX >= 765 && touchX <= 915 && touchY >= 22 && touchY <= 56) {
            m_targetView = AppView::HomeHub;
            m_isAnimating = true;
            m_animFrame = 0;
            return;
        }
        if (touchX >= 400 && touchX <= 575 && touchY >= 22 && touchY <= 56) {
            int nextFilter = (static_cast<int>(m_catchFilter) + 1) % static_cast<int>(CatchFilterMode::Count);
            m_catchFilter = static_cast<CatchFilterMode>(nextFilter);
            if (onFilterChanged) onFilterChanged(m_selectedGame, m_catchFilter, m_currentTypeFilter);
            return;
        }
        if (touchX >= 590 && touchX <= 750 && touchY >= 22 && touchY <= 56) {
            m_typeFilterIndex = (m_typeFilterIndex + 1) % static_cast<int>(ELEMENT_TYPES.size());
            m_currentTypeFilter = ELEMENT_TYPES[m_typeFilterIndex];
            if (onFilterChanged) onFilterChanged(m_selectedGame, m_catchFilter, m_currentTypeFilter);
            return;
        }
        int startX = 40;
        int startY = 104;
        int cardW = 136;
        int cardH = 136;
        int gapX = 14;
        int gapY = 12;
        int total = static_cast<int>(m_entries.size());
        for (int i = 0; i < PAGE_SIZE; ++i) {
            int entryIndex = m_scrollOffset + i;
            if (entryIndex >= total) break;
            int col = i % GRID_COLS;
            int row = i / GRID_COLS;
            int cardX = startX + col * (cardW + gapX);
            int cardY = startY + row * (cardH + gapY);
            if (touchX >= cardX && touchX <= cardX + cardW && touchY >= cardY && touchY <= cardY + cardH) {
                m_selectedIndex = entryIndex;
                return;
            }
        }
    }
}
void PokedexUI::drawText(const std::string& text, int x, int y, SDL_Color color, TTF_Font* font, bool center) {
    if (!font || text.empty()) return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, surf);
    if (tex) {
        SDL_Rect dst;
        dst.w = surf->w;
        dst.h = surf->h;
        dst.x = center ? (x - dst.w / 2) : x;
        dst.y = center ? (y - dst.h / 2) : y;
        SDL_RenderCopy(m_renderer, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

void PokedexUI::drawTextWrapped(const std::string& text, int x, int y, Uint32 wrapLength, SDL_Color color, TTF_Font* font) {
    if (!font || text.empty()) return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), color, wrapLength);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, surf);
    if (tex) {
        SDL_Rect dst = { x, y, surf->w, surf->h };
        SDL_RenderCopy(m_renderer, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}
void PokedexUI::drawProgressBar(int x, int y, int width, int height, int value, int maxValue, SDL_Color color) {
    SDL_SetRenderDrawColor(m_renderer, 35, 42, 54, 255);
    SDL_Rect bgRect = { x, y, width, height };
    SDL_RenderFillRect(m_renderer, &bgRect);
    if (maxValue <= 0) maxValue = 1;
    float ratio = std::clamp(static_cast<float>(value) / static_cast<float>(maxValue), 0.0f, 1.0f);
    int fillWidth = static_cast<int>(width * ratio);
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, 255);
    SDL_Rect fillRect = { x, y, fillWidth, height };
    SDL_RenderFillRect(m_renderer, &fillRect);
}
void PokedexUI::renderTitleScreen() {
    SDL_Color pokedexRed = { 227, 53, 13, 255 };
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_SetRenderDrawColor(m_renderer, pokedexRed.r, pokedexRed.g, pokedexRed.b, 255);
    SDL_RenderClear(m_renderer);
    int circleSize = 250;
    SDL_SetRenderDrawColor(m_renderer, 30, 30, 35, 255);
    SDL_Rect outerCircle = { SCREEN_WIDTH / 2 - circleSize / 2, SCREEN_HEIGHT / 2 - circleSize / 2 - 50, circleSize, circleSize };
    SDL_RenderFillRect(m_renderer, &outerCircle);
    int innerSize = 160;
    SDL_SetRenderDrawColor(m_renderer, white.r, white.g, white.b, 255);
    SDL_Rect innerCircle = { SCREEN_WIDTH / 2 - innerSize / 2, SCREEN_HEIGHT / 2 - innerSize / 2 - 50, innerSize, innerSize };
    SDL_RenderFillRect(m_renderer, &innerCircle);
    drawText("Switchdex", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 150, white, m_fontHuge, true);
    static int blinkTimer = 0;
    blinkTimer = (blinkTimer + 1) % 60;
    if (blinkTimer < 30) {
        drawText(t("Press Any Button to Start"), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 220, white, m_fontLarge, true);
    }
    drawText("[-] " + t("Language"), SCREEN_WIDTH / 2, SCREEN_HEIGHT - 40, white, m_fontSmall, true);
}
void PokedexUI::renderHomeHub() {
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color lightGray = { 220, 220, 225, 255 };
    SDL_Color gray = { 150, 150, 160, 255 };
    SDL_Color pokedexRed = { 227, 53, 13, 255 };
    SDL_Color darkRed = { 180, 20, 10, 255 };
    SDL_Color bgDark = { 240, 240, 245, 255 };
    SDL_Color cardBg = { 255, 255, 255, 255 };
    SDL_Color textGreen = { 52, 199, 89, 255 };
    SDL_Color darkText = { 40, 40, 45, 255 };
    SDL_SetRenderDrawColor(m_renderer, pokedexRed.r, pokedexRed.g, pokedexRed.b, 255);
    SDL_Rect headerRect = { 0, 0, SCREEN_WIDTH, 120 };
    SDL_RenderFillRect(m_renderer, &headerRect);
    SDL_SetRenderDrawColor(m_renderer, darkRed.r, darkRed.g, darkRed.b, 255);
    SDL_Rect headerShadow = { 0, 120, SCREEN_WIDTH, 4 };
    SDL_RenderFillRect(m_renderer, &headerShadow);
    drawText("Switchdex", 60, 45, white, m_fontHuge);
    drawText(t("Select a Game Pokédex Module"), 60, 85, lightGray, m_fontMedium);
    SDL_SetRenderDrawColor(m_renderer, darkRed.r, darkRed.g, darkRed.b, 255);
    SDL_Rect profileRect = { SCREEN_WIDTH - 280, 30, 240, 50 };
    SDL_RenderFillRect(m_renderer, &profileRect);
    SDL_SetRenderDrawColor(m_renderer, white.r, white.g, white.b, 255);
    SDL_RenderDrawRect(m_renderer, &profileRect);
    drawText(t("Profile: ") + m_profileName, SCREEN_WIDTH - 160, 55, white, m_fontMedium, true);
    int startX = 60;
    int cardY = 160;
    int cardW = 216;
    int cardH = 480;
    int gap = 20;
    for (size_t i = 0; i < m_gameCards.size(); ++i) {
        const auto& game = m_gameCards[i];
        int cardX = startX + static_cast<int>(i) * (cardW + gap);
        bool isSelected = (static_cast<int>(i) == m_selectedGameIndex);
        SDL_SetRenderDrawColor(m_renderer, 200, 200, 210, 255);
        SDL_Rect shadowRect = { cardX + 4, cardY + 4, cardW, cardH };
        SDL_RenderFillRect(m_renderer, &shadowRect);
        SDL_SetRenderDrawColor(m_renderer, cardBg.r, cardBg.g, cardBg.b, 255);
        SDL_Rect cardRect = { cardX, cardY, cardW, cardH };
        SDL_RenderFillRect(m_renderer, &cardRect);
        SDL_SetRenderDrawColor(m_renderer, isSelected ? pokedexRed.r : 220, isSelected ? pokedexRed.g : 220, isSelected ? pokedexRed.b : 225, 255);
        SDL_RenderDrawRect(m_renderer, &cardRect);
        if (isSelected) {
            SDL_Rect innerBorder = { cardX + 1, cardY + 1, cardW - 2, cardH - 2 };
            SDL_RenderDrawRect(m_renderer, &innerBorder);
            SDL_Rect innerBorder2 = { cardX + 2, cardY + 2, cardW - 4, cardH - 4 };
            SDL_RenderDrawRect(m_renderer, &innerBorder2);
        }
        SDL_Texture* coverTex = loadTexture(game.coverPath);
        if (coverTex) {
            SDL_Rect coverRect = { cardX + 8, cardY + 8, cardW - 16, 200 };
            SDL_RenderCopy(m_renderer, coverTex, nullptr, &coverRect);
        } else {
            SDL_SetRenderDrawColor(m_renderer, 230, 230, 235, 255);
            SDL_Rect coverRect = { cardX + 8, cardY + 8, cardW - 16, 200 };
            SDL_RenderFillRect(m_renderer, &coverRect);
            drawText(t("NO IMAGE"), cardX + cardW / 2, cardY + 100, gray, m_fontSmall, true);
        }
        drawText(game.title, cardX + cardW / 2, cardY + 230, darkText, m_fontMedium, true);
        drawText(game.subtitle, cardX + cardW / 2, cardY + 260, gray, m_fontSmall, true);
        SDL_SetRenderDrawColor(m_renderer, 230, 230, 235, 255);
        SDL_Rect separator = { cardX + 20, cardY + 290, cardW - 40, 1 };
        SDL_RenderFillRect(m_renderer, &separator);
        if (game.hasSaveData) {
            drawText(t("Save Detected"), cardX + cardW / 2, cardY + 310, textGreen, m_fontSmall, true);
        } else {
            drawText(t("No Save Found"), cardX + cardW / 2, cardY + 310, gray, m_fontSmall, true);
        }
        std::ostringstream progText;
        progText << game.caughtCount << " / " << game.totalCount;
        drawText(progText.str(), cardX + cardW / 2, cardY + 360, darkText, m_fontSmall, true);
        drawProgressBar(cardX + 30, cardY + 385, cardW - 60, 4, game.caughtCount, game.totalCount, isSelected ? pokedexRed : lightGray);
        int pct = (game.totalCount > 0) ? (game.caughtCount * 100 / game.totalCount) : 0;
        drawText(std::to_string(pct) + "%", cardX + cardW / 2, cardY + 405, gray, m_fontSmall, true);
        if (isSelected) {
            SDL_Rect enterBtn = { cardX + 30, cardY + 430, cardW - 60, 36 };
            SDL_SetRenderDrawColor(m_renderer, pokedexRed.r, pokedexRed.g, pokedexRed.b, 255);
            SDL_RenderFillRect(m_renderer, &enterBtn);
            drawText(t("BOOT DEX"), cardX + cardW / 2, cardY + 448, SDL_Color{255, 255, 255, 255}, m_fontSmall, true);
        }
    }
    SDL_Color darkHints = { 100, 100, 110, 255 };
    drawText(t("[A] Boot Dex   [◄ / ►] Select Game   [L / R] Change Profile   [-] Language   [+] Exit"), SCREEN_WIDTH / 2, 675, darkHints, m_fontSmall, true);
}
void PokedexUI::renderFilterHeader() {
    SDL_Color pokedexRed = { 227, 53, 13, 255 };
    SDL_Color darkRed = { 180, 20, 10, 255 };
    SDL_SetRenderDrawColor(m_renderer, pokedexRed.r, pokedexRed.g, pokedexRed.b, 255);
    SDL_Rect topBar = { 0, 0, SCREEN_WIDTH, 84 };
    SDL_RenderFillRect(m_renderer, &topBar);
    SDL_SetRenderDrawColor(m_renderer, darkRed.r, darkRed.g, darkRed.b, 255);
    SDL_Rect accent = { 0, 82, SCREEN_WIDTH, 4 };
    SDL_RenderFillRect(m_renderer, &accent);
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color lightRed = { 255, 120, 120, 255 };
    SDL_Color accentColor = { 255, 255, 255, 255 };
    SDL_Color yellow = { 255, 214, 10, 255 };
    std::string gameTitle = (m_selectedGame == DexGame::National) ? t("National Pokédex") :
                            (m_selectedGame == DexGame::SwordShield) ? "Sword & Shield" :
                            (m_selectedGame == DexGame::ScarletViolet) ? "Scarlet & Violet" :
                            (m_selectedGame == DexGame::LegendsArceus) ? "Legends: Arceus" :
                            (m_selectedGame == DexGame::LegendsZA) ? "Legends: Z-A" : "BD & SP";
    drawText(gameTitle, 40, 25, white, m_fontLarge);
    std::string catchFilterStr = (m_catchFilter == CatchFilterMode::All) ? t("ALL") :
                                 (m_catchFilter == CatchFilterMode::CaughtOnly) ? t("CAUGHT ONLY") : t("UNCAUGHT ONLY");
    int filterX = 400;
    int filterY = 22;
    int filterH = 34;
    SDL_Rect yBadge = { filterX, filterY, 175, filterH };
    SDL_SetRenderDrawColor(m_renderer, darkRed.r, darkRed.g, darkRed.b, 255);
    SDL_RenderFillRect(m_renderer, &yBadge);
    SDL_SetRenderDrawColor(m_renderer, white.r, white.g, white.b, 255);
    SDL_RenderDrawRect(m_renderer, &yBadge);
    drawText(t("Status: ") + catchFilterStr, filterX + 87, filterY + 17, white, m_fontSmall, true);
    filterX += 190;
    SDL_Rect xBadge = { filterX, filterY, 160, filterH };
    SDL_SetRenderDrawColor(m_renderer, darkRed.r, darkRed.g, darkRed.b, 255);
    SDL_RenderFillRect(m_renderer, &xBadge);
    SDL_SetRenderDrawColor(m_renderer, yellow.r, yellow.g, yellow.b, 255);
    SDL_RenderDrawRect(m_renderer, &xBadge);
    drawText(t("Type: ") + (m_currentTypeFilter == "ALL" ? t("ALL") : m_currentTypeFilter), filterX + 80, filterY + 17, yellow, m_fontSmall, true);
    filterX += 175;
    SDL_Rect bBadge = { filterX, filterY, 150, filterH };
    SDL_SetRenderDrawColor(m_renderer, darkRed.r, darkRed.g, darkRed.b, 255);
    SDL_RenderFillRect(m_renderer, &bBadge);
    drawText(t("[B] Game Menu"), filterX + 75, filterY + 17, lightRed, m_fontSmall, true);
    size_t unlockedCount = 0;
    for (const auto& e : m_entries) {
        if (e.isUnlocked) unlockedCount++;
    }
    std::ostringstream countStr;
    countStr << t("Registered: ") << unlockedCount << " / " << m_entries.size();
    drawText(countStr.str(), 1110, 28, white, m_fontMedium);
}
void PokedexUI::renderGrid() {
    int startX = 40;
    int startY = 104;
    int cardW = 136;
    int cardH = 136;
    int gapX = 14;
    int gapY = 12;
    int total = static_cast<int>(m_entries.size());
    for (int i = 0; i < PAGE_SIZE; ++i) {
        int entryIndex = m_scrollOffset + i;
        if (entryIndex >= total) break;
        const auto& entry = m_entries[entryIndex];
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;
        int cardX = startX + col * (cardW + gapX);
        int cardY = startY + row * (cardH + gapY);
        bool isSelected = (entryIndex == m_selectedIndex);
        if (isSelected) {
            SDL_SetRenderDrawColor(m_renderer, 50, 68, 95, 255);
        } else {
            SDL_SetRenderDrawColor(m_renderer, 26, 30, 40, 255);
        }
        SDL_Rect cardRect = { cardX, cardY, cardW, cardH };
        SDL_RenderFillRect(m_renderer, &cardRect);
        if (isSelected) {
            SDL_SetRenderDrawColor(m_renderer, 79, 172, 254, 255);
            SDL_RenderDrawRect(m_renderer, &cardRect);
        }
        SDL_Texture* tex = loadTexture(entry.spritePath);
        SDL_Rect spriteRect = { cardX + (cardW - 74) / 2, cardY + 10, 74, 74 };
        if (tex) {
            if (entry.isUnlocked) {
                SDL_SetTextureColorMod(tex, 255, 255, 255);
            } else {
                SDL_SetTextureColorMod(tex, 15, 15, 20);
            }
            SDL_RenderCopy(m_renderer, tex, nullptr, &spriteRect);
        } else {
            SDL_SetRenderDrawColor(m_renderer, entry.isUnlocked ? 45 : 22, entry.isUnlocked ? 60 : 25, entry.isUnlocked ? 80 : 32, 255);
            SDL_RenderFillRect(m_renderer, &spriteRect);
            SDL_SetRenderDrawColor(m_renderer, entry.isUnlocked ? 79 : 40, entry.isUnlocked ? 172 : 50, entry.isUnlocked ? 254 : 65, 255);
            SDL_Rect inner = { spriteRect.x + 18, spriteRect.y + 18, 38, 38 };
            SDL_RenderDrawRect(m_renderer, &inner);
        }
        std::ostringstream numStr;
        numStr << "#" << std::setw(3) << std::setfill('0') << entry.speciesId;
        SDL_Color tagColor = isSelected ? SDL_Color{255, 255, 255, 255} : SDL_Color{140, 150, 165, 255};
        drawText(numStr.str(), cardX + cardW / 2, cardY + 94, tagColor, m_fontSmall, true);
        std::string displayName = entry.isUnlocked ? entry.name : "???";
        SDL_Color nameColor = entry.isUnlocked ? SDL_Color{230, 235, 245, 255} : SDL_Color{90, 100, 115, 255};
        drawText(displayName, cardX + cardW / 2, cardY + 114, nameColor, m_fontSmall, true);
    }
}
void PokedexUI::renderDetailPanel() {
    int panelX = 800;
    int panelY = 104;
    int panelW = 440;
    int panelH = 585;
    SDL_SetRenderDrawColor(m_renderer, 24, 28, 36, 255);
    SDL_Rect panelRect = { panelX, panelY, panelW, panelH };
    SDL_RenderFillRect(m_renderer, &panelRect);
    SDL_SetRenderDrawColor(m_renderer, 42, 48, 62, 255);
    SDL_RenderDrawRect(m_renderer, &panelRect);
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_entries.size())) {
        return;
    }
    const auto& current = m_entries[m_selectedIndex];
    SDL_Texture* bigTex = loadTexture(current.spritePath);
    SDL_Rect bigSpriteRect = { panelX + (panelW - 120) / 2, panelY + 15, 120, 120 };
    if (bigTex) {
        if (current.isUnlocked) {
            SDL_SetTextureColorMod(bigTex, 255, 255, 255);
        } else {
            SDL_SetTextureColorMod(bigTex, 15, 15, 20);
        }
        SDL_RenderCopy(m_renderer, bigTex, nullptr, &bigSpriteRect);
    }
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color gray = { 155, 170, 190, 255 };
    SDL_Color cyan = { 79, 172, 254, 255 };
    SDL_Color green = { 67, 233, 123, 255 };
    SDL_Color orange = { 255, 154, 60, 255 };
    if (current.isUnlocked) {
        std::ostringstream title;
        title << "#" << std::setw(3) << std::setfill('0') << current.speciesId << " " << current.name;
        drawText(title.str(), panelX + 28, panelY + 150, white, m_fontLarge);
        std::string typeStr = current.type1 + (current.type2.empty() ? "" : " / " + current.type2);
        drawText(t("Region: ") + current.region, panelX + 28, panelY + 188, cyan, m_fontMedium);
        drawText(t("Type: ") + typeStr, panelX + 28, panelY + 215, gray, m_fontMedium);
        drawText(t("Pokédex Entry:"), panelX + 28, panelY + 252, white, m_fontMedium);
        drawTextWrapped(current.description, panelX + 28, panelY + 280, 380, gray, m_fontSmall);
        int statsY = panelY + 340;
        drawText(t("BASE STATS"), panelX + 28, statsY, white, m_fontMedium);
        auto renderStatLine = [&](const char* label, int val, int yOffset, SDL_Color color) {
            drawText(t(label), panelX + 28, statsY + yOffset, gray, m_fontSmall);
            drawProgressBar(panelX + 110, statsY + yOffset + 2, 220, 14, val, 200, color);
            drawText(std::to_string(val), panelX + 345, statsY + yOffset, white, m_fontSmall);
        };
        renderStatLine("HP", current.hp, 26, green);
        renderStatLine("Attack", current.attack, 50, orange);
        renderStatLine("Defense", current.defense, 74, cyan);
        renderStatLine("Sp. Atk", current.spAttack, 98, orange);
        renderStatLine("Sp. Def", current.spDefense, 122, cyan);
        renderStatLine("Speed", current.speed, 146, green);
    } else {
        std::ostringstream title;
        title << "#" << std::setw(3) << std::setfill('0') << current.speciesId << " ???";
        drawText(title.str(), panelX + 28, panelY + 150, gray, m_fontLarge);
        drawText(t("STATUS: NOT REGISTERED"), panelX + 28, panelY + 200, SDL_Color{235, 60, 60, 255}, m_fontMedium);
        drawText(t("Capture this Pokémon in your save files"), panelX + 28, panelY + 245, gray, m_fontSmall);
        drawText(t("to unlock full registration & stats."), panelX + 28, panelY + 270, gray, m_fontSmall);
    }
}
void PokedexUI::renderFooterHints() {
    SDL_Color gray = { 150, 160, 180, 255 };
    drawText(t("[A] View Detail   [ZL / ZR] Select Game   [L / R] Page Jump   [-] Language   [+] Exit"), SCREEN_WIDTH / 2, 675, gray, m_fontSmall, true);
}
void PokedexUI::renderGamePokedex() {
    renderFilterHeader();
    renderGrid();
    renderDetailPanel();
}
void PokedexUI::render() {
    if (m_isAnimating) {
        m_animFrame++;
        if (m_animFrame == 10) {
            m_currentView = m_targetView;
        }
    }
    if (m_currentView == AppView::TitleScreen) {
        SDL_SetRenderDrawColor(m_renderer, 227, 53, 13, 255);
    } else if (m_currentView == AppView::HomeHub) {
        SDL_SetRenderDrawColor(m_renderer, 240, 240, 245, 255);
    } else if (m_currentView == AppView::PokemonDetail) {
        SDL_SetRenderDrawColor(m_renderer, 25, 28, 35, 255);
    } else {
        SDL_SetRenderDrawColor(m_renderer, 15, 17, 23, 255);
    }
    SDL_RenderClear(m_renderer);
    if (m_currentView == AppView::TitleScreen) {
        renderTitleScreen();
    } else if (m_currentView == AppView::HomeHub) {
        renderHomeHub();
    } else if (m_currentView == AppView::PokemonDetail) {
        renderPokemonDetail();
    } else {
        renderGamePokedex();
    }
    if (m_isAnimating) {
        int maxFrames = 20;
        float progress = 0.0f;
        if (m_animFrame <= 10) {
            progress = (float)m_animFrame / 10.0f;
        } else {
            progress = 1.0f - ((float)(m_animFrame - 10) / 10.0f);
        }
        
        SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
        int alpha = (int)(255.0f * progress);
        if (alpha > 255) alpha = 255;
        if (alpha < 0) alpha = 0;
        
        SDL_SetRenderDrawColor(m_renderer, 10, 12, 18, alpha);
        SDL_Rect fullScreen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
        SDL_RenderFillRect(m_renderer, &fullScreen);
        SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
        
        if (m_animFrame >= maxFrames) {
            m_isAnimating = false;
        }
    }
    SDL_RenderPresent(m_renderer);
}

void PokedexUI::renderPokemonDetail() {
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color gray = { 150, 150, 160, 255 };
    SDL_Color typeColor = { 100, 100, 255, 255 };
    
    SDL_SetRenderDrawColor(m_renderer, 25, 28, 35, 255);
    SDL_RenderClear(m_renderer);
    
    if (m_entries.empty() || m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_entries.size())) return;
    const auto& entry = m_entries[m_selectedIndex];
    
    drawText("#" + std::to_string(entry.speciesId) + " " + entry.name, 50, 30, white, m_fontHuge);
    drawText("[B] " + t("Back"), SCREEN_WIDTH - 150, 40, gray, m_fontMedium);
    
    SDL_Texture* tex = loadTexture(entry.spritePath);
    if (tex) {
        SDL_Rect srcRect = { 0, 0, 120, 120 };
        SDL_Rect dstRect = { 100, 150, 360, 360 };
        SDL_RenderCopy(m_renderer, tex, &srcRect, &dstRect);
    }
    
    int infoX = 500;
    int infoY = 150;
    
    drawText("Type: " + entry.type1 + (entry.type2.empty() ? "" : " / " + entry.type2), infoX, infoY, white, m_fontLarge);
    drawText("Region: " + entry.region, infoX, infoY + 50, gray, m_fontMedium);
    drawTextWrapped(entry.description, infoX, infoY + 90, 700, gray, m_fontMedium);
    
    int statsY = infoY + 200;
    drawText("Abilities:", infoX, statsY, white, m_fontMedium);
    std::string abStr = entry.ability1;
    if (!entry.ability2.empty()) abStr += " / " + entry.ability2;
    if (!entry.hiddenAbility.empty()) abStr += " (Hidden: " + entry.hiddenAbility + ")";
    drawText(abStr, infoX + 20, statsY + 30, gray, m_fontSmall);
    if (entry.evolveLevel > 0) {
        drawText("Evolves at level: " + std::to_string(entry.evolveLevel), infoX + 20, statsY + 60, gray, m_fontSmall);
    }
    
    int saveY = statsY + 110;
    drawText("Save File Details:", infoX, saveY, white, m_fontMedium);
    
    if (entry.isUnlocked && entry.saveExp > 0) {
        drawText("EXP: " + std::to_string(entry.saveExp) + " (Growth: " + entry.growthRate + ")", infoX + 20, saveY + 30, typeColor, m_fontSmall);
        std::string activeAb = "Unknown";
        if (entry.saveAbilityIndex == 0) activeAb = entry.ability1;
        else if (entry.saveAbilityIndex == 1) activeAb = (!entry.ability2.empty() ? entry.ability2 : entry.ability1);
        else if (entry.saveAbilityIndex == 2) activeAb = entry.hiddenAbility;
        drawText("Active Ability: " + activeAb, infoX + 20, saveY + 60, typeColor, m_fontSmall);
    } else if (entry.isUnlocked) {
        drawText("Caught, but detailed data not available in this save.", infoX + 20, saveY + 30, gray, m_fontSmall);
    } else {
        drawText("Not Caught in this save.", infoX + 20, saveY + 30, gray, m_fontSmall);
    }
}

}

