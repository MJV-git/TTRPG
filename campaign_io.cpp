#include "campaign_io.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>

void saveCampaign(const Game& game, const std::string& filename) {
    const std::filesystem::path target(filename);
    auto temporary = target;
    temporary += ".tmp";
    if (std::filesystem::exists(temporary)) throw std::runtime_error("Temporary save already exists: " + temporary.string());
    try {
        std::ofstream out(temporary, std::ios::trunc);
        if (!out) throw std::runtime_error("Cannot open save path: " + filename);
        game.save(out);
        out.close();
        if (!out) throw std::runtime_error("Could not finish writing save.");
        std::filesystem::rename(temporary, target);
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw;
    }
}

Game loadCampaign(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) throw std::runtime_error("Cannot open save: " + filename);
    return Game::load(in);
}
