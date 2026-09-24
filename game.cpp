#include "game.h"

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

void Game::processTurn() {
    switch (currentState){
        case gamestate::playerTurn:
        break;

        case gamestate::worldTurn:
        break;

        case gamestate::enemyTurn:
        break;

        case gamestate::roundEnd:
        break;
    }
}

void Game::render() {
}

void Game::draw() {
}

void Game::update() {
}

void Game::advancePlayerTurn(){
    if(getPlayer() && currentState == gamestate::playerTurn){
        playerIdx++;
            if(playerIdx == playerList.size()){
            currentState = gamestate::worldTurn;
            return;
        }

    }
    
}


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
bool Game::movement(Entity& entity, int sector){
    int currentSector = entity.getSector();
    int destination = sector;
    
    if(currentState != gamestate::playerTurn){
        std::cout << "Not Player Turn" << std::endl;
        return false;
    }
    if(&entity != getPlayer()){
        std::cout << "It is " << getPlayer()->getName() << "'s Turn" << std::endl;
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
        return n * (n + 1) / 2;
    }
Player* Game::getPlayer(){
    if(playerList.empty() || playerIdx < 0 || playerIdx >= playerList.size()){
        return nullptr;
    }
    return playerList[playerIdx];
}
