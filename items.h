#pragma once
#include "entity.h"
#include <string>
#include <vector>

enum class WeaponId { pistol, beamCarbine, rivetRifle, sword, rotaryRiveter, staff, needleRifle, thermalCannon, pileMaul, resonanceGlaive };
enum class Consumable { medkit, grenade };
enum class Accuracy { strength, precision, will };
struct Weapon {
    WeaponId id;
    std::string name;
    int damage;
    damageType type;
    Accuracy accuracy;
    int requiredStrength;
    int requiredPrecision = 0, requiredWill = 0;
};
struct WeaponProfile {
    int damage;
    damageType type;
    int hitChance;
    bool underRequirement;
};
const std::vector<Weapon>& weapons();
const Weapon& weapon(WeaponId id);
WeaponProfile weaponProfile(WeaponId id, const Entity& wielder);
std::string weaponRequirements(WeaponId id);
std::string consumableName(Consumable item);
