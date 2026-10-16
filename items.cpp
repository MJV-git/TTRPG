#include "items.h"

// Define Item, Weapon, and Consumable member functions here when declared in items.h.
std::unordered_map<std::string, weaponDef> Weapon::weaponTable{
    {"Power Sword", {5, 3, statReq::strength, damageType::physical}},
    {"Bolter", {5, 3, statReq::precision, damageType::physical}},
    {"Staff", {5, 3, statReq::will, damageType::psychic}}

};

Weapon::Weapon(int dmg, int statLvl, statReq req, damageType dmgType) : baseDmg(dmg), requiredStatLvl(statLvl), requirement(req), wDamageType(dmgType) {

}

