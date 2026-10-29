#pragma once
#include "entity.h"
#include "items.h"
#include <array>
#include <vector>
class Player : public Entity {
    friend class Game;
    std::vector<WeaponId> arsenal {WeaponId::pistol};
    WeaponId equipped = WeaponId::pistol;
    std::array<int, 2> supplies {{1, 1}};
    int poison = 0;
    bool recovering = false;
    bool guarding = false;
public:
    using Entity::Entity;
    int getArmor() const override { return armor + (sector == 1 ? 2 : 0); }
    int getWill() const override { return will + (sector == 5 ? 4 : 0); }
    const std::vector<WeaponId>& getWeapons() const { return arsenal; }
    WeaponId getWeapon() const { return equipped; }
    int getSupply(Consumable item) const { return supplies.at(static_cast<std::size_t>(item)); }
    int getPoison() const { return poison; }
    bool isRecovering() const { return recovering; }
    bool isGuarding() const { return guarding; }
    bool equip(WeaponId id);
    bool giveWeapon(WeaponId id);
};
