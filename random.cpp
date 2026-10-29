#include "random.h"
#include <random>

std::random_device rd2;
std::mt19937 roll(rd2());


int diceRoller(int sides){
    if(sides <= 0){
        return 0;
    }
    int result;
    std::uniform_int_distribution<int> dice(1,sides);
    result = dice(roll);
    return result;
}