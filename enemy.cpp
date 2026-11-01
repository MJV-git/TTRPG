#include "enemy.h"
const std::vector<EnemyDef>& enemyDefinitions() {
    static const std::vector<EnemyDef> values {
        {"Hollow scavenger", 5, 1, 2, 2, 1, damageType::physical, 5, EnemyRole::fighter, "Holds position. Attacks local defenders."},
        {"Siege automaton", 10, 3, 4, 2, 2, damageType::physical, 7, EnemyRole::fighter, "Heavy armor and a strong melee attack. Use psychic fire or grenades."},
        {"Signal leech", 6, 2, 4, 3, 4, damageType::psychic, 6, EnemyRole::fighter, "Psychic attacks bypass Armor. High Will resists them."},
        {"Breach tuner", 4, 0, 2, 2, 3, damageType::psychic, 3, EnemyRole::breachTuner, "Channels psychic attacks at local defenders. Will protects against its signal pulses."},
        {"Gate anchor", 9, 3, 3, 1, 2, damageType::physical, 6, EnemyRole::anchor, "Shields its portal from weapons. Kill the Anchor or collapse its portal with a breach grenade."},
        {"Wire stalker", 4, 0, 3, 1, 1, damageType::physical, 4, EnemyRole::hunter, "Pursues the nearest defender along an outgoing road. Cannot attack while moving."},
        {"Rust drone", 7, 2, 2, 3, 1, damageType::physical, 4, EnemyRole::infector, "Damaging hits inflict two rounds of corrosion. Guard can prevent infection."},
        {"Kiln sentinel", 8, 1, 5, 1, 2, damageType::physical, 8, EnemyRole::fighter, "A walking kiln controlled by the Ember Regent. Strong local melee attacks; suppress or strike through its light armor."},
        {"Cinder tracker", 9, 1, 4, 1, 4, damageType::physical, 6, EnemyRole::hunter, "Ember Regent's hunter follows outgoing roads. High Will resists psychic weapons."},
        {"Prism mote", 7, 0, 1, 3, 5, damageType::psychic, 7, EnemyRole::fighter, "A drifting lens controlled by the Prism Oracle. High Will but no Armor: use physical weapons."},
        {"Refraction crawler", 9, 1, 2, 4, 4, damageType::psychic, 8, EnemyRole::hunter, "Pursues defenders along roads, then attacks with signal bursts. Guard reduces its psychic damage."},
        {"Echo runner", 6, 0, 4, 5, 2, damageType::physical, 5, EnemyRole::hunter, "Echo Weaver's swift hunter. Pursues along roads and benefits from the patron's double attacks."},
        {"Harmonic prowler", 12, 2, 5, 4, 3, damageType::physical, 6, EnemyRole::hunter, "An armored pursuit machine. Suppression stops its entire activation, including double attacks."},
        {"Rust carrier", 12, 3, 3, 1, 2, damageType::physical, 6, EnemyRole::infector, "Rust Gardener's armored sentinel inflicts corrosion. Psychic weapons avoid its physical armor."},
        {"Orchard crawler", 16, 2, 4, 1, 2, damageType::physical, 7, EnemyRole::rustHunter, "A durable hunter that follows roads and inflicts corrosion on damaging hits."}
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
int Enemy::attack(Entity& target, int instabilityBonus) const {
    return target.takeDamage(getAttackType(), getAttackDamage() + instabilityBonus);
}
