#include "boss.h"
const std::vector<BossDef>& bosses() {
    static const std::vector<BossDef> values {
        {BossId::khorne, "Khorne", "The Blood Siege",
         "Shortest deadline. The manifested boss deals 2 extra damage below half health.",
         24, 50, 3, 3, 10, damageType::physical},
        {BossId::tzeentch, "Tzeentch", "The Fractured Veil",
         "Four starting portals instead of two. The boss deals psychic damage.",
         32, 52, 2, 5, 10, damageType::psychic},
        {BossId::slaanesh, "Slaanesh", "The Endless Choir",
         "Every enemy and the boss attacks twice per enemy phase.",
         28, 50, 2, 3, 7, damageType::physical},
        {BossId::nurgle, "Nurgle", "The Rot Beneath",
         "Damaging enemy hits inflict two rounds of rot (1 damage per round).",
         30, 60, 3, 4, 9, damageType::physical}
    };
    return values;
}
const BossDef& bossDefinition(BossId id) { return bosses().at(static_cast<std::size_t>(id)); }
Boss::Boss(BossId id, int players)
    : Entity(bossDefinition(id).health + 6 * players, bossDefinition(id).armor,
             4, 4, bossDefinition(id).will, 9, bossDefinition(id).name), kind(id) {}
