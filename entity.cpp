#include "entity.h"
#include "map.h"
#include "game.h"

Entity::Entity( int eHealth, int eArmor, int eStrength, int ePrecision, int eWill, int eSector) : health(eHealth), armor(eArmor), strength(eStrength), precision(ePrecision), will(eWill), sector(eSector){
}

void Entity::MoveTo() {
    
}

void Entity::CheckStatus() {
}

void Entity::Kill() {
}

void Entity::Spawn() {
}
