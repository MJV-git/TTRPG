#include "entity.h"
#include "map.h"
#include "game.h"

Entity::Entity( int health, int armor, int strength, int precision, int will, int sector){
    health = health;
    armor = armor;
    strength = strength;
    precision = precision;
    will = will;
    sector = sector;
}

void Entity::MoveTo() {
    
}

void Entity::CheckStatus() {
}

void Entity::Kill() {
}

void Entity::Spawn() {
}
