#pragma once

#include <string>
#include <vector>

class Sector{
    private:
    int ID;//track sectors in sector vector
    std::string name;//names
    std::vector<int> destinations;//list valid destinations per sector IE sector 1 can go to: 2,6,5 search vector for these values when check connecting.


    public:
    Sector(int, std::string, std::vector<int>);
    bool CheckConnecting(int);//validate sectors are copnnected
};
