#include "items.h"
#include <algorithm>
#include <stdexcept>
const std::vector<Weapon>& weapons() {
    static const std::vector<Weapon> values {
        {WeaponId::pistol, "Bolter pistol", 3, damageType::physical, Accuracy::precision, 0, 2},
        {WeaponId::lasgun, "Lasgun", 4, damageType::physical, Accuracy::precision, 0, 2},
        {WeaponId::bolter, "Bolter rifle", 5, damageType::physical, Accuracy::precision, 0, 2},
        {WeaponId::sword, "Power sword", 6, damageType::physical, Accuracy::strength, 2},
        {WeaponId::heavyBolter, "Heavy bolter", 7, damageType::physical, Accuracy::precision, 4, 2},
        {WeaponId::staff, "Force staff", 7, damageType::psychic, Accuracy::will, 0, 0, 3},
        {WeaponId::marksmanBolter, "Marksman bolter", 10, damageType::physical, Accuracy::precision, 0, 5},
        {WeaponId::plasmaGun, "Heavy plasma gun", 12, damageType::physical, Accuracy::precision, 4, 3},
        {WeaponId::thunderHammer, "Thunder hammer", 11, damageType::physical, Accuracy::strength, 5},
        {WeaponId::forceHalberd, "Force halberd", 11, damageType::psychic, Accuracy::will, 0, 0, 5}
    };
    return values;
}
const Weapon& weapon(WeaponId id) { return weapons().at(static_cast<std::size_t>(id)); }
WeaponProfile weaponProfile(WeaponId id, const Entity& wielder) {
    const auto& w = weapon(id);
    const int deficiencies = (wielder.getStrength() < w.requiredStrength) +
        (wielder.getPrecision() < w.requiredPrecision) + (wielder.getWill() < w.requiredWill);
    const int surplus = w.accuracy == Accuracy::strength ? wielder.getStrength() - w.requiredStrength :
        w.accuracy == Accuracy::precision ? wielder.getPrecision() - w.requiredPrecision : wielder.getWill() - w.requiredWill;
    return {w.damage, w.type, std::min(100, 80 - 20 * deficiencies + 5 * std::max(0, surplus)), deficiencies != 0};
}
std::string weaponRequirements(WeaponId id) {
    const auto& w = weapon(id); std::string result;
    auto append = [&](const std::string& stat, int required) {
        if (required) result += (result.empty() ? "" : " / ") + stat + " " + std::to_string(required);
    };
    append("STR", w.requiredStrength); append("PRE", w.requiredPrecision); append("WILL", w.requiredWill);
    return result.empty() ? "None" : result;
}

std::string consumableName(Consumable item) {
    switch (item) {
        case Consumable::medkit: return "Medkit";
        case Consumable::grenade: return "Krak grenade";
    }
    throw std::invalid_argument("Unknown consumable.");
}
