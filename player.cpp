#include "player.h"
#include <iostream>


Player::Player( int pHealth, int pArmor, int pStrength, int pPrecision, int pWill, int pSector, std::string pName) : Entity(pHealth, pArmor, pStrength, pPrecision, pWill, pSector, pName) {
}


void Player::equipWeapon(int index){
    if (index < 0 || index >=inventory.size()){
        return;
    }
    if (inventory[index] == nullptr){
        std::cout << "No Item Found";
    } else if (inventory[index] != nullptr){
        Weapon* weapon = dynamic_cast<Weapon*>(inventory[index].get());
        if(weapon == nullptr){
            std::cout << "Not a Weapon" << std::endl;
            return;
        }else{
            equipped = weapon;
        }
    }

}
