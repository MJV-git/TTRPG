#pragma once
#include "entity.h"
#include <vector>
enum class BossId { khorne, tzeentch, slaanesh, nurgle };
struct BossDef {
    BossId id;
    std::string name, title, rule;
    int deadline, health, armor, will, damage;
    damageType type;
};
const std::vector<BossDef>& bosses();
const BossDef& bossDefinition(BossId id);
class Boss : public Entity {
    friend class Game;
    BossId kind;
public:
    Boss(BossId id, int players);
    BossId getKind() const { return kind; }
};
