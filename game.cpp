#include "game.h"

void Game::Initialize() {
    CreateSectors();
    GenerateEntities();
}

void Game::ProcessTurn() {
}

void Game::Render() {
}

void Game::Draw() {
}

void Game::Update() {
}


void Game::CreateSectors() {

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

        AddToSV(Sector(ID, name, destinations));
    }

}
void Game::MainMenu(){

}
void Game::GenerateEntities(){

    
}
void Game::CharacterCreator(){
    

}
