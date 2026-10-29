#pragma once
#include <SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include <cstdint>
#include <string>
#include <vector>

class Player;

namespace ui {
constexpr int Width = 1440, Height = 960;
struct Color {
    unsigned char r, g, b, a = 255;
};
inline constexpr Color background {12, 18, 22}, panel {19, 27, 32}, raised {25, 35, 41};
inline constexpr Color border {49, 62, 67}, muted {143, 158, 163}, text {226, 232, 229};
inline constexpr Color gold {221, 177, 104}, teal {97, 191, 171}, red {222, 112, 107};
inline constexpr Color purple {169, 128, 203};

enum class Icon {
    barracks, armory, alley, streets, cathedral, infirmary, slums, market, bridge,
    marine, guardsman, psyker, sister, defender, portal,
    cultist, chaosMarine, sorcerer, acolyte, warden, hound, drone,
    bloodletter, fleshHound, horror, flamer, daemonette, fiend, plaguebearer, beast,
    khorne, tzeentch, slaanesh, nurgle, insignia, supplies, search, shield, clock, grenade, skull
};
Icon sectorIcon(int sector);
Icon defenderIcon(const Player& player);
Icon enemyIcon(int kind);
Icon patronIcon(int patron);
Color sectorColor(int sector);

class Draw {
    struct Glyph { SDL_Texture* texture = nullptr; int width = 0, height = 0, left = 0, top = 0, advance = 0; };
    SDL_Renderer* renderer;
    FT_Library library = nullptr;
    FT_Face face = nullptr;
    std::map<std::pair<int, std::uint32_t>, Glyph> glyphs;
    const Glyph& glyph(int size, std::uint32_t character);
public:
    Draw(SDL_Renderer* renderer, const std::string& font = {});
    ~Draw();
    Draw(const Draw&) = delete;
    Draw& operator=(const Draw&) = delete;
    void fill(SDL_Rect box, Color color);
    void outline(SDL_Rect box, Color color);
    void line(int x1, int y1, int x2, int y2, Color color);
    void circle(int x, int y, int radius, Color color, bool filled = false);
    void label(int x, int y, const std::string& text, int size = 16, Color color = ui::text);
    int measure(const std::string& text, int size = 16);
    std::string elide(const std::string& text, int width, int size = 16);
    std::vector<std::string> wrap(const std::string& text, int width, int size = 16);
    int paragraph(int x, int y, const std::string& text, int width, int size = 16, Color color = ui::muted, int maxLines = 100);
    void bar(SDL_Rect box, int amount, int maximum, Color color);
    void icon(int x, int y, Icon kind, Color color, float scale = 1.0F);
    void backdrop();
    void arrow(float x1, float y1, float x2, float y2, Color color);
    SDL_Renderer* getRenderer() { return renderer; }
};
}
