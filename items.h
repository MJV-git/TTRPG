#pragma once

#include <string>


//different item types
class Item{
    protected:
    int id;
    int value;
    std::string name; 

};

class Weapon: public Item{
    int damageMelee;
    int damageRanged;
    int damagePsychic;

};

class Consumable: public Item{
    int healthRestore;
    int armRestore;

};
