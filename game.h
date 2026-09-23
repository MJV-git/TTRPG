#pragma once

#include <string>
#include <vector>
#include "map.h"
#include "entity.h"
#include <memory>



class Game{
    private:
    int turnCount, chaosLevel;//track turns and chaos level
    std::string selectedBoss; //track boss selected
    std::vector<Sector> sectorVector; //sectors stored here
    std::vector<std::unique_ptr<Entity>> entityVector;// entities stored here


    public:
    void CharacterCreator();
    void GenerateEntities();
    void MainMenu();
    void CreateSectors();//generate sectors
    void AddToEV(std::unique_ptr<Entity> entity){
        entityVector.push_back(std::move(entity));
    }
    void AddToSV(Sector generatedSector){
        sectorVector.push_back(generatedSector);
    };
    void Initialize(); // set the board
    void ProcessTurn();// process all events

    //GUI shit, do not touch until you have numbers working
    void Render();
    void Draw();
    void Update();

};
