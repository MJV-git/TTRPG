#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "map.h"
#include "entity.h"
#include "player.h"
#include "enemy.h"
#include <memory>
#include <random>

enum class gamestate{
    playerTurn, worldTurn, enemyTurn, roundEnd
};

class Game{
    private:
    gamestate currentState = gamestate::playerTurn;
    int playerIdx = 0;
    std::vector<Player*> playerList;
    int turnCount, chaosLevel;//track turns and chaos level
    std::string selectedBoss; //track boss selected
    std::vector<Sector> sectorVector; //sectors stored here
    std::vector<std::unique_ptr<Entity>> entityVector;// entities stored here


    public:
    void advancePlayerTurn();
    Player* getPlayer();
    int statCost(int);
    bool movement(Entity& , int);
    bool characterCreator(int, int, int, int ,int, std::string);
    void generateEntities();
    void mainMenu();
    void createSectors();//generate sectors
    void addToEV(std::unique_ptr<Entity> entity){
        entityVector.push_back(std::move(entity));
    }
    void addToSV(Sector generatedSector){
        sectorVector.push_back(generatedSector);
    };
    void initialize(); // set the board
    void processTurn();// process all events

    //GUI shit, do not touch until you have numbers working
    void render();
    void draw();
    void update();

};
