#include "map.h"
#include "game.h"

Sector::Sector(int sectorID, std::string sectorName,std::vector<int> sectorDestinations) : ID(sectorID), name(sectorName), destinations(sectorDestinations){}


bool Sector::CheckConnecting() {
    // TODO: Return the connectivity result before calling this function.
}
