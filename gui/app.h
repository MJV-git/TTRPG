#pragma once
#include "draw.h"
#include "game.h"
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Gui {
    struct Button { std::string id; SDL_Rect rect; bool enabled; std::function<void()> action; std::string tip; };
    struct Recruit { int preset = 0; std::string name; std::array<int, 5> stats {{4, 4, 4, 4, 4}}; int startingWeapon = 1; };
    enum class Screen { title, setup, battle };
    enum class Modal { none, archive, help, inventory, custom, pause, journal, trade, market, bridge };
    enum class Presentation { idle, action, handoff, drawing, flipping, card, worldEffect,
                              enemyIntro, enemyPreview, enemyResult };
    SDL_Window* window;
    SDL_Renderer* renderer;
    ui::Draw draw;
    std::vector<Button> buttons;
    std::optional<Game> game;
    std::optional<Game> tutorialReturnGame;
    std::vector<std::string> tutorialReturnHistory;
    int tutorialStep = -1;
    bool tutorialPracticed = false;
    Screen screen = Screen::title;
    Modal modal = Modal::none;
    std::array<Recruit, 4> recruits;
    std::vector<std::string> history;
    std::vector<std::string> recentLog;
    std::optional<Game> beforeAnimation;
    Presentation presentation = Presentation::idle;
    Uint32 animationClock = 0, stageStarted = 0, lastTick = 0;
    EnemyIntent intent;
    std::string actionLabel;
    ActionType animatedAction = ActionType::wait;
    int actingPlayer = 0;
    bool autoSave = false, fastAnimations = false;
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> cardFace {nullptr, SDL_DestroyTexture};
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> cardBack {nullptr, SDL_DestroyTexture};
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> menuArtwork {nullptr, SDL_DestroyTexture};
    int cachedCard = -2;
    std::string savePath = "campaign.sav", archivePath = "campaign.sav", seedText;
    std::string notice, focusId;
    std::string* focused = nullptr;
    int focusLimit = 32, mouseX = -1, mouseY = -1;
    int lastQuickPreset = -1, quickPlayerCount = 1;
    int inventoryPage = 0;
    int tradeRecipient = 0, tradePage = 0;
    Uint32 equippedAt = 0;
    int selectedBoss = 0, squadSize = 1, customRecruit = 0;
    int selectedSector = 1, selectedTarget = 0, targetPage = 0, logOffset = 0, helpPage = 0;
    bool running = true, noticeError = false;
    std::uint32_t initialSeed;
    bool fixedSeed;
    int frames = 0;

    void button(const std::string& id, SDL_Rect box, const std::string& label,
                std::function<void()> action, bool enabled = true, bool primary = false, const std::string& tip = {});
    void field(const std::string& id, SDL_Rect box, std::string& value, int limit, const std::string& placeholder);
    void heading(const std::string& eyebrow, const std::string& title, const std::string& subtitle);
    void toast(const std::string& text, bool error = false);
    void clearFocus();
    void openModal(Modal value);
    void titleScreen();
    void startTutorial();
    void exitTutorial();
    void setTutorialStep(int step);
    void tutorialOverlay();
    void setupScreen();
    void battleScreen();
    void board();
    void sectorPanel();
    void defenderPanel();
    void logPanel();
    void modalScreen();
    void archiveModal();
    void helpModal();
    void inventoryModal();
    void tradeModal();
    void marketModal();
    void bridgeModal();
    void customModal();
    void journalModal();
    void pauseModal();
    void endOverlay();
    void gatherLog();
    void perform(Action action);
    void selectSector(int id);
    void syncSelection();
    bool save(bool announce = false);
    void load(const std::string& path);
    void startQuick();
    void cycleRecruit(int index, int direction);
    void deploy();
    void quit();
    void leaveWithoutSaving(bool toTitle);
    void beginStage(Presentation value);
    void resumePresentation();
    float animationScale() const { return fastAnimations ? 0.25F : 1.25F; }
    void updatePresentation(Uint32 now);
    void advanceGamePhase();
    void prepareEnemy();
    void finishInternalRound();
    bool acceptsOrders() const;
    void phaseRail();
    void presentationOverlay();
    void boardEffects();
    void combatAnimation();
    ui::Icon actorIcon(const std::string& name, bool defender) const;
    void actionAnimation();
    SDL_Texture* eventTexture(bool front);
    void eventArtwork(int art, int x, int y);
    void settleTest(bool acknowledge = true, const std::string& enemyScreenshot = {});
    void capture(const std::string& path);
    void clickTest(const std::string& id);
    void expect(bool condition, const std::string& message);

public:
    Gui(SDL_Window* window, SDL_Renderer* renderer, const std::string& font, std::uint32_t seed, bool fixed);
    void handle(const SDL_Event& event);
    void render(bool present = true);
    void run();
    void quick() { startQuick(); }
    void restore(const std::string& path) { load(path); }
    bool isPlaying() const { return game.has_value(); }
    void screenshot(const std::string& path) { render(false); capture(path); SDL_RenderPresent(renderer); }
    void smokeTest(const std::string& screenshotPath);
};
