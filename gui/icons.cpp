#include "draw.h"
#include "game.h"
#include <array>
#include <cmath>
#include <initializer_list>

namespace ui {
Icon sectorIcon(int sector) { return static_cast<Icon>(sector - 1); }
Icon patronIcon(int patron) { return static_cast<Icon>(static_cast<int>(Icon::emberRegent) + patron); }
Icon enemyIcon(int kind) {
    static constexpr std::array<Icon, 15> icons {{Icon::scavenger, Icon::siegeAutomaton, Icon::signalLeech,
        Icon::breachTuner, Icon::anchor, Icon::wireStalker, Icon::rustDrone, Icon::kilnSentinel, Icon::cinderTracker,
        Icon::prismMote, Icon::refractionCrawler, Icon::echoRunner, Icon::harmonicProwler, Icon::rustCarrier, Icon::orchardCrawler}};
    return icons.at(kind);
}
Icon defenderIcon(const Player& p) {
    for (std::size_t i = 0; i < characters().size(); ++i) {
        const auto& c = characters()[i];
        if (p.getMaxHealth() == c.health && p.getBaseArmor() == c.armor && p.getStrength() == c.strength &&
            p.getPrecision() == c.precision && p.getBaseWill() == c.will)
            return static_cast<Icon>(static_cast<int>(Icon::bulwark) + i);
    }
    return Icon::defender;
}
Color sectorColor(int sector) {
    static constexpr std::array<Color, 9> colors {{{138,174,184}, {211,166,104}, {155,165,185},
        {153,179,158}, {187,159,209}, {116,195,178}, {193,144,117}, {193,179,123}, {144,174,205}}};
    return colors.at(sector - 1);
}

// Every symbol uses the same 32-unit drawing space. Keeping the artwork procedural
// gives map tokens, portraits and large card emblems the same visual identity.
void Draw::icon(int x, int y, Icon kind, Color c, float scale) {
    auto px = [&](float v) { return x + static_cast<int>(std::lround(v * scale)); };
    auto py = [&](float v) { return y + static_cast<int>(std::lround(v * scale)); };
    auto l = [&](float a, float b, float d, float e) { line(px(a), py(b), px(d), py(e), c); };
    auto ring = [&](float a, float b, float r, bool solid = false) { circle(px(a), py(b), static_cast<int>(std::lround(r*scale)), c, solid); };
    auto rect = [&](float a, float b, float w, float h, bool solid = false) {
        SDL_Rect box {px(a), py(b), static_cast<int>(std::lround(w*scale)), static_cast<int>(std::lround(h*scale))};
        if (solid) fill(box, c); else outline(box, c);
    };
    auto path = [&](std::initializer_list<SDL_FPoint> vertices) {
        auto a = vertices.begin();
        for (auto b = a + 1; b != vertices.end(); ++a, ++b) l(a->x, a->y, b->x, b->y);
    };
    auto skull = [&] {
        path({{-6,-5},{-4,-9},{4,-9},{6,-5},{5,2},{3,3},{3,6},{-3,6},{-3,3},{-5,2},{-6,-5}});
        l(-4,-3,-2,-2); l(2,-2,4,-3); l(-1,0,0,-2); l(0,-2,1,0);
        for (int i : {-2,0,2}) l(i,3,i,5);
    };
    // Veyra uses work helmets, signal instruments and articulated machines.
    auto visor = [&] {
        path({{-9,7},{-9,-7},{-5,-12},{6,-12},{10,-7},{10,7},{5,12},{-4,12},{-9,7}});
        rect(-6,-5,13,5); l(-4,5,5,5); l(0,5,0,10);
        rect(-13,-1,4,7); rect(10,-1,4,7);
    };
    auto signalCore = [&] {
        path({{0,-12},{8,-4},{8,5},{0,13},{-8,5},{-8,-4},{0,-12}});
        ring(0,0,4); l(-3,0,3,0); l(0,-3,0,3);
        for (int side : {-1,1}) { l(side*12,-9,side*12,9); l(side*12,-9,side*8,-13); }
    };
    auto crawler = [&] {
        rect(-7,-9,14,17); rect(-4,-6,8,5); l(-4,4,4,4);
        for (int side : {-1,1}) for (int row : {-7,0,7}) {
            path({{side*7.0F,static_cast<float>(row)},{side*13.0F,static_cast<float>(row+3)},
                  {side*15.0F,static_cast<float>(row+7)}});
        }
    };
    switch (kind) {
        case Icon::barracks:
            path({{-14,13},{-14,-9},{-10,-9},{-10,-14},{-6,-14},{-6,-9},{6,-9},{6,-14},{10,-14},{10,-9},{14,-9},{14,13},{-14,13}});
            rect(-5,0,10,13); path({{-5,0},{0,-5},{5,0}});
            for (int i : {-10,10}) { l(i,-5,i,1); l(i,5,i,9); }
            l(-14,10,-7,10); l(7,10,14,10); l(0,3,0,12); break;
        case Icon::armory:
            path({{-13,-9},{0,-14},{13,-9},{11,7},{0,15},{-11,7},{-13,-9}});
            path({{-10,10},{9,-11},{10,-5},{-6,11}}); path({{10,10},{-9,-11},{-10,-5},{6,11}});
            l(-11,4,-4,10); l(4,10,11,4); ring(0,-1,2); break;
        case Icon::alley:
            path({{-14,14},{-14,-13},{-7,-10},{-5,14}}); path({{14,14},{14,-13},{7,-10},{5,14}});
            for (int j : {-6,0,6}) { l(-12,j,-9,j+1); l(9,j+1,12,j); }
            l(-3,13,0,-6); l(3,13,0,-6); l(-3,-3,0,-6); l(0,-6,3,-3); l(-14,-13,14,-13); break;
        case Icon::streets:
            for (int sign : {-1,1}) {
                path({{sign*5.0F,-14},{sign*5.0F,-5},{sign*14.0F,-5}});
                path({{sign*5.0F,14},{sign*5.0F,5},{sign*14.0F,5}});
            }
            for (int j : {-12,-8,8,12}) { l(j,-1,j,1); l(-1,j,1,j); }
            ring(0,0,3); l(-12,-10,-9,-13); l(9,13,12,10); break;
        case Icon::cathedral:
            path({{-14,14},{-14,-1},{-10,-7},{-6,-1},{-6,14},{6,14},{6,-1},{10,-7},{14,-1},{14,14},{-14,14}});
            path({{-6,14},{-6,-6},{0,-14},{6,-6},{6,14}});
            ring(0,-3,3); path({{-3,13},{-3,7},{0,3},{3,7},{3,13}});
            l(-10,1,-10,9); l(10,1,10,9); l(-2,-12,2,-12); break;
        case Icon::infirmary:
            path({{-12,-9},{-5,-9},{-5,-13},{5,-13},{5,-9},{12,-9},{14,-7},{14,12},{-14,12},{-14,-7},{-12,-9}});
            rect(-3,-5,6,14,true); rect(-7,-1,14,6,true); l(-11,-5,-11,8); l(11,-5,11,8); l(-5,-11,5,-11); break;
        case Icon::slums:
            path({{-14,14},{-14,-3},{-10,-3},{-10,-11},{-3,-8},{-3,2},{2,2},{2,-13},{10,-13},{10,-2},{14,-2},{14,14},{-14,14}});
            path({{-1,14},{-5,8},{-1,5},{-4,1}}); l(5,-9,7,-9); l(5,-5,7,-5); l(5,0,7,0); l(6,5,11,5);
            l(-8,-4,-6,-4); l(-12,10,-8,10); l(6,10,9,10); break;
        case Icon::market:
            path({{-15,-3},{-10,-12},{10,-12},{15,-3},{-15,-3}});
            for (int i : {-10,-5,0,5,10}) { l(i,-11,i*1.3F,-3); ring(i,-1,2); }
            l(-12,1,-12,13); l(12,1,12,13); rect(-13,8,26,5); rect(-6,1,5,7); ring(6,5,3); break;
        case Icon::bridge:
            l(-15,6,15,6); l(-15,10,15,10);
            for (int i : {-10,10}) { rect(i-2,-13,4,27); l(i,-15,i,-13); }
            path({{-15,-2},{-10,-9},{-5,-3},{0,-1},{5,-3},{10,-9},{15,-2}});
            for (int i : {-5,0,5}) l(i,-2,i,5);
            l(-14,14,-4,14); l(4,14,14,14); break;
        case Icon::bulwark:
            visor(); rect(-13,10,8,5); rect(6,10,8,5); l(-5,-15,6,-15); break;
        case Icon::trailguard:
            visor(); l(-6,-8,6,-8); l(13,-4,13,-15); ring(13,-15,1,true); break;
        case Icon::resonant:
            visor(); ring(-12,-7,3); ring(13,-7,3); l(-6,-15,6,-15); l(-3,-17,3,-17); break;
        case Icon::duskblade:
            visor(); path({{-14,13},{-14,-12},{-11,-16},{-11,13}}); l(-16,7,-9,7); break;
        case Icon::signalLeech:
            signalCore(); path({{0,13},{5,16},{11,12},{15,15}}); break;
        case Icon::breachTuner:
            signalCore(); l(0,-12,0,-17); l(-5,-15,5,-15); l(-4,15,4,15); break;
        case Icon::defender:
            path({{-12,-11},{0,-15},{12,-11},{11,6},{0,15},{-11,6},{-12,-11}});
            ring(0,-5,4); path({{-7,7},{-5,1},{5,1},{7,7},{-7,7}}); l(-3,10,3,10); break;
        case Icon::portal:
            for (int i = 0; i < 12; ++i) {
                float a = i*6.2831853F/12;
                l(12*std::cos(a),14*std::sin(a),15*std::cos(a+.12F),17*std::sin(a+.12F));
            }
            for (int i = 0; i < 48; ++i) {
                float a = i*6.2831853F/48, b = (i+1)*6.2831853F/48;
                l(10*std::cos(a),13*std::sin(a),10*std::cos(b),13*std::sin(b));
            }
            path({{2,-10},{-4,-5},{3,-2},{-3,3},{4,6},{-1,10}}); ring(0,0,3); break;
        case Icon::scavenger:
            visor(); l(-15,13,-15,-12); l(-15,-12,-11,-8); l(-15,-12,-18,-8); break;
        case Icon::siegeAutomaton:
            rect(-10,-12,20,23); rect(-7,-8,14,5); rect(-5,3,10,5);
            rect(-15,-4,5,15); rect(10,-4,5,15); l(-7,14,-2,14); l(2,14,7,14); break;
        case Icon::anchor:
            rect(-11,-13,22,25); rect(-7,-9,14,18); ring(0,-2,4);
            l(0,2,0,16); l(-8,16,8,16); l(-15,-4,-11,-4); l(11,-4,15,-4); break;
        case Icon::wireStalker:
            crawler(); l(-3,-9,-7,-15); l(3,-9,7,-15); break;
        case Icon::cinderTracker:
            crawler(); path({{-4,-10},{0,-17},{4,-10}}); ring(0,2,2,true); break;
        case Icon::harmonicProwler:
            crawler(); l(-3,-15,-3,-10); l(0,-17,0,-10); l(3,-15,3,-10); break;
        case Icon::rustDrone:
            rect(-6,-7,12,16); ring(0,-2,3); ring(-12,-9,4); ring(12,-9,4);
            l(-12,-5,-6,0); l(12,-5,6,0); l(-3,9,-5,15); l(3,9,5,15); break;
        case Icon::kilnSentinel:
            path({{-10,13},{-10,-8},{-6,-12},{6,-12},{10,-8},{10,13},{-10,13}});
            rect(-6,-5,12,12); for (int i : {-3,1,5}) l(-5,i,5,i);
            l(-15,12,-15,-15); l(-15,-15,-11,-11); l(13,-9,13,9); break;
        case Icon::prismMote:
            path({{0,-15},{12,0},{0,15},{-12,0},{0,-15},{0,15}});
            l(-12,0,12,0); l(-15,-11,-10,-11); l(10,11,15,11); break;
        case Icon::refractionCrawler:
            crawler(); path({{0,-16},{6,-8},{0,-2},{-6,-8},{0,-16}}); break;
        case Icon::echoRunner:
            rect(-5,-12,10,10); ring(0,-7,2); l(0,-2,0,6);
            path({{0,2},{-10,-2},{-13,2}}); path({{0,2},{9,6},{13,2}});
            path({{0,6},{-7,10},{-3,15}}); path({{0,6},{7,9},{13,15}}); break;
        case Icon::rustCarrier:
            rect(-10,-12,20,25); rect(-6,-8,12,7); l(-6,4,6,4); l(0,1,0,8);
            path({{-14,-6},{-14,7},{-10,11}}); path({{14,-6},{14,7},{10,11}}); break;
        case Icon::orchardCrawler:
            crawler(); path({{0,-9},{0,-16},{-6,-13},{0,-11},{6,-15}});
            l(-4,2,4,2); l(0,0,0,5); break;
        case Icon::emberRegent:
            // A furnace stack and heat vents, with no heraldic skull or horns.
            path({{-12,14},{-10,-5},{-6,-5},{-6,-15},{6,-15},{6,-5},{10,-5},{12,14},{-12,14}});
            rect(-6,0,12,10); for (int i : {3,6,9}) l(-5,i,5,i);
            l(-15,-10,-12,-6); l(15,-10,12,-6); break;
        case Icon::prismOracle:
            path({{0,-16},{11,-3},{4,14},{-9,6},{-12,-4},{0,-16}});
            path({{0,-16},{0,1},{11,-3}}); l(0,1,4,14); l(0,1,-9,6);
            l(-16,12,-10,12); l(11,-12,16,-12); break;
        case Icon::echoWeaver:
            for (int row : {-9,0,9})
                path({{-15,static_cast<float>(row)},{-8,static_cast<float>(row)},
                      {-4,static_cast<float>(row-5)},{2,static_cast<float>(row+5)},
                      {7,static_cast<float>(row)},{15,static_cast<float>(row)}});
            break;
        case Icon::rustGardener:
            l(0,15,0,-15); path({{0,-9},{-8,-12},{-13,-8}});
            path({{0,-2},{8,-7},{13,-4}}); path({{0,5},{-8,1},{-13,5}});
            path({{0,11},{8,7},{13,11}}); rect(-3,13,6,3); break;
        case Icon::insignia:
            // The city's relay beacon: an open square, mast and signal bars.
            path({{-14,9},{-14,-12},{-7,-12}}); path({{7,-12},{14,-12},{14,9}});
            l(0,-8,0,13); path({{-6,13},{0,7},{6,13},{-6,13}}); ring(0,-9,3);
            l(-8,-6,-8,2); l(8,-6,8,2); l(-11,15,11,15); break;
        case Icon::supplies:
            path({{-13,-7},{0,-12},{13,-7},{13,10},{0,15},{-13,10},{-13,-7},{0,-2},{13,-7}});
            l(0,-2,0,15); l(-6,-9,7,-4); l(7,-4,7,12); l(-9,2,-4,4); l(-9,6,-4,8); break;
        case Icon::search: ring(-3,-3,9); ring(-3,-3,6); path({{4,4},{13,12},{11,15},{2,6}}); l(-6,-6,-1,-6); break;
        case Icon::shield: path({{-13,-12},{0,-16},{13,-12},{11,5},{0,16},{-11,5},{-13,-12}}); l(0,-11,0,10); l(-7,-3,7,-3); break;
        case Icon::clock:
            ring(0,0,13); ring(0,0,10); l(0,0,0,-7); l(0,0,6,3); ring(0,0,2);
            for (int i : {-1,1}) { l(0,i*11,0,i*13); l(i*11,0,i*13,0); } break;
        case Icon::grenade:
            rect(-4,-12,8,4); ring(6,-12,3); path({{-4,-8},{-9,-3},{-9,9},{-4,14},{4,14},{9,9},{9,-3},{4,-8},{-4,-8}});
            for (int i : {-3,3,9}) l(-8,i,8,i);
            l(-3,-5,-3,12); l(3,-5,3,12); break;
        case Icon::skull: skull(); l(-10,12,10,8); l(-10,8,10,12); break;
    }
}
}
