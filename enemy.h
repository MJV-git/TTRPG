#pragma once

#include "entity.h"
#include <unordered_map>

struct EnemyDef{
    int health;
    int armor;
    int strength;
    int precision;
    int will;
    damageType attackType;
    int attackDamage;
};

class Enemy: public Entity{
    private:
    damageType attackType;
    int attackDamage;


    public:
    

    
    Enemy(std::string typeName, int sector);

    void attack(Entity& target, int chaosBonus);


};
