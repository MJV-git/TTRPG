#include "draw.h"
#include "game.h"
#include <array>
#include <cmath>
#include <initializer_list>

namespace ui {
Icon sectorIcon(int sector) { return static_cast<Icon>(sector - 1); }
Icon patronIcon(int patron) { return static_cast<Icon>(static_cast<int>(Icon::khorne) + patron); }
Icon enemyIcon(int kind) {
    static constexpr std::array<Icon, 15> icons {{Icon::cultist, Icon::chaosMarine, Icon::sorcerer,
        Icon::acolyte, Icon::warden, Icon::hound, Icon::drone, Icon::bloodletter, Icon::fleshHound,
        Icon::horror, Icon::flamer, Icon::daemonette, Icon::fiend, Icon::plaguebearer, Icon::beast}};
    return icons.at(kind);
}
Icon defenderIcon(const Player& p) {
    for (std::size_t i = 0; i < characters().size(); ++i) {
        const auto& c = characters()[i];
        if (p.getMaxHealth() == c.health && p.getBaseArmor() == c.armor && p.getStrength() == c.strength &&
            p.getPrecision() == c.precision && p.getBaseWill() == c.will)
            return static_cast<Icon>(static_cast<int>(Icon::marine) + i);
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
    auto hood = [&] { path({{-10,8},{-8,-4},{0,-14},{8,-4},{10,8},{0,14},{-10,8}}); skull(); };
    auto helm = [&] {
        path({{-8,-3},{-7,-10},{-3,-13},{3,-13},{7,-10},{8,-3},{6,7},{0,11},{-6,7},{-8,-3}});
        l(-6,-4,-1,-2); l(1,-2,6,-4); rect(-3,1,6,5); l(0,1,0,6);
        l(-11,1,-14,4); l(-14,4,-14,11); l(-14,11,-7,14);
        l(11,1,14,4); l(14,4,14,11); l(14,11,7,14);
    };
    auto horns = [&] { path({{-6,-6},{-13,-15},{-12,-5},{-7,0}}); path({{6,-6},{13,-15},{12,-5},{7,0}}); };
    auto beastHead = [&] {
        path({{-10,-10},{-7,-4},{-11,1},{-7,9},{0,14},{7,9},{11,1},{7,-4},{10,-10},{3,-6},{-3,-6},{-10,-10}});
        l(-7,0,-2,2); l(2,2,7,0); path({{-3,7},{0,5},{3,7},{0,10},{-3,7}});
        l(-5,6,-4,10); l(5,6,4,10);
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
        case Icon::marine: helm(); rect(-4,-12,8,3); l(-11,5,-8,5); l(8,5,11,5); break;
        case Icon::guardsman:
            path({{-12,-5},{-9,-6},{-8,-11},{-4,-14},{5,-14},{9,-10},{10,-6},{13,-5},{-12,-5}});
            path({{-7,-4},{-6,7},{0,12},{6,7},{7,-4}}); l(-5,-1,-1,-1); l(1,-1,5,-1);
            rect(-4,4,8,3); l(-14,14,-7,10); l(7,10,14,14); l(0,-12,0,-8); break;
        case Icon::psyker: case Icon::sorcerer: case Icon::acolyte:
            hood(); ring(0,-4,2); l(-13,-8,-15,0); l(-15,0,-11,4); l(13,-8,15,0); l(15,0,11,4);
            if (kind == Icon::sorcerer) horns();
            if (kind == Icon::psyker) { ring(0,-1,16); l(-3,10,3,10); }
            if (kind == Icon::acolyte) { l(-3,11,0,8); l(0,8,3,11); } break;
        case Icon::sister:
            helm(); path({{0,-15},{-3,-10},{0,-6},{3,-10},{0,-15}});
            path({{-8,11},{-3,9},{0,14},{3,9},{8,11}}); l(-12,5,-9,8); l(9,8,12,5); break;
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
        case Icon::cultist: hood(); l(-12,-12,-9,-10); l(9,-10,12,-12); break;
        case Icon::chaosMarine: helm(); horns(); l(-14,8,-10,11); l(10,11,14,8); break;
        case Icon::warden:
            path({{-14,-12},{0,-15},{14,-12},{12,6},{0,16},{-12,6},{-14,-12}});
            skull(); l(-10,-7,-9,4); l(10,-7,9,4); path({{-6,9},{0,13},{6,9}}); break;
        case Icon::hound: case Icon::fleshHound: case Icon::fiend:
            beastHead();
            if (kind == Icon::fleshHound) { horns(); l(-12,7,-8,13); l(8,13,12,7); }
            if (kind == Icon::fiend) { l(-13,-14,-5,-5); l(13,-14,5,-5); path({{0,10},{-2,15},{4,16}}); }
            break;
        case Icon::drone:
            ring(0,2,8); ring(0,0,3); ring(-9,-8,5); ring(9,-8,5);
            l(-9,-13,-9,-3); l(-14,-8,-4,-8); l(9,-13,9,-3); l(4,-8,14,-8);
            path({{-6,7},{-12,10},{-14,15}}); path({{6,7},{12,10},{14,15}}); l(0,10,0,15); break;
        case Icon::bloodletter: skull(); horns(); path({{-8,2},{-6,10},{0,15},{6,10},{8,2}}); l(0,6,0,12); break;
        case Icon::horror:
            path({{-12,13},{-8,2},{-13,-3},{-5,-5},{-7,-13},{1,-8},{7,-14},{8,-6},{14,-2},{8,3},{12,13},{0,9},{-12,13}});
            ring(-3,-1,2); ring(4,-2,3); path({{-5,5},{0,3},{5,5},{0,7},{-5,5}}); break;
        case Icon::flamer:
            path({{-11,13},{-14,4},{-9,-2},{-11,-10},{-4,-6},{0,-16},{5,-5},{10,-11},{9,0},{14,6},{9,14},{-11,13}});
            ring(0,3,5); ring(0,3,2); l(-5,10,-2,8); l(3,9,6,11); break;
        case Icon::daemonette:
            skull(); path({{-5,-7},{-10,-13},{-14,-9},{-10,-4}}); path({{5,-7},{10,-13},{14,-9},{10,-4}});
            path({{-8,1},{-12,8},{-8,14},{-4,8}}); path({{8,1},{12,8},{8,14},{4,8}}); l(0,7,0,14); break;
        case Icon::plaguebearer:
            path({{-10,-7},{-6,-12},{-2,-9},{0,-16},{3,-9},{9,-8},{12,5},{7,14},{-7,14},{-12,5},{-10,-7}});
            ring(0,-4,3); l(-6,3,6,3); for (int i : {-4,0,4}) l(i,3,i,7);
            ring(-6,10,1,true); ring(6,10,1,true); break;
        case Icon::beast:
            path({{-15,12},{-12,4},{-13,-4},{-7,-10},{-2,-9},{3,-13},{10,-8},{13,2},{11,8},{15,13},{-15,12}});
            ring(-4,-3,2); ring(4,-4,2); path({{-7,3},{0,1},{7,3},{5,8},{-3,8},{-7,3}});
            path({{0,5},{0,12},{6,15},{10,13}}); ring(-8,9,1,true); ring(8,-1,1,true); break;
        case Icon::khorne:
            skull(); path({{-15,-13},{-8,-13},{-8,-6},{-3,-6}}); path({{15,-13},{8,-13},{8,-6},{3,-6}});
            path({{-14,1},{-8,11},{0,16},{8,11},{14,1}}); l(-10,9,-14,14); l(10,9,14,14); break;
        case Icon::tzeentch:
            path({{-14,0},{-6,-6},{7,-6},{14,0},{7,6},{-6,6},{-14,0}}); ring(1,0,4); ring(1,0,1,true);
            path({{-5,-7},{-4,-13},{2,-16},{7,-12}}); path({{4,7},{2,12},{-5,15},{-10,12}}); l(-11,-5,-14,-10); break;
        case Icon::slaanesh:
            ring(-3,-3,9); path({{2,-12},{9,-14},{12,-9},{7,-5}});
            l(-3,6,-3,16); l(-8,11,2,11); path({{6,4},{13,8},{9,14}}); ring(-3,-3,3); break;
        case Icon::nurgle:
            for (const auto& p : std::array<SDL_Point,3>{{{0,-7},{-8,7},{8,7}}}) { ring(p.x,p.y,6); ring(p.x,p.y,2); }
            l(-5,-2,-3,2); l(5,-2,3,2); l(-2,7,2,7); break;
        case Icon::insignia:
            path({{-3,-14},{0,-17},{3,-14},{2,7},{0,14},{-2,7},{-3,-14}});
            for (int side : {-1,1}) for (int i = 0; i < 4; ++i)
                path({{side*4.0F,-5.0F+i*3},{side*(16.0F-i*2),-10.0F+i*4},{side*(14.0F-i*2),-5.0F+i*4},{side*4.0F,0.0F+i*2}});
            l(-5,8,5,8); ring(0,-5,3); break;
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
        case Icon::skull: skull(); horns(); l(-10,12,10,8); l(-10,8,10,12); break;
    }
}
}
