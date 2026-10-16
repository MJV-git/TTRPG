#include "items.h"

// Define Item, Weapon, and Consumable member functions here when declared in items.h.
const std::unordered_map<std::string, weaponDef> Weapon::weaponTable{
    {"Power Sword", {5, 3, statReq::strength, damageType::physical}},
    {"Bolter", {5, 3, statReq::precision, damageType::physical}},
    {"Staff", {5, 3, statReq::will, damageType::psychic}}

};

Weapon::Weapon(std::string wName) {
    name = wName;
    baseDmg = weaponTable.at(wName).baseDmg;
    requiredStatLvl = weaponTable.at(wName).requiredStatLvl;
    requirement = weaponTable.at(wName).requirement;
    wDamageType = weaponTable.at(wName).wDamageType;

}

