#include "entity.h"
#include "game.h"

int main(){
    //The main function of infinite debugging
    //currently testing different mechanics, game loop will go here once everything is set in place
    Game game;
    Player* currentPlayer;
    bool characterCreated = game.characterCreator(4, 4, 4, 4, 4, "Jeff");
    bool character2Created = game.characterCreator(4, 4, 4, 4, 4, "Bob");
    if(characterCreated && character2Created){
        std::cout << characterCreated << std::endl;
        game.initialize();
    }
    currentPlayer = game.getPlayer();
    if (currentPlayer == nullptr){
        std::cout << "No Players!" << std::endl;
        return 1;
    }
    
    bool moved = game.movement(*currentPlayer, 2);
    std::cout << currentPlayer->getSector() << std::endl;
    if(moved){
        game.advancePlayerTurn();
    }

    currentPlayer = game.getPlayer();
    if (currentPlayer == nullptr){
        std::cout << "No Players!" << std::endl;
        return 1;
    }
    std::cout << currentPlayer->getName() << std::endl;
    
    moved = game.movement(*currentPlayer, 2);
    std::cout << currentPlayer->getSector() << std::endl;
    std::cout << currentPlayer->getSector() << std::endl;
    if(moved){
        game.advancePlayerTurn();
    }

    //process turns test + 2 player testing
    game.processTurn();
    game.processTurn();
    game.processTurn();

    currentPlayer = game.getPlayer();
    if (currentPlayer == nullptr){
        std::cout << "No Players!" << std::endl;
        return 1;
    }

    game.movement(*currentPlayer, 6);
    std::cout << currentPlayer->getSector() << std::endl;
    std::cout << currentPlayer->getName() << std::endl;
    
    /*damage test
    Entity dummy(10, 0, 4, 4, 4, 4, "dummy");
    dummy.takeDamage(damageType::psychic, 16);
    dummy.getHealth();*/

    currentPlayer->takeDamage(damageType::physical, 20);
    std::cout << currentPlayer->getSector() << std::endl;
    
 
    Entity testTarget(30, 4, 1, 1, 1, 2, "Test Target");
    Enemy tyranid("Tyranid", 2);
    Enemy sorcerer("Chaos Sorcerer", 2);

    tyranid.attack(testTarget, 2);
    std::cout << testTarget.getHealth() << std::endl; // 27

    sorcerer.attack(testTarget, 2);
    std::cout << testTarget.getHealth() << std::endl; // 20

    //weapons testing
    Weapon weapon("Bolter");
    std::cout << "Damage: " << weapon.getDmg() << "\nRequired Stat Level: " <<weapon.getSLvl() << std::endl;
    switch(weapon.getReq()){
        case statReq::strength:
        std::cout << "Required Stat: Strength" << std::endl;
        break;

        case statReq::precision:
        std::cout << "Required Stat: Precision" << std::endl;
        break;

        case statReq::will:
        std::cout << "Required Stat: Will" << std::endl;
        break;
    }
    switch(weapon.getDType()){
        case damageType::physical:
        std::cout << "Damage Type: Physical" << std::endl;
        break;

        case damageType::psychic:
        std::cout << "Damage Type: Psychic" << std::endl;
    }
    Player testplayer(1, 1, 1, 1, 1, 1, "test");
    testplayer.addToInventory(std::make_unique<Weapon>(weapon));
    testplayer.equipWeapon(0);
    if(testplayer.getEquipped() != nullptr){
        std::cout << testplayer.getEquipped()->getIName() << std::endl;

    }




}
