#pragma once
#include <string>

enum class damageType { physical, psychic, trueDamage };

class Entity {
protected:
    int health, armor, strength, precision, will, sector, maxHealth;
    std::string name;
public:
    Entity(int hp, int armor, int strength, int precision, int will, int sector, std::string name);
    virtual ~Entity() = default;
    int takeDamage(damageType type, int amount);
    int heal(int amount);
    void heal();
    void moveTo(int destination) { sector = destination; }
    bool getAlive() const { return health > 0; }
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    virtual int getArmor() const { return armor; }
    int getBaseArmor() const { return armor; }
    int getStrength() const { return strength; }
    int getPrecision() const { return precision; }
    virtual int getWill() const { return will; }
    int getBaseWill() const { return will; }
    int getSector() const { return sector; }
    const std::string& getName() const { return name; }
};
