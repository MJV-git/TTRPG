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
    std::string name; 

    public:
    virtual ~Item() = default;
    std::string getIName() const {return name;}

};

class Weapon: public Item{
    static const std::unordered_map<std::string, weaponDef> weaponTable;

    int baseDmg;
    statReq requirement;
    int requiredStatLvl;
    damageType wDamageType;

    public:
    Weapon(std::string);

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
