#pragma once

#include <vector>
#include <memory>
#include "entity.h"
#include "items.h"


class Player: public Entity {
    private:
    std::vector<std::unique_ptr<Item>> inventory; //store items
    Weapon* equipped = nullptr;


    public:
    //constructor
    Player(int, int, int, int, int, int, std::string);//constructor
    void addToInventory(std::unique_ptr<Item> item){
        inventory.push_back(std::move(item));
    }

    //functions
    void equipWeapon(int);
    void playerAttack(Entity*);


    //getters
    Weapon* getEquipped() const {return equipped;}


   
};
