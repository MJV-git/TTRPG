#pragma once

#include <string>

enum class damageType{
    physical,
    psychic
};

class Entity {
    protected:
    int ID, health, armor, strength, precision, will, sector; // stats
    std::string name;//name
    bool isAlive = health > 0;// HP check
    bool overKilled = false;

    public:
    void takeDamage(damageType, int);
    Entity(int, int, int, int, int, int, std::string name);
    virtual ~Entity() = default;
    void moveTo(int);// traverse from sector to sector, use checkconnecting fucntion in map
    void checkStatus();// check all stats and alive bool
    void kill();//kill an entity, remove from entity vector
    void spawn();//add to entity vector

    //getters
    int getSector();
    std::string getName();
    bool getAlive();
    int getHealth();



};
