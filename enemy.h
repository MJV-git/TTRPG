#pragma once
#include "entity.h"
#include <vector>
enum class EnemyRole { fighter, acolyte, warden, hunter, infector, plagueHunter };
struct EnemyDef {
    std::string name;
    int health, armor, strength, precision, will;
    damageType attackType;
    int attackDamage;
    EnemyRole role = EnemyRole::fighter;
    std::string tactic;
};
const std::vector<EnemyDef>& enemyDefinitions();
class Enemy : public Entity {
    friend class Game;
    int id;
    int kind;
    bool suppressed = false;
public:
    Enemy(int id, int kind, int sector);
    int getId() const { return id; }
    int getKind() const { return kind; }
    bool isSuppressed() const { return suppressed; }
    const EnemyDef& definition() const { return enemyDefinitions().at(kind); }
    damageType getAttackType() const;
    int getAttackDamage() const;
    int attack(Entity& target, int chaosBonus) const;
};
