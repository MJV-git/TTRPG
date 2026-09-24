#include "game.h"

int main(){
    Game game;
    Player* currentPlayer;
    bool characterCreated = game.characterCreator(4, 4, 4, 4, 4, "Jeff");
    if(characterCreated){
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
    game.movement(*currentPlayer, 6);
    std::cout << currentPlayer->getSector() << std::endl;
    
}
