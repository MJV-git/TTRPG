#include "entity.h"
#include "map.h"
#include "game.h"


//as it stands, all children of this class are identical, will see if there will need to even be specific children or if they can be consolidated as one, ECS maybe? I like my way but maybe worth.
Entity::Entity( int eHealth, int eArmor, int eStrength, int ePrecision, int eWill, int eSector, std::string eName) : health(eHealth), armor(eArmor), strength(eStrength), precision(ePrecision), will(eWill), sector(eSector), name(eName), maxHealth(eHealth){
}
//actual movement
void Entity::moveTo(int destination) {
    sector = destination;
    
}

void Entity::kill() {
    
}

void Entity::spawn() {
}

void Entity::takeDamage(damageType damageType, int damage){
    //apply damage to selected entity
    //if dead, don't attack
    if (!isAlive){
        return;
    }
    //deal damage based on enum damagetype thru armor
    switch(damageType){
        case damageType::physical:
        damage -= armor;
        break;

        case damageType::psychic:
        damage -= will;
        break;
    }
    //if armor saves or not
    if(damage > 0){
        health -= damage;
        std::cout << "Damage Taken: " << damage << std::endl;
    }else{
        std::cout << "No Damage Taken" << std::endl;
    }
    //death, TODO: change sector to infirmary somehow, most likely in game.cpp
    if(health <= 0){
        std::cout << "You have been defeated, for now..." << std::endl;


        if (health <= -5){
            std::cout << "They really messed you up, you will need a turn to recover..." << std::endl;
            overKilled = true;
        }

        health = 0;
        isAlive = false;
        return;
    }

}
void Entity::heal(){
    health = maxHealth;
    isAlive = true;
}
//getters

bool Entity::getAlive(){
    return isAlive;
}
int Entity::getHealth(){
    return health;
}
int Entity::getSector(){
    return sector;
}
std::string Entity::getName(){
    return name;
}

//setters
void Entity::setMaxHealth(int mHealth){
    maxHealth = mHealth;

}
