#include "map.h"
#include "game.h"
#include <vector>
#include <algorithm>

Sector::Sector(int sectorID, std::string sectorName,std::vector<int> sectorDestinations) : ID(sectorID), name(sectorName), destinations(sectorDestinations){}


bool Sector::CheckConnecting(int destination) {
    if(std::find(destinations.begin(), destinations.end(), destination) != destinations.end()){
        return true;
    }else{
        return false;
    }
    
}
