#pragma once

#include "entity.h"
#include <string>
#include <unordered_map>
enum class statReq {
    strength,
    precision,
    will

};
struct weaponDef{
    int baseDmg;
    int requiredStatLvl;
    statReq requirement;
    damageType wDamageType;
    
};

//different item types
class Item{
    protected:
    int id;
    int value;
    std::string name; 

    public:
    virtual ~Item() = default;

};

class Weapon: public Item{
    static std::unordered_map<std::string, weaponDef> weaponTable;

    int baseDmg;
    statReq requirement;
    int requiredStatLvl;
    damageType wDamageType;

    public:
    Weapon(int, int, statReq, damageType);

    //getters
    int getDmg() const {return baseDmg;}
    int getSLvl() const {return requiredStatLvl;}
    statReq getReq() const {return requirement;}
    damageType getDType() const {return wDamageType;}


};

class Consumable: public Item{
    int healthRestore;
    int armRestore;

};
