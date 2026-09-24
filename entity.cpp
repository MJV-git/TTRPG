#include "entity.h"
#include "map.h"
#include "game.h"

Entity::Entity( int eHealth, int eArmor, int eStrength, int ePrecision, int eWill, int eSector, std::string eName) : health(eHealth), armor(eArmor), strength(eStrength), precision(ePrecision), will(eWill), sector(eSector), name(eName){
}

void Entity::moveTo(int destination) {
    sector = destination;
    
}

void Entity::checkStatus() {
}

void Entity::kill() {
}

void Entity::spawn() {
}

int Entity::getSector(){
    return sector;
}
std::string Entity::getName(){
    return name;
}
