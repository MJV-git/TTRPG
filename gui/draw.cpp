#include "draw.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <stdexcept>

namespace ui {
namespace {
std::uint32_t nextCharacter(const std::string& text, std::size_t& index) {
    const auto lead = static_cast<unsigned char>(text[index++]);
    if (lead < 128) return lead;
    const int count = lead >= 0xF0 ? 3 : lead >= 0xE0 ? 2 : lead >= 0xC0 ? 1 : 0;
    if (count == 0 || index + count > text.size()) return '?';
    std::uint32_t code = lead & ((1U << (6 - count)) - 1);
    for (int i = 0; i < count; ++i) {
        const auto continuation = static_cast<unsigned char>(text[index]);
        if ((continuation & 0xC0) != 0x80) return '?';
        code = (code << 6) | (continuation & 0x3F); ++index;
    }
    return code > 0x10FFFF ? '?' : code;
}
void removeCharacter(std::string& value) {
    if (value.empty()) return;
    std::size_t start = value.size() - 1;
    while (start > 0 && (static_cast<unsigned char>(value[start]) & 0xC0) == 0x80) --start;
    value.erase(start);
}
}
Draw::Draw(SDL_Renderer* renderer, const std::string& requested) : renderer(renderer) {
    std::vector<std::string> paths;
    if (!requested.empty()) paths.push_back(requested);
    else {
        if (const char* font = std::getenv("TTRPG_FONT")) paths.emplace_back(font);
        paths.insert(paths.end(), {
            "/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
            "/usr/share/fonts/gnu-free/FreeSans.otf",
            "C:/Windows/Fonts/segoeui.ttf", "/System/Library/Fonts/Supplemental/Arial.ttf"
        });
    }
    if (FT_Init_FreeType(&library)) throw std::runtime_error("Could not initialize FreeType.");
    for (const auto& path : paths) {
        if (!FT_New_Face(library, path.c_str(), 0, &face)) break;
    }
    if (!face) {
        FT_Done_FreeType(library); library = nullptr;
        throw std::runtime_error("No readable TrueType/OpenType font. Use --font PATH or TTRPG_FONT.");
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}
Draw::~Draw() {
    for (auto& pair : glyphs) if (pair.second.texture) SDL_DestroyTexture(pair.second.texture);
    if (face) FT_Done_Face(face);
    if (library) FT_Done_FreeType(library);
}
const Draw::Glyph& Draw::glyph(int size, std::uint32_t character) {
    const auto key = std::make_pair(size, character);
    auto it = glyphs.find(key);
    if (it != glyphs.end()) return it->second;
    if (FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(size)) || FT_Load_Char(face, character, FT_LOAD_RENDER))
        throw std::runtime_error("Could not render font glyph.");
    const auto& bitmap = face->glyph->bitmap;
    Glyph g;
    g.width = static_cast<int>(bitmap.width); g.height = static_cast<int>(bitmap.rows);
    g.left = face->glyph->bitmap_left; g.top = face->glyph->bitmap_top;
    g.advance = static_cast<int>(face->glyph->advance.x >> 6);
    if (g.width && g.height) {
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, g.width, g.height, 32, SDL_PIXELFORMAT_RGBA32);
        if (!surface) throw std::runtime_error(SDL_GetError());
        auto* pixels = static_cast<Uint32*>(surface->pixels);
        for (int y = 0; y < g.height; ++y) for (int x = 0; x < g.width; ++x) {
            const unsigned char alpha = bitmap.buffer[y * bitmap.pitch + x];
            pixels[y * surface->pitch / 4 + x] = SDL_MapRGBA(surface->format, 255, 255, 255, alpha);
        }
        g.texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        if (!g.texture) throw std::runtime_error(SDL_GetError());
        SDL_SetTextureBlendMode(g.texture, SDL_BLENDMODE_BLEND);
    }
    return glyphs.emplace(key, g).first->second;
}
void Draw::fill(SDL_Rect box, Color c) { SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a); SDL_RenderFillRect(renderer, &box); }
void Draw::outline(SDL_Rect box, Color c) { SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a); SDL_RenderDrawRect(renderer, &box); }
void Draw::line(int x1, int y1, int x2, int y2, Color c) { SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a); SDL_RenderDrawLine(renderer, x1, y1, x2, y2); }
void Draw::circle(int x, int y, int radius, Color color, bool filled) {
    if (filled) {
        for (int dy = -radius; dy <= radius; ++dy) {
            int dx = static_cast<int>(std::sqrt(radius * radius - dy * dy));
            line(x - dx, y + dy, x + dx, y + dy, color);
        }
    } else {
        for (int i = 0; i < 180; ++i) {
            const double a = i * 6.28318530718 / 180, b = (i + 1) * 6.28318530718 / 180;
            line(x + static_cast<int>(radius * std::cos(a)), y + static_cast<int>(radius * std::sin(a)),
                 x + static_cast<int>(radius * std::cos(b)), y + static_cast<int>(radius * std::sin(b)), color);
        }
    }
}
void Draw::label(int x, int y, const std::string& s, int size, Color color) {
    for (std::size_t index = 0; index < s.size();) {
        const auto character = nextCharacter(s, index);
        if (character == '\n') continue;
        const auto& g = glyph(size, character);
        if (g.texture) {
            SDL_SetTextureColorMod(g.texture, color.r, color.g, color.b);
            SDL_SetTextureAlphaMod(g.texture, color.a);
            SDL_Rect dest {x + g.left, y + size - g.top, g.width, g.height};
            SDL_RenderCopy(renderer, g.texture, nullptr, &dest);
        }
        x += g.advance;
    }
}
int Draw::measure(const std::string& s, int size) {
    int result = 0;
    for (std::size_t index = 0; index < s.size();) result += glyph(size, nextCharacter(s, index)).advance;
    return result;
}
std::string Draw::elide(const std::string& s, int width, int size) {
    if (measure(s, size) <= width) return s;
    std::string result = s;
    while (!result.empty() && measure(result + "...", size) > width) removeCharacter(result);
    return result + "...";
}
std::vector<std::string> Draw::wrap(const std::string& text, int width, int size) {
    std::vector<std::string> lines;
    std::istringstream paragraphs(text);
    std::string paragraph;
    while (std::getline(paragraphs, paragraph)) {
        std::istringstream words(paragraph);
        std::string word, line;
        while (words >> word) {
            if (!line.empty() && measure(line + " " + word, size) > width) { lines.push_back(line); line.clear(); }
            while (measure(word, size) > width && word.size() > 1) {
                if (!line.empty()) { lines.push_back(line); line.clear(); }
                std::size_t length = 0;
                nextCharacter(word, length);
                while (length < word.size()) {
                    std::size_t next = length; nextCharacter(word, next);
                    if (measure(word.substr(0, next), size) > width) break;
                    length = next;
                }
                lines.push_back(word.substr(0, length)); word.erase(0, length);
            }
            line += (line.empty() ? "" : " ") + word;
        }
        lines.push_back(line);
    }
    return lines;
}
int Draw::paragraph(int x, int y, const std::string& s, int width, int size, Color c, int maxLines) {
    const auto lines = wrap(s, width, size);
    for (int i = 0; i < std::min(static_cast<int>(lines.size()), maxLines); ++i) {
        label(x, y, i == maxLines - 1 && static_cast<int>(lines.size()) > maxLines ? elide(lines[i] + " ...", width, size) : lines[i], size, c);
        y += size + 6;
    }
    return y;
}
void Draw::bar(SDL_Rect box, int amount, int maximum, Color color) {
    fill(box, {38, 46, 49});
    if (maximum > 0) { box.w = box.w * std::clamp(amount, 0, maximum) / maximum; if (box.w > 0) fill(box, color); }
}
void Draw::arrow(float x1, float y1, float x2, float y2, Color color) {
    line(static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2), color);
    const float angle = std::atan2(y2 - y1, x2 - x1);
    const float x = x1 + (x2 - x1) * 0.62F, y = y1 + (y2 - y1) * 0.62F;
    for (float side : {-0.55F, 0.55F})
        line(static_cast<int>(x), static_cast<int>(y), static_cast<int>(x - 11 * std::cos(angle + side)), static_cast<int>(y - 11 * std::sin(angle + side)), color);
}
void Draw::backdrop() {
    fill({0, 0, Width, Height}, background);
    for (int x = 0; x < Width; x += 40) line(x, 0, x, Height, {19, 27, 31});
    for (int y = 0; y < Height; y += 40) line(0, y, Width, y, {19, 27, 31});
    for (int y = 0; y < Height; y += 4) line(0, y, Width, y, {0, 0, 0, 12});
}
}
