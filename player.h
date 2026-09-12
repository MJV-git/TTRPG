#pragma once

#include <vector>
#include <memory>
#include "entity.h"
#include "items.h"


class Player: public Entity {
    private:
    std::vector<std::unique_ptr<item>> inventory; //store items


    public:
    Player();//constructor
    void CreatePlayer();

   
};
