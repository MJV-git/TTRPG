#include "game.h"

int main(){
    Game game;
    Player* currentPlayer;
    bool characterCreated = game.CharacterCreator(4, 4, 4, 4, 4);
    if(characterCreated){
        std::cout << characterCreated << std::endl;
        game.Initialize();
    }
    currentPlayer = game.GetPlayer();
    if (currentPlayer == nullptr){
        std::cout << "No Players!" << std::endl;
        return 1;
    }
    
    bool moved = game.Movement(*currentPlayer, 2);
    std::cout << currentPlayer->GetSector() << std::endl;
    if(moved){
        game.AdvancePlayerTurn();
    }
    game.Movement(*currentPlayer, 6);
    std::cout << currentPlayer->GetSector() << std::endl;
    
}
