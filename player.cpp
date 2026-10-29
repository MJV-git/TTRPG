#include "player.h"
#include <algorithm>
bool Player::equip(WeaponId id) {
    if (std::find(arsenal.begin(), arsenal.end(), id) == arsenal.end()) return false;
    equipped = id;
    return true;
}
bool Player::giveWeapon(WeaponId id) {
    if (std::find(arsenal.begin(), arsenal.end(), id) != arsenal.end()) return false;
    arsenal.push_back(id);
    return true;
}
