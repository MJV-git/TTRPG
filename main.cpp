#include "entity.h"
#include "game.h"

int main(){
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


}
