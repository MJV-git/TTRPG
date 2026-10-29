#include "map.h"
#include <algorithm>
#include <array>
#include <queue>
#include <stdexcept>
const std::vector<Sector>& sectors() {
    // Each sector lists its outgoing routes. Reverse access must be listed separately.
    static const std::vector<Sector> board {
        {1, "Barracks", {2, 6, 5}}, {2, "Armory", {1, 3}},
        {3, "Alley", {4}}, {4, "Streets", {5, 7, 8}},
        {5, "Cathedral", {6, 7, 4}}, {6, "Infirmary", {1, 5}},
        {7, "Slums", {9}}, {8, "Market", {4, 3, 9}},
        {9, "Bridge", {7, 8}}
    };
    return board;
}
const Sector& sectorAt(int id) {
    if (id < 1 || id > 9) throw std::out_of_range("Sector must be between 1 and 9.");
    return sectors().at(static_cast<std::size_t>(id - 1));
}
bool Sector::checkConnecting(int destination) const {
    return std::find(destinations.begin(), destinations.end(), destination) != destinations.end();
}
std::vector<int> outgoingRoutes(int sector, bool reversed) {
    sectorAt(sector);
    if (!reversed) return sectorAt(sector).destinations;
    std::vector<int> result;
    for (const auto& origin : sectors()) if (origin.checkConnecting(sector)) result.push_back(origin.id);
    return result;
}
int nextStep(int from, int to, bool reversed) {
    sectorAt(from);
    sectorAt(to);
    std::array<int, 10> previous {};
    std::queue<int> pending;
    pending.push(from);
    previous[from] = from;
    while (!pending.empty()) {
        int current = pending.front(); pending.pop();
        if (current == to) break;
        for (int dest : outgoingRoutes(current, reversed)) {
            if (previous[dest] != 0) continue;
            previous[dest] = current; pending.push(dest);
        }
    }
    if (previous[to] == 0) return from;
    int step = to;
    while (step != from && previous[step] != from) step = previous[step];
    return step;
}
