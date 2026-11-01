#include "items.h"
#include <algorithm>
#include <stdexcept>
const std::vector<Weapon>& weapons() {
    static const std::vector<Weapon> values {
        {WeaponId::pistol, "Rivet pistol", 3, damageType::physical, Accuracy::precision, 0, 2},
        {WeaponId::beamCarbine, "Beam carbine", 4, damageType::physical, Accuracy::precision, 0, 2},
        {WeaponId::rivetRifle, "Rivet rifle", 5, damageType::physical, Accuracy::precision, 0, 2},
        {WeaponId::sword, "Arc blade", 6, damageType::physical, Accuracy::strength, 2},
        {WeaponId::rotaryRiveter, "Rotary riveter", 7, damageType::physical, Accuracy::precision, 4, 2},
        {WeaponId::staff, "Resonance staff", 7, damageType::psychic, Accuracy::will, 0, 0, 3},
        {WeaponId::needleRifle, "Needle rifle", 10, damageType::physical, Accuracy::precision, 0, 5},
        {WeaponId::thermalCannon, "Thermal cannon", 12, damageType::physical, Accuracy::precision, 4, 3},
        {WeaponId::pileMaul, "Pile maul", 11, damageType::physical, Accuracy::strength, 5},
        {WeaponId::resonanceGlaive, "Resonance glaive", 11, damageType::psychic, Accuracy::will, 0, 0, 5}
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
        case Consumable::grenade: return "Breach grenade";
    }
    throw std::invalid_argument("Unknown consumable.");
}
