#include "enemy.h"
const std::vector<EnemyDef>& enemyDefinitions() {
    static const std::vector<EnemyDef> values {
        {"Chaos Cultist", 5, 1, 2, 2, 1, damageType::physical, 5, EnemyRole::fighter, "Holds position. Attacks local defenders."},
        {"Chaos Space Marine", 10, 3, 4, 2, 2, damageType::physical, 7, EnemyRole::fighter, "Heavy armor and a strong melee attack. Use psychic fire or grenades."},
        {"Chaos Sorcerer", 6, 2, 4, 3, 4, damageType::psychic, 6, EnemyRole::fighter, "Psychic attacks bypass Armor. High Will resists them."},
        {"Rift Acolyte", 4, 0, 2, 2, 3, damageType::psychic, 3, EnemyRole::acolyte, "Channels psychic attacks at local defenders. Will protects against its spells."},
        {"Rift Warden", 9, 3, 3, 1, 2, damageType::physical, 6, EnemyRole::warden, "Shields its portal from weapons. Kill the Warden or collapse its portal with a krak grenade."},
        {"Warp Hound", 4, 0, 3, 1, 1, damageType::physical, 4, EnemyRole::hunter, "Pursues the nearest defender along an outgoing road. Cannot attack while moving."},
        {"Plague Drone", 7, 2, 2, 3, 1, damageType::physical, 4, EnemyRole::infector, "Damaging hits inflict two rounds of rot. Guard can prevent infection."},
        {"Bloodletter", 8, 1, 5, 1, 2, damageType::physical, 8, EnemyRole::fighter, "Khorne's blade bearer. Strong local melee attacks; suppress or strike through its light armor."},
        {"Flesh Hound", 9, 1, 4, 1, 4, damageType::physical, 6, EnemyRole::hunter, "Khorne's hunter follows outgoing roads. High Will resists psychic weapons."},
        {"Pink Horror", 7, 0, 1, 3, 5, damageType::psychic, 7, EnemyRole::fighter, "Tzeentch's sorcerous daemon. High Will but no Armor: use physical weapons."},
        {"Flamer of Tzeentch", 9, 1, 2, 4, 4, damageType::psychic, 8, EnemyRole::hunter, "Pursues defenders along roads, then attacks with warp fire. Guard reduces its psychic damage."},
        {"Daemonette", 6, 0, 4, 5, 2, damageType::physical, 5, EnemyRole::hunter, "Slaanesh's swift hunter. Pursues along roads and benefits from the patron's double attacks."},
        {"Fiend of Slaanesh", 12, 2, 5, 4, 3, damageType::physical, 6, EnemyRole::hunter, "An armored pursuing beast. Suppression stops its entire activation, including double attacks."},
        {"Plaguebearer", 12, 3, 3, 1, 2, damageType::physical, 6, EnemyRole::infector, "Nurgle's armored sentinel inflicts rot. Psychic weapons avoid its physical armor."},
        {"Beast of Nurgle", 16, 2, 4, 1, 2, damageType::physical, 7, EnemyRole::plagueHunter, "A durable hunter that follows roads and inflicts rot on damaging hits."}
    };
    return values;
}
Enemy::Enemy(int identifier, int type, int location)
    : Entity(enemyDefinitions().at(type).health, enemyDefinitions().at(type).armor,
             enemyDefinitions().at(type).strength, enemyDefinitions().at(type).precision,
             enemyDefinitions().at(type).will, location, enemyDefinitions().at(type).name),
      id(identifier), kind(type) {}
damageType Enemy::getAttackType() const { return enemyDefinitions().at(kind).attackType; }
int Enemy::getAttackDamage() const { return enemyDefinitions().at(kind).attackDamage; }
int Enemy::attack(Entity& target, int chaosBonus) const {
    return target.takeDamage(getAttackType(), getAttackDamage() + chaosBonus);
}
