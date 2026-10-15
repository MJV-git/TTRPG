#include "enemy.h"
#include "entity.h"

const std::unordered_map<std::string, EnemyDef> enemyTypes {
    {"Tyranid", {5, 1, 2, 2, 1, damageType::physical, 5}},
    {"Tyranid Warrior", {10, 3, 4, 2, 2, damageType::physical, 7}},
    {"Chaos Sorcerer", {6, 2, 4, 3, 4, damageType::psychic, 6}}
};


Enemy::Enemy(std::string typeName, int sector)
    : Entity(
          enemyTypes.at(typeName).health,
          enemyTypes.at(typeName).armor,
          enemyTypes.at(typeName).strength,
          enemyTypes.at(typeName).precision,
          enemyTypes.at(typeName).will,
          sector,
          typeName),
      attackType(enemyTypes.at(typeName).attackType),
      attackDamage(enemyTypes.at(typeName).attackDamage) {
}

void Enemy::attack(Entity& target, int chaosBonus){
    target.takeDamage(attackType, attackDamage + chaosBonus);
}
