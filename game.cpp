#include "game.h"
#include "entity.h"
#include "player.h"
#include <random>

//since this is a learning project, I looked up how this clearly googled bullshit actually works
//apparently this random_device function generates a seed, computationally expensive so it is called only once per run
std::random_device rd;
//mt19937 is the number scrambler, that generates a sequence of numbers from the generated seed
std::mt19937 gen(rd());
//uniform int distribution compresses the selected generated number into the desired rrange which is 2-9 for the purpose of spawning enemies.        
std::uniform_int_distribution<int> secDist(2,9);
//still don't fully understand it, magic mathematics number bullshit.
//now if I ever need a random number anywhere i just create a uniform int distribution and name it, range it, and call it
    

void Game::initialize() {
    createSectors();
    generateEntities();
}
//steps through the turn process, must be called for each turn section.
void Game::processTurn() {
    switch (currentState){
        case gamestate::playerTurn:
        break;

        case gamestate::worldTurn:
        std::cout << "PLACEHOLDER CARD EVENT" << std::endl;
        currentState = gamestate::enemyTurn;
        break;

        case gamestate::enemyTurn:
        for (int i = 0; i < entityVector.size(); i++){
            Enemy* enemy = dynamic_cast<Enemy*>(entityVector[i].get());
            if (enemy != nullptr && enemy->getAlive()){
                std::cout << enemy->getName() << std::endl;
                std::vector<Player*> targets;
                for (int j = 0; j < playerList.size(); j++){
                    if (playerList[j]->getAlive() && playerList[j]->getSector() == enemy->getSector()){
                        std::cout << playerList[j]->getName() << std::endl;
                        targets.push_back(playerList[j]);
                    }
                }
                if (targets.empty()){
                    continue;
                }else {
                    std::uniform_int_distribution<int> selectedTargetIdx(0, targets.size()-1);
                    int targetedPlayer = selectedTargetIdx(gen);
                    std::cout << "Selected Target: " << targets[targetedPlayer]->getName() << std::endl;
                    targets[targetedPlayer]->takeDamage(damageType::physical, 5);            
                }
            }
        }
        //std::cout << "PLACEHOLDER ENEMY ATTACK TURN" << std::endl;
        currentState = gamestate::roundEnd;
        break;

        case gamestate::roundEnd:
        turnCount++;
        turnCount += chaosLevel;
        playerIdx = 0;
        currentState = gamestate::playerTurn;
        break;
    }
}
//gui stuff
void Game::render() {
}

void Game::draw() {
}

void Game::update() {
}
//advances specifically player turn, seperate from process turn to allow for multiple players. GUI will handle this
void Game::advancePlayerTurn(){
    if(getPlayer() && currentState == gamestate::playerTurn){
        playerIdx++;
            if(playerIdx == playerList.size()){
            currentState = gamestate::worldTurn;
            return;
        }

    }
    
}

//create the starting sectors, NOTE: considering making infirmary a leave only sector with no way of getting in except dying or world event.
void Game::createSectors() {

    std::string names[9] {"Barracks", "Armory", "Alley", "Streets", "Cathedral", "Infirmary", "Slums", "Market", "Bridge"};

    for (int i = 0; i < 9; i++){
        int ID = i + 1;
        std::string name = names[i];
        std::vector<int> destinations;

        switch (ID) {
            case 1:
            destinations.push_back(2);
            destinations.push_back(6);
            destinations.push_back(5);
            break;
            case 2:
            destinations.push_back(1);
            destinations.push_back(3);
            break;
            case 3:
            destinations.push_back(4);
            break;
            case 4:
            destinations.push_back(5);
            destinations.push_back(7);
            destinations.push_back(8);
            break;
            case 5:
            destinations.push_back(6);
            destinations.push_back(7);
            destinations.push_back(4);
            break;
            case 6:
            destinations.push_back(1);
            destinations.push_back(5);
            break;
            case 7:
            destinations.push_back(9);
            break;
            case 8:
            destinations.push_back(4);
            destinations.push_back(3);
            destinations.push_back(9);
            break;
            case 9:
            destinations.push_back(7);
            destinations.push_back(8);
            break;
        }

        addToSV(Sector(ID, name, destinations));
    }

}
void Game::mainMenu(){

}
//movement validation system, uses moveTo on success
bool Game::movement(Entity& entity, int sector){
    int currentSector = entity.getSector();
    int destination = sector;
    Player* currentPlayer = getPlayer();
    

    if(currentState != gamestate::playerTurn){
        std::cout << "Not Player Turn" << std::endl;
        return false;
    }
    if(currentPlayer == nullptr){
        std::cout << "Bad Player List" << std::endl;
        return false;
    }
    if(&entity != currentPlayer){
        std::cout << "It is " << currentPlayer->getName() << "'s Turn" << std::endl;
        return false;
    }
    if(currentSector < 1 ||  currentSector > sectorVector.size()){
        std::cout << "Sector Origin error: Sector Vector is Writhing in Pain" << std::endl;
        return false;
    }
    if(sector < 1 ||  sector > sectorVector.size()){
        std::cout << "Sector Destination error: Sector Vector is Writhing in Pain" << std::endl;
        return false;
    }

    if(sectorVector[currentSector - 1].checkConnecting(destination)){
        entity.moveTo(destination);
        return true;

    }
    std::cout << "Sector Connection Failure" << std::endl;
    return false;


}
//generates the initial enemies and adds to entity vector, TODO: later will include portals.
void Game::generateEntities(){
    int i = 0;
    while(i < 2){
        int rSec = secDist(gen);
        auto enemy = std::make_unique<Enemy>(5, 1, 2, 2, 2, rSec, "Tyranid");
        addToEV(std::move(enemy));
        i++;
    }
    

    
}
bool Game::characterCreator(int pHealth, int pArmor, int pStrength, int pPrecision, int pWill, std::string name){
    //create character. NOTE: This will be powered by GUI
   /*Add this to GUI when made 
    std::cout << "Please enter a name for your character" << std::endl;
    std::getline(std::cin, name);*/
    if(pArmor > 4 || pHealth < 0 || pArmor < 0 || pStrength < 0 || pPrecision < 0 || pWill < 0){
        std::cout << "Error, invalid values detected" << std::endl;
        return false;
    }
    int pointPool = 50;
    int pointSpend = 0;
    pointSpend += statCost(pHealth);
    pointSpend += statCost(pArmor);
    pointSpend += statCost(pStrength);
    pointSpend += statCost(pPrecision);
    pointSpend += statCost(pWill);
    if(pointPool < pointSpend){
        std::cout << "Error, Exceeded Point Pool" << std::endl;
        return false;
    }
    int pHP = 1 + 3 * pHealth;


    auto player = std::make_unique<Player>(pHP, pArmor, pStrength, pPrecision, pWill, 1, name);

    playerList.push_back(player.get());
    addToEV(std::move(player));
    return true;


}
int Game::statCost(int n){
    //helper function to calculate stat cost of point allocation
        return n * (n + 1) / 2;
    }
    //getter for player character from playerlist
Player* Game::getPlayer(){
    if(playerList.empty() || playerIdx < 0 || playerIdx >= playerList.size()){
        return nullptr;
    }
    return playerList[playerIdx];
}
