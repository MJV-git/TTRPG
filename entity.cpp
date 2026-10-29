#include "entity.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

Entity::Entity(int hp, int a, int s, int p, int w, int location, std::string label)
    : health(hp), armor(a), strength(s), precision(p), will(w), sector(location),
      maxHealth(hp), name(std::move(label)) {
    if (hp < 1 || a < 0 || s < 0 || p < 0 || w < 0 || location < 1 || location > 9)
        throw std::invalid_argument("Invalid entity stats or sector.");
}

int Entity::takeDamage(damageType type, int amount) {
    const int resistance = type == damageType::physical ? getArmor() :
                           type == damageType::psychic ? getWill() : 0;
    const int dealt = std::min(health, std::max(0, amount - resistance));
    health -= dealt;
    return dealt;
}
int Entity::heal(int amount) {
    const int restored = std::min(maxHealth - health, std::max(0, amount));
    health += restored;
    return restored;
}
void Entity::heal() { health = maxHealth; }
