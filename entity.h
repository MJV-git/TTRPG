#pragma once

#include <string>

class Entity {
    protected:
    int ID, health, armor, strength, precision, will, sector; // stats
    std::string name;//name
    bool isAlive;// HP check

    public:
    Entity(int, int, int, int, int, int);
    virtual ~Entity() = default;
    void MoveTo();// traverse from sector to sector, use checkconnecting fucntion in map
    void CheckStatus();// check all stats and alive bool
    void Kill();//kill an entity, remove from entity vector
    void Spawn();//add to entity vector


};
