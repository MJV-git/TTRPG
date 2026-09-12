#pragma once

#include <string>


//different item types
class item{
    protected:
    int id;
    int value;
    std::string name; 

};

class weapon: public item{
    int damageMelee;
    int damageRanged;
    int damagePsychic;

};

class consumable: public item{
    int healthRestore;
    int armRestore;

};
