#pragma once

#include "entity.h"

class Enemy: public Entity{
    private:
    int ID; //track enemy


    public:
    void AttackPlayer();


};
