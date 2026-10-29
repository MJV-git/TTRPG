#pragma once
#include <string>
#include <vector>
struct Sector {
    int id;
    std::string name;
    std::vector<int> destinations;
    bool checkConnecting(int destination) const;
};
const std::vector<Sector>& sectors();
const Sector& sectorAt(int id);
std::vector<int> outgoingRoutes(int sector, bool reversed = false);
int nextStep(int from, int to, bool reversed = false);
