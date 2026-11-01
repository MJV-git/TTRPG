#include "app.h"
#include <charconv>
#include <iostream>
#include <memory>
#include <stdexcept>

int main(int argc, char** argv) {
    bool initialized = false;
    try {
        bool quick = false, smoke = false, fixed = false;
        std::uint32_t seed = std::random_device{}();
        std::string font, load, screenshot;
        for (int i = 1; i < argc; ++i) {
            const std::string option = argv[i];
            if (option == "--quick") quick = true;
            else if (option == "--smoke-test") smoke = true;
            else if (option == "--font" && i + 1 < argc) font = argv[++i];
            else if (option == "--load" && i + 1 < argc) load = argv[++i];
            else if (option == "--screenshot" && i + 1 < argc) screenshot = argv[++i];
            else if (option == "--seed" && i + 1 < argc) {
                const std::string value = argv[++i];
                const auto parsed = std::from_chars(value.data(), value.data() + value.size(), seed);
                if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) throw std::runtime_error("Seed must be an unsigned 32-bit integer.");
                fixed = true;
            } else if (option == "--help" || option == "-h") {
                std::cout << "Veyra / The Last Watch / graphical edition\n"
                          << "Usage: ttrpg_gui [--quick] [--seed N] [--load PATH] [--font PATH]\n"
                          << "  --quick        Deploy a varied preset defender against Ember Regent\n"
                          << "  --seed N       Choose a repeatable scenario\n"
                          << "  --load PATH    Resume a terminal or graphical save\n"
                          << "  --font PATH    Use a local TrueType/OpenType font\n"
                          << "  --screenshot PATH.bmp  Render one frame to a BMP and exit\n"
                          << "  --smoke-test   Run automated UI interaction checks in an isolated directory\n";
                return 0;
            } else throw std::runtime_error("Unknown or incomplete option: " + option);
        }
        if (quick && !load.empty()) throw std::runtime_error("Choose --quick or --load, not both.");
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) throw std::runtime_error(SDL_GetError());
        initialized = true;
        auto window = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>(
            SDL_CreateWindow("Veyra - The Last Watch", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             ui::Width, ui::Height, SDL_WINDOW_RESIZABLE | (smoke || !screenshot.empty() ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN)), SDL_DestroyWindow);
        if (!window) throw std::runtime_error(SDL_GetError());
        SDL_SetWindowMinimumSize(window.get(), 960, 640);
        auto renderer = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>(
            SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC), SDL_DestroyRenderer);
        if (!renderer) renderer.reset(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_SOFTWARE));
        if (!renderer) throw std::runtime_error(SDL_GetError());
        SDL_RenderSetLogicalSize(renderer.get(), ui::Width, ui::Height);
        {
            Gui app(window.get(), renderer.get(), font, seed, fixed);
            if (smoke) app.smokeTest(screenshot);
            else {
                if (quick) app.quick();
                if (!load.empty()) { app.restore(load); if (!app.isPlaying()) throw std::runtime_error("Could not load campaign: " + load); }
                if (!screenshot.empty()) app.screenshot(screenshot);
                else app.run();
            }
        }
        renderer.reset(); window.reset(); SDL_Quit();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "GUI error: " << e.what() << '\n';
        if (initialized) SDL_Quit();
        return 1;
    }
}
