#include "player.h"
#include <iostream>
#include "random.h"


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
void Player::playerAttack(Entity* target){
    if(target == nullptr){
        std::cout << "No target." << std::endl;
        return;
    }
    if(target->getSector() != getSector()){
        std::cout << "Not in same sector" << std::endl;
    }
    if(getEquipped() == nullptr){
        std::cout << "No weapon equipped" << std::endl;
        return;
    }
    bool meetsRequirements = false;
    damageType dmgType = getEquipped()->getDType();
    int damage = getEquipped()->getDmg();
    int requiredStatLvl = getEquipped()->getSLvl();
    switch(getEquipped()->getReq()){

        case statReq::strength:
        if(strength >= requiredStatLvl){
            meetsRequirements = true;
            
        }
        damage+=strength;
        break;

        case statReq::precision:
        if(precision >= requiredStatLvl){
            meetsRequirements = true;
            
        }
        damage+=precision;
        break;

        case statReq::will:
        if(will >= requiredStatLvl){
            meetsRequirements = true;
            damage+=will;
        }else{
            if(strength >= 1){
                meetsRequirements = true;
            }
            damage = 1;
            damage+=strength;
            dmgType = damageType::physical;
        }
        break;
        }
        if(meetsRequirements){
            target->takeDamage(dmgType, damage);
        }else{
            if(diceRoller(2) == 1){
                return;
            }
            else{
                target->takeDamage(dmgType, damage);
            }
    }
    
    

}