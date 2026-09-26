#include "map.h"
#include "game.h"
#include <vector>
#include <algorithm>

Sector::Sector(int sectorID, std::string sectorName,std::vector<int> sectorDestinations) : ID(sectorID), name(sectorName), destinations(sectorDestinations){}


bool Sector::checkConnecting(int destination) {
    //movement validation for entities to make sure the sectors are connected, may possibly be used for enemy locomotion as well as any modifiers with world event card system when implemented.
    if(std::find(destinations.begin(), destinations.end(), destination) != destinations.end()){
        return true;
    }else{
        return false;
    }
    
}
