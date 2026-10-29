#pragma once
#include "game.h"
#include <string>

void saveCampaign(const Game& game, const std::string& filename);
Game loadCampaign(const std::string& filename);
