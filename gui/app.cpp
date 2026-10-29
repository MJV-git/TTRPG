#include "app.h"
#include <set>
#include "campaign_io.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
using namespace ui;
std::string num(int value) { return std::to_string(value); }
bool inside(SDL_Rect box, int x, int y) { return x >= box.x && y >= box.y && x < box.x + box.w && y < box.y + box.h; }
const std::array<SDL_Point, 9> positions {{{164, 252}, {466, 252}, {800, 252}, {800, 422}, {466, 422}, {164, 422}, {164, 594}, {800, 594}, {466, 594}}};
const std::array<const char*, 9> descriptions {{
    "+2 armor for defenders.", "+5 true damage on defender attacks.",
    "+20% ranged accuracy for defenders.", "Move along outgoing roads for free.",
    "+4 Will for defenders.", "Rest fully heals and cures rot.",
    "+2 physical damage on all melee attacks.", "Take one of three offers. Costs an action; refreshes each defender turn.",
    "Use Collapse Bridge with a krak grenade. Defenders here die permanently. If the god is here, it loses half its current HP and moves to Slums."
}};

}

Gui::Gui(SDL_Window* window, SDL_Renderer* renderer, const std::string& font, std::uint32_t seed, bool fixed)
    : window(window), renderer(renderer), draw(renderer, font), initialSeed(seed), fixedSeed(fixed) {
    for (int i = 0; i < 4; ++i) { recruits[i].preset = i; recruits[i].name = characters().at(i).name; }
    if (fixedSeed) seedText = std::to_string(seed);
}
void Gui::toast(const std::string& message, bool error) { notice = message; noticeError = error; }
void Gui::clearFocus() { focused = nullptr; focusId.clear(); SDL_StopTextInput(); }
void Gui::openModal(Modal value) { modal = value; clearFocus(); logOffset = 0; }
void Gui::button(const std::string& id, SDL_Rect box, const std::string& label, std::function<void()> action,
                 bool enabled, bool primary, const std::string& tip) {
    if (tutorialStep >= 0 && id.rfind("tutorial-", 0) != 0 && id.rfind("inventory-", 0) != 0 &&
        id.rfind("equip-", 0) != 0 && id != "card-resolve" && id != "skip-animation") enabled = false;
    const bool hover = inside(box, mouseX, mouseY);
    const Color fill = !enabled ? Color{22, 29, 33} : primary ? (hover ? Color{237, 197, 130} : gold) : hover ? Color{40, 53, 59} : raised;
    draw.fill(box, fill);
    draw.outline(box, !enabled ? Color{37, 46, 50} : primary ? gold : hover ? muted : border);
    const int size = 16;
    const auto clipped = draw.elide(label, box.w - 16, size);
    draw.label(box.x + (box.w - draw.measure(clipped, size)) / 2, box.y + (box.h - size) / 2 - 2,
               clipped, size, !enabled ? Color{91, 106, 112} : primary ? background : text);
    buttons.push_back({id, box, enabled, std::move(action), tip});
}
void Gui::field(const std::string& id, SDL_Rect box, std::string& value, int limit, const std::string& placeholder) {
    draw.fill(box, background); draw.outline(box, focused == &value ? gold : border);
    const std::string shown = value.empty() && focused != &value ? placeholder : value;
    draw.label(box.x + 12, box.y + 10, draw.elide(shown, box.w - 24, 17), 17, value.empty() ? muted : text);
    if (focused == &value && (SDL_GetTicks() / 500) % 2 == 0) {
        const int cursor = std::min(box.x + box.w - 12, box.x + 12 + draw.measure(shown, 17));
        draw.line(cursor, box.y + 9, cursor, box.y + box.h - 9, gold);
    }
    buttons.push_back({id, box, true, [this, &value, limit, id] {
        focused = &value; focusId = id; focusLimit = limit; SDL_StartTextInput();
    }, "Type to edit. Ctrl+A clears the field."});
}
void Gui::heading(const std::string&, const std::string& title, const std::string&) {
    draw.label(38, 52, title, 35, text);
    draw.line(40, 137, 1400, 137, border);
}
void Gui::gatherLog() {
    if (!game) return;
    const auto incoming = game->takeLog();
    recentLog = incoming;
    for (const auto& message : incoming) if (message.find("ENEMY LAST STAND /") == 0 || message.find("THE RIFT REOPENS /") == 0 || message.find("LAST STAND ENDS /") == 0) toast(message);
    history.insert(history.end(), incoming.begin(), incoming.end());
    if (history.size() > 500) history.erase(history.begin(), history.begin() + static_cast<long>(history.size() - 500));
    logOffset = 0;
}
void Gui::syncSelection() {
    if (!game) return;
    selectedSector = game->currentPlayer().getSector(); selectedTarget = 0; targetPage = 0;
    for (const auto& e : game->getEnemies()) if (e.getSector() == selectedSector) { selectedTarget = e.getId(); break; }
    if (selectedTarget == 0) for (const auto& p : game->getPortals()) if (p.sector == selectedSector) selectedTarget = -1;
    if (game->hasLivingBoss() && game->getBoss()->getSector() == selectedSector) selectedTarget = -2;
}
void Gui::selectSector(int id) {
    selectedSector = id; selectedTarget = 0; targetPage = 0;
    for (const auto& e : game->getEnemies()) if (e.getSector() == id) { selectedTarget = e.getId(); break; }
    if (selectedTarget == 0) for (const auto& p : game->getPortals()) if (p.sector == id) selectedTarget = -1;
    if (game->hasLivingBoss() && game->getBoss()->getSector() == id) selectedTarget = -2;
}
bool Gui::save(bool announce) {
    if (tutorialStep >= 0) return true;
    if (!game) return true;
    try {
        saveCampaign(*game, savePath);
        if (announce) toast("Campaign saved: " + savePath);
        return true;
    } catch (const std::exception& e) { toast(std::string("Save failed: ") + e.what(), true); return false; }
}
void Gui::load(const std::string& path) {
    try {
        auto restored = loadCampaign(path);
        game = std::move(restored); savePath = path; archivePath = path;
        screen = Screen::battle; modal = Modal::none; clearFocus();
        history.clear(); gatherLog(); syncSelection(); resumePresentation(); toast("Campaign restored. The watch continues.");
    } catch (const std::exception& e) { toast(e.what(), true); }
}
void Gui::perform(Action action) {
    if (!acceptsOrders()) return;
    beforeAnimation = *game;
    actingPlayer = static_cast<int>(game->getPlayerIndex());
    const auto result = game->act(action, false);
    if (!result.accepted) { beforeAnimation.reset(); toast(result.error + " No action spent.", true); return; }
    const std::array<const char*, 15> labels {{"MOVEMENT", "WEAPON ATTACK", "ATTACK THE RIFT", "ATTACK THE AVATAR", "SCAVENGE", "REST", "MEDKIT", "KRAK GRENADE", "KRAK GRENADE", "KRAK GRENADE", "HOLD POSITION", "TAKE COVER", "SUPPRESSING FIRE", "MARKET SUPPLIES", "BRIDGE COLLAPSE"}};
    actionLabel = labels.at(static_cast<std::size_t>(action.type));
    animatedAction = action.type;
    notice.clear(); gatherLog(); beginStage(Presentation::action);
    if (autoSave) save();
}
void Gui::cycleRecruit(int index, int direction) {
    auto& recruit = recruits[index];
    const auto oldName = recruit.preset == 4 ? "Custom Defender" : characters()[recruit.preset].name;
    recruit.preset = (recruit.preset + direction + 5) % 5;
    if (recruit.name == oldName || recruit.name.empty()) recruit.name = recruit.preset == 4 ? "Custom Defender" : characters()[recruit.preset].name;
    clearFocus();
}
void Gui::startQuick() {
    Game candidate(BossId::khorne, fixedSeed ? initialSeed : std::random_device{}());
    const auto recruit = quickDefender(candidate.getSeed(), lastQuickPreset); lastQuickPreset = recruit.preset;
    candidate.addPreset(recruit.preset, recruit.name);
    for (int i=1; i<quickPlayerCount; ++i) {
        const auto member=quickDefender(candidate.getSeed()+static_cast<std::uint32_t>(i), (recruit.preset+i-1)%4);
        candidate.addPreset(member.preset,member.name);
    }
    candidate.start();
    game = std::move(candidate); screen = Screen::battle; modal = Modal::none;
    clearFocus(); history.clear(); gatherLog(); syncSelection(); beginStage(Presentation::idle);
    toast("Select a sector to inspect it.");
}
void Gui::deploy() {
    std::uint32_t seed = std::random_device{}();
    if (!seedText.empty()) {
        const auto parsed = std::from_chars(seedText.data(), seedText.data() + seedText.size(), seed);
        if (parsed.ec != std::errc{} || parsed.ptr != seedText.data() + seedText.size()) { toast("Seed must be a whole number between 0 and 4294967295, or blank for random.", true); return; }
    }
    Game candidate(static_cast<BossId>(selectedBoss), seed);
    for (int i = 0; i < squadSize; ++i) {
        const auto& r = recruits[i];
        const auto& s = r.stats;
        const bool added = r.preset == 4 ? candidate.addCustom(s[0], s[1], s[2], s[3], s[4], r.name, characters()[r.startingWeapon].weapon) : candidate.addPreset(r.preset, r.name);
        if (!added) { toast("Defender " + num(i + 1) + ": use a callsign of 1-32 characters and a custom build within 50 points.", true); return; }
    }
    candidate.start(); game = std::move(candidate); screen = Screen::battle; modal = Modal::none;
    clearFocus(); history.clear(); gatherLog(); syncSelection(); notice.clear(); beginStage(Presentation::handoff);
    if (autoSave) save();
}
void Gui::quit() {
    if (game && !save()) { modal = Modal::pause; return; }
    running = false;
}
void Gui::titleScreen() {
    if (!menuArtwork) {
        // Cache a supersampled illustration: smooth linework without redrawing
        // the radial gradients every frame, including on software renderers.
        menuArtwork.reset(SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,Width*2,Height*2));
        if (!menuArtwork) throw std::runtime_error(SDL_GetError());
        SDL_Texture* original=SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer,menuArtwork.get()); SDL_RenderSetLogicalSize(renderer,Width,Height);
        // Engraved watch crest: layered metal, a gothic arch and a restrained amber glow.
        // Keep the illustration procedural so it scales with the existing icon system.
        draw.fill({0,0,Width,924},{10,16,20});
        for (int r=365;r>0;r-=3) {
            const auto light=static_cast<unsigned char>(9+(365-r)/22);
            draw.circle(1045,423,r,{light,static_cast<unsigned char>(light+6),static_cast<unsigned char>(light+8)},true);
        }
        for (int side : {-1,1}) {
            const int x=1045+side*300;
            draw.line(x,230,x,783,{58,65,62}); draw.line(x+side*8,240,x+side*8,786,{28,39,42});
            draw.line(x,230,1045,86,{58,65,62}); draw.line(x+side*8,240,1045,72,{28,39,42});
            draw.line(x,783,1045,855,{58,65,62});
            for (int i=0;i<5;++i) {
                const int y=310+i*88;
                draw.line(x-side*8,y,x+side*8,y+12,{106,92,64});
                draw.line(x+side*8,y+12,x-side*8,y+24,{106,92,64});
            }
        }
        const int cx=1045,cy=405;
        draw.circle(cx+7,cy+12,234,{4,9,12},true);
        draw.circle(cx,cy,234,{89,77,53},true);
        draw.circle(cx,cy,231,{24,31,33},true);
        draw.circle(cx,cy,219,{164,132,80}); draw.circle(cx,cy,215,{67,64,50});
        draw.circle(cx,cy,190,{47,55,55}); draw.circle(cx,cy,184,{92,81,57});
        for (int i=0;i<72;++i) {
            const double a=i*6.283185307/72;
            const int inner=i%6==0 ? 197 : 205;
            draw.line(cx+static_cast<int>(inner*std::cos(a)),cy+static_cast<int>(inner*std::sin(a)),
                      cx+static_cast<int>(212*std::cos(a)),cy+static_cast<int>(212*std::sin(a)),i%6==0 ? gold : Color{89,84,66});
        }
        for (int side : {-1,1}) {
            for (int feather=0;feather<9;++feather) {
                const int outer=cx+side*(173-feather*10), top=cy-62+feather*12;
                const int inner=cx+side*(55+feather*2), bottom=cy+26+feather*7;
                draw.line(inner+3,bottom+4,outer+3,top+4,{5,12,15});
                draw.line(inner,bottom,outer,top,{185,145,84});
                draw.line(inner,bottom+3,outer,top+4,{90,77,51});
                draw.line(outer,top,outer-side*7,top+22,{223,185,117});
            }
            // Laurel leaves along the lower rim.
            for (int leaf=0;leaf<8;++leaf) {
                const double a=0.20+leaf*0.115;
                const int x=cx+side*static_cast<int>(163*std::cos(a)), y=cy+static_cast<int>(163*std::sin(a));
                draw.line(x,y,x-side*14,y-15,{153,130,83});
                draw.line(x,y,x+side*4,y-22,{153,130,83});
                draw.line(x-side*14,y-15,x+side*4,y-22,{80,78,57});
            }
        }
        // A gilded blade behind the defender's helmet, with a raised central shield.
        draw.line(cx,cy-169,cx-14,cy-139,gold); draw.line(cx,cy-169,cx+14,cy-139,gold);
        draw.line(cx-14,cy-139,cx-11,cy+120,gold); draw.line(cx+14,cy-139,cx+11,cy+120,gold);
        draw.line(cx,cy-154,cx,cy+130,{242,208,147});
        draw.line(cx-39,cy+106,cx+39,cy+106,gold); draw.circle(cx,cy+142,8,gold);
        draw.circle(cx,cy-9,87,{12,22,27},true); draw.circle(cx,cy-9,88,{127,109,76});
        draw.icon(cx+2,cy-6,Icon::marine,{5,10,13},4.4F);
        draw.icon(cx,cy-9,Icon::marine,{221,196,147},4.4F);
        draw.circle(cx,cy-9,94,{62,67,59});
        for (int x : {cx-222,cx+222}) { draw.circle(x,cy,6,gold,true); draw.circle(x,cy,3,{49,46,35},true); }
        draw.icon(cx,cy-222,Icon::insignia,gold,0.65F);
        draw.line(837,731,1253,731,{84,76,56});
        for (int i=0;i<4;++i) {
            const int x=895+i*100;
            draw.line(x-32,788,x,756,{70,75,66}); draw.line(x,756,x+32,788,{70,75,66});
            draw.line(x+32,788,x,820,{70,75,66}); draw.line(x,820,x-32,788,{70,75,66});
            draw.icon(x,788,static_cast<Icon>(static_cast<int>(Icon::marine)+i),{174,157,118},1.15F);
        }
        SDL_SetRenderTarget(renderer,original); SDL_RenderSetLogicalSize(renderer,Width,Height);
    }
    SDL_RenderCopy(renderer,menuArtwork.get(),nullptr,nullptr);
    // Fine framing and a clean control column keep the menu readable.
    draw.line(76,134,516,134,{86,77,55}); draw.icon(93,134,Icon::insignia,gold,0.8F);
    draw.label(74,188,"THE LAST",58,text); draw.label(70,252,"WATCH",100,gold);
    draw.line(76,382,516,382,{86,77,55});
    button("new",{76,419,440,52},"New campaign",[this] { screen=Screen::setup; notice.clear(); },true,true);
    draw.fill({76,489,440,113},panel); draw.outline({76,489,440,113},border);
    draw.label(94,504,"PLAYERS",12,muted);
    for (int i=1;i<=4;++i) button("quick-count-"+num(i),{266+(i-1)*56,499,46,32},num(i),[this,i] { quickPlayerCount=i; },true,quickPlayerCount==i);
    button("quick",{92,547,408,40},"Quick deployment",[this] { startQuick(); });
    button("continue",{76,621,440,46},game ? "Return to watch" : "Continue save",[this] {
        if (game) {screen=Screen::battle;syncSelection();resumePresentation();} else load(savePath);
    },game || std::filesystem::exists(savePath));
    button("tutorial",{76,683,214,44},"Tutorial",[this] { startTutorial(); });
    button("manual",{302,683,214,44},"Field manual",[this] { openModal(Modal::help); });
    button("title-archive",{76,743,440,42},"Load campaign",[this] { archivePath=savePath;openModal(Modal::archive); });
    button("exit",{76,801,440,42},"Exit",[this] { if(game) openModal(Modal::pause);else running=false; });
}
void Gui::setupScreen() {
    heading("MUSTER / NEW CAMPAIGN", "New campaign", "Select an adversary, assemble your defenders, and deploy into the district.");
    button("setup-back", {1260, 48, 140, 44}, "Back", [this] { screen = Screen::title; clearFocus(); });
    draw.label(40, 160, "ADVERSARY", 13, gold);
    for (int i = 0; i < 4; ++i) {
        const auto& b = bosses()[i];
        SDL_Rect box {40 + i * 344, 192, 328, 188};
        draw.fill(box, selectedBoss == i ? Color{40, 39, 32} : panel);
        draw.outline(box, selectedBoss == i ? gold : border);
        draw.icon(box.x + 28, box.y + 32, patronIcon(i), selectedBoss == i ? gold : muted);
        draw.label(box.x + 54, box.y + 17, b.name, 25, selectedBoss == i ? gold : text);
        draw.label(box.x + 18, box.y + 63, "DOOM LIMIT  " + num(b.deadline), 13, muted);
        draw.paragraph(box.x + 18, box.y + 90, b.rule, box.w - 36, 16, text, 3);
        buttons.push_back({"boss-" + num(i), box, true, [this, i] { selectedBoss = i; clearFocus(); }, "Select " + b.name});
    }
    draw.label(40, 408, "DEFENDERS", 13, gold);
    draw.label(1040, 408, "SQUAD SIZE", 13, muted);
    button("squad-minus", {1150, 397, 42, 36}, "-", [this] { --squadSize; clearFocus(); }, squadSize > 1);
    draw.label(1214, 401, num(squadSize), 22, text);
    button("squad-plus", {1255, 397, 42, 36}, "+", [this] { ++squadSize; clearFocus(); }, squadSize < 4);
    draw.label(40, 450, "DEFENDER", 12, muted); draw.label(434, 450, "CALLSIGN", 12, muted);
    draw.label(795, 450, "PROFILE", 12, muted);
    for (int i = 0; i < squadSize; ++i) {
        const int y = 478 + i * 73;
        auto& r = recruits[i];
        draw.fill({40, y, 1360, 64}, panel); draw.label(56, y + 19, "P" + num(i + 1), 17, gold);
        button("class-prev-" + num(i), {106, y + 10, 38, 44}, "<", [this, i] { cycleRecruit(i, -1); });
        button("class-" + num(i), {150, y + 10, 208, 44}, r.preset == 4 ? "Custom defender" : characters()[r.preset].name,
            [this, i] { cycleRecruit(i, 1); }, true, false, "Choose a defender. Use either arrow to cycle.");
        button("class-next-" + num(i), {364, y + 10, 38, 44}, ">", [this, i] { cycleRecruit(i, 1); });
        draw.icon(416, y + 32, static_cast<Icon>(static_cast<int>(Icon::marine) + r.preset), teal, 0.6F);
        field("name-" + num(i), {430, y + 10, 330, 44}, r.name, 32, "Enter a callsign");
        if (r.preset == 4) {
            const auto& s = r.stats;
            draw.label(795, y + 10, "HP " + num(1 + 3*s[0]) + "   ARM " + num(s[1]) + "   STR " + num(s[2]) + "   PRE " + num(s[3]) + "   WILL " + num(s[4]), 15, muted);
            button("custom-" + num(i), {795, y + 33, 250, 26}, "Allocate 50 stat points", [this, i] { customRecruit = i; openModal(Modal::custom); });
        } else {
            const auto& c = characters()[r.preset];
            draw.label(795, y + 10, "HP " + num(c.health) + "   ARM " + num(c.armor) + "   STR " + num(c.strength) + "   PRE " + num(c.precision) + "   WILL " + num(c.will), 15, muted);
            draw.label(795, y + 33, weapon(c.weapon).name, 16, text);
        }
    }
    draw.line(40, 796, 1400, 796, border);
    draw.label(40, 824, "SCENARIO SEED", 12, muted);
    field("seed", {40, 849, 240, 44}, seedText, 10, "Random");
    button("deploy", {1030, 831, 370, 62}, "DEPLOY THE WATCH", [this] { deploy(); }, true, true);
}
void Gui::board() {
    draw.fill({24, 170, 960, 510}, (game->hasLivingBoss() || game->isLastStand()) ? Color{36, 19, 29} : Color{15, 23, 28}); draw.outline({24, 170, 960, 510}, (game->hasLivingBoss() || game->isLastStand()) ? red : border);
    draw.label(42, 175, "SECTOR MAP", 12, muted);
    if (game->getReversedRounds()) draw.label(535, 175, "ROADS REVERSED / " + num(game->getReversedRounds()), 12, purple);
    for (const auto& s : sectors()) {
        for (int dest : game->getRoutes(s.id)) {
            const auto reverseRoutes = game->getRoutes(dest);
            const bool reciprocal = std::find(reverseRoutes.begin(), reverseRoutes.end(), s.id) != reverseRoutes.end();
            if (reciprocal && s.id > dest) continue;
            const auto a = positions[s.id - 1], b = positions[dest - 1];
            Color c = s.id == selectedSector || (reciprocal && dest == selectedSector) ? gold : Color{72, 91, 98};
            if ((s.id == 8 && dest == 3) || (s.id == 3 && dest == 8)) {
                draw.line(a.x + 90, a.y, 944, a.y, c);
                draw.arrow(944, static_cast<float>(a.y), 944, static_cast<float>(b.y), c);
                draw.line(944, b.y, b.x + 90, b.y, c);
            } else {
                draw.arrow(static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(b.x), static_cast<float>(b.y), c);
                if (reciprocal) draw.arrow(static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(a.x), static_cast<float>(a.y), c);
            }
        }
    }
    const auto& active = game->currentPlayer();
    const bool defenderTurn = game->getOutcome()==Outcome::ongoing && game->getPhase()==TurnPhase::players &&
        (presentation==Presentation::idle || presentation==Presentation::handoff);
    for (const auto& s : sectors()) {
        const auto at = positions[s.id - 1];
        SDL_Rect box {at.x - 106, at.y - 56, 212, 112};
        const bool selected = selectedSector == s.id;
        const bool reachable = game->canMove(active.getSector(), s.id);
        const Color accent = sectorColor(s.id);
        int portalHP = 0;
        std::vector<const Enemy*> localEnemies;
        for (const auto& p : game->getPortals()) if (p.sector == s.id) portalHP = p.health;
        for (const auto& e : game->getEnemies()) if (e.getSector() == s.id) localEnemies.push_back(&e);
        const bool boss = game->hasLivingBoss() && game->getBoss()->getSector() == s.id;
        draw.fill({box.x + 4, box.y + 5, box.w, box.h}, {6, 10, 13, 210});
        draw.fill(box, game->getClosure(s.id) ? Color{48, 23, 29} : game->isLastStand() ? Color{37, 23, 31} : panel);
        draw.fill({box.x + 1, box.y + 1, 210, 27}, {accent.r, accent.g, accent.b, 22});
        draw.outline(box, selected ? gold : reachable ? Color{88, 108, 107} : border);
        draw.line(box.x + 9, box.y + 28, box.x + 203, box.y + 28, {accent.r, accent.g, accent.b, 65});
        // Reinforced corner brackets and an engraved sector badge stay inside each tile.
        for (int side : {0,1}) {
            const int cx = box.x + (side ? 211 : 0), dir = side ? -1 : 1;
            draw.line(cx, box.y, cx + dir*11, box.y, selected ? gold : accent);
            draw.line(cx, box.y + 111, cx + dir*11, box.y + 111, selected ? gold : accent);
        }
        draw.label(box.x + 11, box.y + 5, s.name, 16, text);
        draw.label(box.x + 183, box.y + 7, "0" + num(s.id), 11, accent);
        draw.circle(box.x + 32, box.y + 57, 24, {accent.r, accent.g, accent.b, 12}, true);
        draw.icon(box.x + 32, box.y + 57, sectorIcon(s.id), accent, 1.35F);
        static const std::array<const char*, 9> roles {{"ARMOR +2", "TRUE DAMAGE +5", "RANGED AIM +20%", "FREE EXITS", "WILL +4", "FULL REST", "MELEE DAMAGE +2", "3 MARKET OFFERS", "COLLAPSIBLE"}};
        draw.label(box.x + 65, box.y + 34, game->isBridgeCollapsed() && s.id == 9 ? "COLLAPSED" : game->getClosure(s.id) ? "CORDON / " + num(game->getClosure(s.id)) : roles[s.id-1], 10, game->getClosure(s.id) ? red : accent);
        if (!localEnemies.empty()) {
            const int shown = std::min(3, static_cast<int>(localEnemies.size()));
            for (int i = 0; i < shown; ++i) draw.icon(box.x + 77 + i*26, box.y + 65, enemyIcon(localEnemies[i]->getKind()), red, 0.68F);
            if (localEnemies.size() > 3) draw.label(box.x + 142, box.y + 59, "+" + num(static_cast<int>(localEnemies.size()) - 3), 10, red);
        }
        draw.line(box.x + 10, box.y + 82, box.x + 202, box.y + 82, border);
        int px = box.x + 18;
        for (std::size_t i = 0; i < game->getPlayers().size(); ++i) if (game->getPlayers()[i].getAlive() && game->getPlayers()[i].getSector() == s.id) {
            const bool low = game->getPlayers()[i].getHealth()*3 <= game->getPlayers()[i].getMaxHealth();
            const bool activeToken = i == game->getPlayerIndex() && defenderTurn;
            const Color ink = activeToken ? ((animationClock/450)%2 == 0 ? Color{105,255,110} : Color{35,110,44}) :
                low && (animationClock/400)%2 == 0 ? red : teal;
            draw.icon(px, box.y + 96, defenderIcon(game->getPlayers()[i]), ink, 0.65F);
            draw.label(px + 11, box.y + 92, num(static_cast<int>(i + 1)), 10, ink); px += 32;
        }
        if (s.id == 9 && game->isBridgeCollapsed()) {
            draw.line(box.x+5,box.y+30,box.x+207,box.y+107,red);
            draw.line(box.x+207,box.y+30,box.x+5,box.y+107,red);
        }
        if (portalHP) {
            draw.icon(box.x + 181, box.y + 62, Icon::portal, purple, 0.8F);
            draw.label(box.x + 195, box.y + 58, num(portalHP), 10, purple);
        }
        if (boss) {
            draw.icon(box.x + 159, box.y + 96, patronIcon(static_cast<int>(game->getBoss()->getKind())), red, 0.7F);
            draw.label(box.x + 174, box.y + 92, "GOD", 10, red);
        } else if (game->getClosure(s.id)) {
            for (int i = 0; i < 5; ++i) draw.line(box.x + 158+i*8,box.y+102,box.x+163+i*8,box.y+90,red);
        }
        buttons.push_back({"sector-" + num(s.id), box, true, [this, id = s.id] { selectSector(id); }, "Inspect " + s.name + ". Selecting does not spend an action."});
    }
    boardEffects();
    std::string conditions;
    if (game->getPlayerAimRounds()) conditions = "ASHFALL / Defender accuracy -20%";
    if (game->getEnemyAimRounds()) conditions += (conditions.empty() ? "" : "  |  ") + std::string("FLARE / Enemy accuracy -20%");
    if (game->getTransportRounds()) conditions += (conditions.empty() ? "" : "  |  ") + std::string("TRANSPORT / Fly to any sector");
    if (conditions.empty()) {
        draw.icon(50, 665, Icon::defender, teal, 0.55F); draw.label(66, 658, "Defender", 12, muted);
        draw.icon(174, 665, Icon::cultist, red, 0.55F); draw.label(190, 658, "Enemy", 12, muted);
        draw.icon(282, 665, Icon::portal, purple, 0.55F); draw.label(298, 658, "Portal / HP", 12, muted);
        draw.icon(421, 665, patronIcon(static_cast<int>(game->getBossDef().id)), red, 0.55F); draw.label(437, 658, "Avatar", 12, muted);
        draw.label(535, 658, "GOLD / Selected routes", 12, gold);
    } else draw.label(42, 658, conditions, 12, gold);
}
void Gui::battleScreen() {
    const auto& g = *game;
    const auto& p = g.currentPlayer();
    if (g.hasLivingBoss() || g.isLastStand()) { draw.fill({0, 0, Width, 80}, {46, 22, 35}); draw.line(0, 79, Width, 79, red); }
    draw.icon(46, 43, Icon::insignia, g.isLastStand() ? red : gold);
    draw.label(76, 19, "THE LAST WATCH", 23, text);
    draw.label(77, 51, (g.hasLivingBoss() ? "AVATAR MANIFESTED" : g.getBossDef().title) + " / " + g.getBossDef().name, 13, g.hasLivingBoss() ? red : muted);
    button("save", {950, 25, 96, 40}, "Save", [this] { save(true); });
    button("archive", {1058, 25, 104, 40}, "Archive", [this] { archivePath = savePath; openModal(Modal::archive); });
    button("codex", {1174, 25, 108, 40}, "Manual", [this] { openModal(Modal::help); });
    button("menu", {1294, 25, 122, 40}, "Menu", [this] { openModal(Modal::pause); });
    const std::array<std::string, 5> labels {{"ROUND " + num(g.getRound()), "DOOM CLOCK", "CHAOS", "OPEN RIFTS", g.isLastStand() ? "ENEMY LAST STAND" : g.hasLivingBoss() ? "AVATAR DEADLINE" : "NEXT PORTAL WAVE"}};
    const std::array<std::string, 5> values {{"P" + num(static_cast<int>(g.getPlayerIndex() + 1)) + " / " + p.getName(), num(g.getDoom()) + " / " + num(g.getBossDef().deadline), num(g.getChaos()) + " / 6", num(static_cast<int>(g.getPortals().size())) + " remaining", g.isLastStand() ? "10% / enemy activation" : g.hasLivingBoss() ? num(g.getFinalRounds()) + " rounds left" : num(g.livingDefenders()) + " total / round " + num(((g.getRound() - 1) / 5 + 1) * 5)}};
    for (int i = 0; i < 5; ++i) {
        SDL_Rect box {24 + i * 282, 89, 264, 64};
        draw.fill(box, (g.hasLivingBoss() || g.isLastStand()) ? Color{38, 23, 32} : panel); draw.label(box.x + 14, box.y + 9, labels[i], 11, muted);
        draw.label(box.x + 14, box.y + 29, draw.elide(values[i], box.w - 28, 19), 19, i == 2 && g.getChaos() > 2 ? red : i == 0 ? teal : gold);
        if (i == 1) draw.bar({box.x, box.y + 61, box.w, 3}, g.getDoom(), g.getBossDef().deadline, gold);
        if (i == 2) draw.bar({box.x, box.y + 61, box.w, 3}, g.getChaos(), 6, red);
    }
    phaseRail();
    board(); sectorPanel(); defenderPanel(); logPanel();
}
void Gui::sectorPanel() {
    const auto& g = *game;
    const auto& p = g.currentPlayer();
    const auto& sector = sectorAt(selectedSector);
    const bool here = selectedSector == p.getSector(), live = acceptsOrders();
    draw.fill({1004, 170, 412, 742}, panel); draw.outline({1004, 170, 412, 742}, border);
    draw.label(1024, 210, sector.name, 28, text);
    draw.icon(1366, 216, sectorIcon(selectedSector), sectorColor(selectedSector), 1.8F);
    draw.paragraph(1024, 254, g.isBridgeCollapsed() && selectedSector == 9 ? "Destroyed. Permanently inaccessible." : g.getClosure(selectedSector) ? "Entry closed temporarily. Occupants can leave." : descriptions[selectedSector - 1], 372, 15, muted, 3);
    std::string exits = "Exits -> ";
    for (int dest : g.getRoutes(sector.id)) exits += num(dest) + " ";
    draw.label(1024, 320, exits + (g.getClosure(selectedSector) ? " | CLOSED" : ""), 14, gold);
    button("move", {1024, 351, 372, 42}, here ? "Defender is here" : (g.freeMove(p.getSector(), selectedSector) ? "Free move to " : g.getTransportRounds() ? "Fly to " : "Move to ") + sector.name,
           [this] { perform({ActionType::move, selectedSector}); }, live && !here && g.canMove(p.getSector(), selectedSector), true,
           here ? "Already here." : g.freeMove(p.getSector(),selectedSector) ? "Free movement from Streets. Your turn continues." : g.getTransportRounds() ? "Transport crew: one action to reach any sector, bypassing roads and cordons." : "Movement costs one action and must follow an outgoing route from your current sector.");
    draw.label(1024, 411, "TARGETS", 11, muted);
    struct Contact { int id, hp, maxHP; std::string name, stats; Color color; Icon icon; };
    std::vector<Contact> contacts;
    const bool portalGuarded = (g.hasLivingBoss() && g.getBoss()->getSector() == selectedSector) ||
        std::any_of(g.getEnemies().begin(), g.getEnemies().end(), [this](const Enemy& e) { return e.getSector() == selectedSector; });
    for (const auto& r : g.getPortals()) if (r.sector == selectedSector)
        contacts.push_back({-1, r.health, 6, "Warp portal", g.portalShielded(selectedSector) ? "SHIELDED / Grenades bypass" : portalGuarded ? "GUARDED / Roll weapon hit chance" : "UNGUARDED / Automatic weapon hit", purple, Icon::portal});
    if (g.hasLivingBoss() && g.getBoss()->getSector() == selectedSector) {
        const auto& b = *g.getBoss();
        contacts.push_back({-2, b.getHealth(), b.getMaxHealth(), b.getName() + " / avatar", "ARM " + num(b.getArmor()) + "   WILL " + num(b.getWill()), purple, patronIcon(static_cast<int>(b.getKind()))});
    }
    for (const auto& e : g.getEnemies()) if (e.getSector() == selectedSector)
        contacts.push_back({e.getId(), e.getHealth(), e.getMaxHealth(), e.getName(), "ARM " + num(e.getArmor()) + "   WILL " + num(e.getWill()), red, enemyIcon(e.getKind())});
    const int pages = std::max(1, (static_cast<int>(contacts.size()) + 1) / 2);
    targetPage = std::clamp(targetPage, 0, pages - 1);
    for (int row = 0; row < 2; ++row) {
        const int index = targetPage * 2 + row;
        SDL_Rect box {1024, 434 + row * 61, 372, 54};
        if (index >= static_cast<int>(contacts.size())) {
            if (row == 0) draw.label(box.x + 10, box.y + 15, "No targets", 15, muted);
            continue;
        }
        const auto& c = contacts[index];
        draw.fill(box, c.id == selectedTarget ? Color{43, 39, 38} : raised);
        draw.outline(box, c.id == selectedTarget ? gold : border);
        draw.icon(box.x + 24, box.y + 25, c.icon, c.color, 1.05F);
        draw.label(box.x + 48, box.y + 5, draw.elide(c.name, 239, 16), 16, c.color);
        draw.label(box.x + 48, box.y + 28, draw.elide(c.stats, 239, 11), 11, muted);
        draw.label(box.x + 296, box.y + 8, num(c.hp) + " HP", 14, text);
        draw.bar({box.x + 296, box.y + 35, 64, 3}, c.hp, c.maxHP, c.color);
        buttons.push_back({"target-" + num(c.id), box, live, [this, id = c.id] { selectedTarget = id; }, "Select target; then choose Attack or Krak grenade."});
    }
    button("targets-prev", {1024, 562, 40, 27}, "<", [this] { --targetPage; }, targetPage > 0);
    draw.label(1082, 566, num(static_cast<int>(contacts.size())) + " contacts  /  " + num(targetPage + 1) + " of " + num(pages), 12, muted);
    button("targets-next", {1356, 562, 40, 27}, ">", [this] { ++targetPage; }, targetPage < pages - 1);
    std::string tactic;
    for (const auto& e : g.getEnemies()) if (e.getId() == selectedTarget) tactic = e.isSuppressed() ? "SUPPRESSED / Will skip its next enemy-phase action. " + e.definition().tactic : e.definition().tactic;
    if (selectedTarget == -1) tactic = g.portalShielded(selectedSector) ? "SHIELDED / Weapons cannot damage this portal until its Warden falls. A krak grenade bypasses the shield and collapses it." : "Unguarded: automatic hit. With other guards, roll your weapon hit chance. Grenades always close an unshielded rift.";
    if (selectedTarget == -2) tactic = "The avatar attacks local defenders or moves toward the nearest one. Grenades bypass its resistance.";
    if (g.isLastStand() && selectedTarget != 0) tactic = "LAST STAND / Holds position to summon a portal (10% per activation). Suppression cancels an activation; a new rift restores normal behavior.";
    draw.paragraph(1024, 598, tactic, 372, 13, selectedTarget == -1 && g.portalShielded(selectedSector) ? purple : muted, 3);
    const bool canAttack = here && live && selectedTarget != 0 && !(selectedTarget == -1 && g.portalShielded(selectedSector));
    button("attack", {1024, 664, 180, 43}, "Attack (" + num(selectedTarget == -1 && !portalGuarded ? 100 : g.playerHitChance(p, p.getWeapon())) + "%)", [this] {
        perform({selectedTarget == -1 ? ActionType::attackPortal : selectedTarget == -2 ? ActionType::attackBoss : ActionType::attackEnemy, selectedTarget});
    }, canAttack, true, "One action. Your equipped weapon determines damage and accuracy.");
    button("grenade", {1216, 664, 180, 43}, "Krak grenade (" + num(p.getSupply(Consumable::grenade)) + ")", [this] {
        perform({selectedTarget == -1 ? ActionType::grenadePortal : selectedTarget == -2 ? ActionType::grenadeBoss : ActionType::grenadeEnemy, selectedTarget});
    }, here && live && selectedTarget != 0 && p.getSupply(Consumable::grenade) > 0, false, "Always hits. Destroy a portal; deal " + num(5+g.playerTrueBonus(p)) + " true damage to enemies or bosses.");
    draw.line(1024, 726, 1396, 726, border);
    draw.label(1024, 739, draw.elide(p.getName(), 372, 11), 11, muted);
    button("search", {1024, 764, 116, 38}, "Search", [this] { perform({ActionType::search}); }, live, false, "Search your current sector, regardless of which sector you are inspecting.");
    button("rest", {1152, 764, 116, 38}, "Rest", [this] { perform({ActionType::rest}); }, live && (p.getHealth() < p.getMaxHealth() || p.getPoison() > 0), false, "Heal 5 HP. At Infirmary: fully heal and cure rot.");
    button("wait", {1280, 764, 116, 38}, "Wait", [this] { perform({ActionType::wait}); }, live, false, "Spend this defender's action without moving.");
    button("medkit", {1024, 814, 116, 38}, "Medkit (" + num(p.getSupply(Consumable::medkit)) + ")", [this] { perform({ActionType::medkit}); }, live && p.getSupply(Consumable::medkit) > 0 && (p.getHealth() < p.getMaxHealth() || p.getPoison() > 0), false, "Heal 8 HP and cure rot. Costs one action.");
    button("guard", {1152, 814, 116, 38}, "Guard", [this] { perform({ActionType::guard}); }, live, false, "Take cover: reduce all enemy-hit damage by 3 this round. Costs one action.");
    button("suppress", {1280, 814, 116, 38}, "Suppress", [this] { perform({ActionType::suppress, selectedTarget}); }, live && here && selectedTarget > 0 && weapon(p.getWeapon()).accuracy == Accuracy::precision, false, "Ranged hit: stop this enemy's next action instead of dealing damage.");
    if (p.getSector() == 8) button("market", {1024,867,372,32}, "Market offers", [this] { openModal(Modal::market); }, live);
    if (p.getSector() == 9) button("collapse-bridge", {1024,867,372,32}, "Collapse Bridge", [this] { openModal(Modal::bridge); }, live && p.getSupply(Consumable::grenade)>0);
}
void Gui::defenderPanel() {
    const auto& p = game->currentPlayer();
    draw.fill({24, 698, 340, 214}, panel); draw.outline({24, 698, 340, 214}, border);
    const bool lowHealth = p.getHealth() * 3 <= p.getMaxHealth();
    if (lowHealth && (animationClock / 400) % 2 == 0) {
        draw.outline({24, 698, 340, 214}, red); draw.outline({25, 699, 338, 212}, red);
    }
    draw.label(42, 713, lowHealth ? "ACTIVE DEFENDER / LOW HEALTH" : "ACTIVE DEFENDER", 11, lowHealth ? red : teal);
    draw.icon(60, 747, defenderIcon(p), lowHealth ? red : teal, 1.1F);
    draw.label(86, 734, draw.elide(p.getName(), 258, 22), 22, text);
    draw.bar({42, 772, 216, 6}, p.getHealth(), p.getMaxHealth(), teal);
    draw.label(270, 763, num(p.getHealth()) + " / " + num(p.getMaxHealth()), 14, text);
    draw.label(42, 793, "ARM " + num(p.getArmor()) + "   STR " + num(p.getStrength()) + "   PRE " + num(p.getPrecision()) + "   WILL " + num(p.getWill()), 13, muted);
    draw.label(42, 821, weapon(p.getWeapon()).name + (p.isGuarding() ? " / GUARD" : "") + (p.getPoison() ? "  / ROT " + num(p.getPoison()) : ""), 16, p.getPoison() ? red : gold);
    button("equipment", {42, 855, 155, 37}, "Equipment", [this] { openModal(Modal::inventory); }, true, false, "Inspect squad and equip weapons for free.");
    draw.label(214, 864, num(game->livingDefenders()) + " alive", 13, muted);
}
void Gui::logPanel() {
    draw.fill({380, 698, 604, 214}, panel); draw.outline({380, 698, 604, 214}, border);
    const bool lastStand = game->isLastStand();
    if (lastStand) {
        draw.fill({380, 698, 604, 214}, {39, 21, 31}); draw.outline({380, 698, 604, 214}, red);
        draw.label(398, 713, "ENEMY LAST STAND", 13, red);
        draw.paragraph(398, 746, "Until the god arrives, enemies hold position: each has a 10% summon chance per hostile activation. A new rift restores normal behavior. Eliminate all enemies and portals to win.", 562, 15, text, 4);
    } else draw.label(398, 713, "BATTLE LOG", 11, muted);
    button("journal", {846, 706, 120, 30}, "Full journal", [this] { openModal(Modal::journal); });
    std::vector<std::string> lines;
    for (const auto& entry : history) {
        const auto wrapped = draw.wrap(entry, 562, 14);
        lines.insert(lines.end(), wrapped.begin(), wrapped.end());
    }
    const int end = std::max(0, static_cast<int>(lines.size()) - logOffset);
    int y = lastStand ? 839 : 746;
    for (int i = std::max(0, end - (lastStand ? 2 : 6)); i < end; ++i) {
        const bool important = lines[i].find("WORLD EVENT") != std::string::npos || lines[i].find("PORTAL SEALED") != std::string::npos || lines[i].find("MANIFESTATION") != std::string::npos;
        draw.label(398, y, lines[i], 14, important ? gold : muted); y += 25;
    }
}
void Gui::archiveModal() {
    draw.label(280, 276, "Save / Load", 32, text);
    draw.paragraph(280, 329, "Choose a save file. Saving replaces that file; loading replaces the current session.", 870, 17, muted, 3);
    draw.label(280, 422, "FILE PATH", 12, muted);
    field("archive-path", {280, 452, 880, 52}, archivePath, 240, "campaign.sav");
    button("archive-save", {280, 640, 275, 48}, "Save to this file", [this] {
        try {
            saveCampaign(*game, archivePath); savePath = archivePath;
            openModal(Modal::none); toast("Campaign saved: " + savePath);
        } catch (const std::exception& e) { toast(e.what(), true); }
    }, game && !archivePath.empty(), true);
    button("archive-load", {578, 640, 275, 48}, "Load this file", [this] { load(archivePath); }, !archivePath.empty());
    button("archive-close", {876, 640, 284, 48}, "Cancel", [this] { openModal(Modal::none); });
}
void Gui::helpModal() {
    struct Section { const char* title; const char* body; };
    struct Page { const char* title; std::array<Section,3> sections; };
    static const std::array<Page,10> pages {{
        {"Objectives", {{{"WIN", "Close every portal AND defeat every enemy. Killing the Chaos god also wins immediately, even if other enemies or portals remain."},
            {"LOSE", "Chaos reaches 6, all defenders die permanently, or the god survives eight rounds after it appears. Killing the god takes priority over losing the squad."},
            {"BEFORE THE GOD ARRIVES", "With no portals left, surviving enemies stop and try to summon: 10% per enemy activation. A new portal or the god's arrival ends this last stand. It never returns after the god appears."}}}},
        {"Turn order", {{{"1 / DEFENDERS", "Each living defender spends one action, then control passes to the next. Inspecting, equipping, trading, and leaving Streets along a road are free."},
            {"2 / WORLD CARD", "After everyone acts, one shared card is drawn. Read it, then choose Resolve. Half of draws are quiet. The board waits while you read."},
            {"3 / HOSTILES", "Rot ticks first, enemies act individually, then the god acts. Doom advances by 1 + chaos. Every fifth round, portals spawn one enemy per living defender across the whole map."}}}},
        {"Travel", {{{"FOLLOW THE ARROWS", "Click a sector to inspect it; click Move to travel there. Only outgoing roads are allowed. Entering a sector does not guarantee a route back."},
            {"FREE EXITS & FLIGHTS", "Road movement out of Streets is free and your turn continues. Transport crew allows flights for the next defender phase. Other flights cost an action."},
            {"CHANGING ACCESS", "Road reversal changes one-way arrows. A cordon blocks entry but lets occupants leave; flights bypass cordons. A destroyed Bridge is permanently inaccessible, even to flights."}}}},
        {"Combat", {{{"HIT CHANCE", "Start at 80%. Lose 20 percentage points for each unmet stat requirement. Gain 5 per surplus aiming-stat point, up to 100%. Example: Strength 5 with a Strength 2 sword gives 95%."},
            {"DAMAGE & RESISTANCE", "Ranged aims with Precision, melee with Strength, psychic with Will. Heavy weapons also require Strength. Armor reduces physical damage; Will reduces psychic damage. True damage ignores both."},
            {"PORTALS & GRENADES", "Portals have 6 HP: unguarded means automatic hits; guards require a hit roll; Wardens block weapon damage. Krak grenades always destroy portals or deal 5 true damage to enemies. Sector bonuses apply."}}}},
        {"Orders & items", {{{"ATTACK, GUARD, SUPPRESS", "Attacks require a target in your sector. Guard reduces incoming physical or psychic hits by 3 this round. Suppress uses a ranged hit roll. On a hit, that enemy skips its next activation; no damage is dealt."},
            {"EQUIPMENT & TRADING", "Equipment shows your actual damage and hit chance. Equip for free. Trade gives one item to a living defender in your sector for free. Keep at least one weapon; duplicate weapons cannot transfer."},
            {"SEARCH & MARKET", "Search costs an action and finds loot. At Market, choose one of three offers for an action. Offers refresh each defender turn. Viewing or loading the game does not reroll them."}}}},
        {"Sector bonuses", {{{"DEFENSE", "Barracks: +2 armor for defenders.\nCathedral: +4 Will for defenders.\nInfirmary: Rest fully heals and cures rot."},
            {"ATTACKS", "Armory: +5 true damage on defender hits, including grenades.\nAlley: +20 percentage points to defender ranged accuracy.\nSlums: +2 physical damage to ALL melee attacks."},
            {"MOVEMENT & SUPPLIES", "Streets: outgoing road movement is free.\nMarket: choose one of three items for an action.\nBridge: can be destroyed using a krak grenade. See Bridge collapse."}}}},
        {"Recovery", {{{"REST & MEDKITS", "Rest spends your action to heal 5 HP. Resting at Infirmary fully heals you and cures rot. A medkit spends your action and one medkit to heal 8 HP and cure rot in any sector."},
            {"ORDINARY DEFEAT", "When an attack or rot reduces you to zero HP, you recover at Infirmary with full HP. Chaos rises by 1. You cannot be attacked again during that hostile phase. Only world events lower chaos."},
            {"PERMANENT DEATH", "Bridge collapse is the exception: defenders on the Bridge die permanently, including the one who destroys it. They never recover at Infirmary. The campaign ends if no defenders survive, unless the god dies too."}}}},
        {"Bridge collapse", {{{"HOW TO DESTROY THE BRIDGE", "Stand on Bridge with a krak grenade. Choose Collapse Bridge, then confirm. This spends one grenade and your action. Throwing a grenade at an enemy or portal does not destroy the Bridge."},
            {"ONLY IF THE GOD IS ON THE BRIDGE", "The god loses half its CURRENT HP, ignoring resistance, then moves to Slums. Damage rounds up: 56 HP becomes 28; 51 becomes 25. A god in another sector takes no damage and does not move."},
            {"EVERYONE ELSE & THE CROSSING", "All defenders on Bridge die permanently, including the acting defender. Regular enemies there die and portals close. Bridge stays inaccessible. Slums loses its normal road exit. A god that appears later starts in Slums."}}}},
        {"Enemies & the god", {{{"ENEMY ROLES", "Some enemies hold their sector; hunters follow roads toward defenders. Wardens shield portals. Psychic enemies test Will; plague enemies can inflict rot. Select a target to read its tactics."},
            {"THE GOD APPEARS", "When doom reaches its deadline, the god arrives and the UI turns red. It has at least 50 base HP plus 6 per starting defender. It attacks locally or moves toward the nearest defender."},
            {"NO MORE LAST STAND", "After the god appears, enemies fight and move normally even with no portals left. Neither enemies nor the god attempt last-stand summons. Suppression still skips an enemy's next activation."}}}},
        {"Saving & controls", {{{"SAVE YOUR CAMPAIGN", "Save or F5 writes your current campaign, including between phases. Archive chooses another file. Saving is manual unless you enable Autosave in Menu."},
            {"LEAVING THE GAME", "Menu offers exits to title or desktop, with or without saving. Leaving without saving discards progress since the last save. Closing the window opens Menu."},
            {"SHORTCUTS", "1-9: inspect sectors. I: Equipment. J: journal. H: manual.\nEscape: close a dialog or open Menu.\nMenu also controls animation speed."}}}}
    }};
    draw.label(280,249,"Field manual",30,text);
    for (int i=0;i<static_cast<int>(pages.size());++i) button("help-topic-"+num(i),{268,317+i*46,225,39},pages[i].title,[this,i] { helpPage=i; },true,helpPage==i);
    draw.line(512,317,512,745,border);
    draw.label(539,319,pages[helpPage].title,25,text);
    for (int i=0;i<3;++i) {
        const auto& section=pages[helpPage].sections[i]; const int y=372+i*126;
        draw.label(539,y,section.title,13,gold);
        draw.paragraph(539,y+25,section.body,620,18,text,4);
    }
    draw.label(699,793,num(helpPage+1)+" / "+num(static_cast<int>(pages.size())),15,muted);
    button("help-prev",{280,778,160,42},"Previous",[this] { --helpPage; },helpPage>0);
    button("help-next",{457,778,160,42},"Next",[this] { ++helpPage; },helpPage+1<static_cast<int>(pages.size()));
    button("help-close",{960,778,200,42},"Close",[this] { openModal(Modal::none); },true,true);
}
void Gui::inventoryModal() {
    const auto& p = game->currentPlayer();
    draw.label(280, 272, p.getName(), 32, text);
    draw.label(280, 324, "WEAPON", 12, muted); draw.label(645, 324, "DAMAGE / ACCURACY", 12, muted);
    int y = 353;
    inventoryPage = std::clamp(inventoryPage, 0, static_cast<int>((p.getWeapons().size() - 1) / 6));
    const int first = inventoryPage * 6, last = std::min(first + 6, static_cast<int>(p.getWeapons().size()));
    for (int i = first; i < last; ++i) {
        const WeaponId id = p.getWeapons()[i];
        const auto profile = weaponProfile(id, p);
        draw.fill({280, y, 880, 52}, raised);
        if (id == p.getWeapon() && equippedAt && animationClock - equippedAt < 1100)
            draw.outline({280, y, 880, 52}, (animationClock / 120) % 2 ? teal : gold);
        draw.label(296, y + 7, weapon(id).name, 18, id == p.getWeapon() ? gold : text);
        std::string requirement = "Requires " + weaponRequirements(id);
        if (profile.underRequirement) requirement += " / unmet requirement: -20% each";
        draw.label(296, y + 31, requirement, 12, muted);
        draw.label(645, y + 14, num(game->playerDamage(p,id)) + (profile.type == damageType::psychic ? " psychic" : " physical") + (game->playerTrueBonus(p) ? " +5 true" : "") + " / " + num(game->playerHitChance(p, id)) + "% hit", 16, text);
        button("equip-" + num(static_cast<int>(id)), {1000, y + 7, 145, 38}, id == p.getWeapon() ? "Equipped" : "Equip", [this, id] {
            if (acceptsOrders() && game->equip(id)) { equippedAt = animationClock; toast("Equipped " + weapon(id).name + ". No action spent."); if (autoSave) save(); }
        }, id != p.getWeapon() && acceptsOrders(), id != p.getWeapon());
        y += 61;
    }
    if (p.getWeapons().size() > 6) {
        button("inventory-prev", {280, 793, 48, 42}, "<", [this] { --inventoryPage; }, inventoryPage > 0);
        draw.label(346, 805, num(inventoryPage + 1) + " / " + num(static_cast<int>((p.getWeapons().size()+5)/6)), 16, muted);
        button("inventory-next", {420, 793, 48, 42}, ">", [this] { ++inventoryPage; }, last < static_cast<int>(p.getWeapons().size()));
    }
    const bool canTrade = std::any_of(game->getPlayers().begin(), game->getPlayers().end(), [&](const Player& other) {
        return &other != &p && other.getAlive() && other.getSector() == p.getSector();
    });
    draw.label(280, 744, "Medkits " + num(p.getSupply(Consumable::medkit)) + "    Krak grenades " + num(p.getSupply(Consumable::grenade)), 16, muted);
    button("inventory-trade", {600, 793, 180, 42}, "Trade", [this] { tradePage = 0; tradeRecipient = 0; openModal(Modal::trade); }, canTrade && acceptsOrders(), false, "Give items to a defender in this sector. Free action.");
    button("inventory-close", {960, 793, 200, 42}, "Close", [this] { openModal(Modal::none); }, true, true);
}
void Gui::tradeModal() {
    const auto& giver = game->currentPlayer();
    std::vector<std::size_t> recipients;
    for (std::size_t i = 0; i < game->getPlayers().size(); ++i)
        if (i != game->getPlayerIndex() && game->getPlayers()[i].getAlive() && game->getPlayers()[i].getSector() == giver.getSector()) recipients.push_back(i);
    draw.label(280, 254, "Trade", 32, text);
    button("trade-close", {960, 793, 200, 42}, "Back", [this] { openModal(Modal::inventory); });
    if (recipients.empty()) { draw.label(280, 330, "No other defender in this sector.", 18, muted); return; }
    tradeRecipient = std::clamp(tradeRecipient, 0, static_cast<int>(recipients.size()) - 1);
    const auto recipient = recipients[tradeRecipient]; const auto& receiver = game->getPlayers()[recipient];
    button("trade-recipient-prev", {280, 318, 48, 42}, "<", [this] { --tradeRecipient; }, tradeRecipient > 0);
    draw.label(350, 329, draw.elide("Give to " + receiver.getName(), 730, 20), 20, teal);
    button("trade-recipient-next", {1112, 318, 48, 42}, ">", [this] { ++tradeRecipient; }, tradeRecipient + 1 < static_cast<int>(recipients.size()));
    std::vector<Item> items;
    for (const auto id : giver.getWeapons()) items.push_back({true, static_cast<int>(id)});
    for (int id = 0; id < 2; ++id) if (giver.getSupply(static_cast<Consumable>(id))) items.push_back({false,id});
    tradePage = std::clamp(tradePage, 0, static_cast<int>((items.size()-1)/5));
    for (int i = tradePage*5; i < std::min(static_cast<int>(items.size()), tradePage*5+5); ++i) {
        const auto item = items[i]; const int y = 386 + (i%5)*65;
        bool enabled = acceptsOrders();
        if (item.isWeapon) enabled = enabled && giver.getWeapons().size() > 1 && std::find(receiver.getWeapons().begin(), receiver.getWeapons().end(), static_cast<WeaponId>(item.id)) == receiver.getWeapons().end();
        else enabled = enabled && receiver.getSupply(static_cast<Consumable>(item.id)) < 99;
        draw.fill({280,y,880,54}, raised);
        draw.label(298,y+17,itemName(item) + (item.isWeapon ? "" : " x" + num(giver.getSupply(static_cast<Consumable>(item.id)))),18,text);
        button("trade-give-" + num(i), {1000,y+7,145,40}, "Give", [this,recipient,item] {
            const auto result = game->trade(recipient,item); gatherLog(); toast(result.accepted ? "Item transferred." : result.error, !result.accepted);
            if (result.accepted && autoSave) save();
        }, enabled, true);
    }
    if (items.size() > 5) {
        button("trade-prev", {280,793,48,42}, "<", [this] { --tradePage; }, tradePage > 0);
        button("trade-next", {350,793,48,42}, ">", [this] { ++tradePage; }, (tradePage+1)*5 < static_cast<int>(items.size()));
    }
    draw.label(280,744,"Trading is free. Keep one weapon; duplicates cannot transfer.",15,muted);
}
void Gui::marketModal() {
    const auto& p = game->currentPlayer();
    draw.label(280,254,"Market",32,text);
    draw.label(280,309,"Choose one item. One action; new offers each defender turn.",17,muted);
    for (int i=0; i<3; ++i) {
        const auto item = game->getMarketOffers()[i];
        const int x = 280+i*298;
        const bool owned = item.isWeapon && std::find(p.getWeapons().begin(),p.getWeapons().end(),static_cast<WeaponId>(item.id)) != p.getWeapons().end();
        const bool full = !item.isWeapon && p.getSupply(static_cast<Consumable>(item.id)) >= 99;
        draw.fill({x,368,284,330},raised); draw.outline({x,368,284,330},border);
        draw.icon(x+142,426,item.isWeapon ? Icon::insignia : item.id == 1 ? Icon::grenade : Icon::supplies,gold,2);
        draw.paragraph(x+18,490,itemName(item),248,21,text,2);
        if (item.isWeapon) {
            const auto id = static_cast<WeaponId>(item.id);
            draw.label(x+18,558,num(weapon(id).damage)+" damage",17,gold);
            draw.paragraph(x+18,589,weaponRequirements(id),248,14,muted,2);
        }
        button("market-take-"+num(i),{x+18,639,248,42},owned ? "Owned" : full ? "Full" : "Take",[this,i] {
            openModal(Modal::none); perform({ActionType::market,i});
        },acceptsOrders() && p.getSector()==8 && !owned && !full,true);
    }
    button("market-close",{960,793,200,42},"Back",[this] { openModal(Modal::none); });
}
void Gui::bridgeModal() {
    draw.label(280,254,"Collapse Bridge?",32,red);
    draw.paragraph(280,321,"PERMANENT DEATH: every defender on the Bridge will be lost. No Infirmary recovery. The crossing can never be used again.",870,21,text,3);
    std::string names;
    for (const auto& p : game->getPlayers()) if (p.getAlive() && p.getSector()==9) {
        if (!names.empty()) names += ", ";
        names += p.getName();
    }
    draw.paragraph(280,426,"Defenders lost: " + names,870,19,red,3);
    const bool bossHere = game->hasLivingBoss() && game->getBoss()->getSector()==9;
    draw.paragraph(280,531,bossHere ? game->getBoss()->getName()+" is on Bridge: "+num(game->getBoss()->getHealth())+" HP becomes "+num(game->getBoss()->getHealth()/2)+" HP, then it moves to Slums. Regular enemies and portals here are destroyed." : "Regular enemies and portals on Bridge are destroyed. The god is not on Bridge, so it takes no damage and is not moved.",870,19,gold,3);
    draw.label(280,643,"Costs 1 krak grenade. Losing the entire squad ends the campaign.",17,muted);
    button("bridge-cancel",{280,756,410,56},"Cancel",[this] { openModal(Modal::none); });
    button("bridge-confirm",{750,756,410,56},"Destroy Bridge / sacrifice defenders",[this] {
        openModal(Modal::none); perform({ActionType::collapseBridge});
    },acceptsOrders() && game->currentPlayer().getSector()==9 && game->currentPlayer().getSupply(Consumable::grenade)>0,true);
}
void Gui::customModal() {
    auto& r = recruits[customRecruit];
    int cost = 0; for (int stat : r.stats) cost += Game::statCost(stat);
    draw.label(280, 274, "Build your defender", 32, text);
    draw.label(280, 328, "POINTS SPENT  " + num(cost) + " / 50", 20, cost > 50 ? red : teal);
    draw.paragraph(280, 368, "HP = 1 + 3 x Vitality. Armor cap: 4; other stats: 9. A bolter pistol is included.", 880, 17, muted, 3);
    const std::array<std::string, 5> labels {{"Vitality", "Armor", "Strength", "Precision", "Will"}};
    for (int i = 0; i < 5; ++i) {
        int y = 453 + i * 56;
        draw.label(300, y + 9, labels[i], 19, text);
        draw.label(600, y + 11, "Cost " + num(Game::statCost(r.stats[i])), 16, muted);
        button("stat-minus-" + num(i), {840, y, 48, 38}, "-", [this, i] { --recruits[customRecruit].stats[i]; }, r.stats[i] > (i == 0 ? 1 : 0));
        draw.label(922, y + 4, num(r.stats[i]), 23, gold);
        const int nextCost = cost - Game::statCost(r.stats[i]) + Game::statCost(r.stats[i] + 1);
        button("stat-plus-" + num(i), {980, y, 48, 38}, "+", [this, i] { ++recruits[customRecruit].stats[i]; },
               r.stats[i] < (i == 1 ? 4 : 9) && nextCost <= 50);
    }
    draw.label(300, 738, "STARTING WEAPON", 13, muted);
    button("weapon-prev", {600, 728, 42, 36}, "<", [this] { auto& choice = recruits[customRecruit].startingWeapon; choice = (choice + 3) % 4; });
    draw.label(661, 730, weapon(characters()[r.startingWeapon].weapon).name, 17, gold);
    draw.label(661, 751, "Requires " + weaponRequirements(characters()[r.startingWeapon].weapon), 12, muted);
    button("weapon-next", {980, 728, 42, 36}, ">", [this] { auto& choice = recruits[customRecruit].startingWeapon; choice = (choice + 1) % 4; });
    button("custom-close", {960, 778, 200, 42}, "Confirm build", [this] { openModal(Modal::none); }, cost <= 50, true);
    button("custom-reset", {280, 778, 200, 42}, "Reset to 4 / 4 / 4 / 4 / 4", [this] { recruits[customRecruit].stats = {{4, 4, 4, 4, 4}}; });
}
void Gui::journalModal() {
    draw.label(280, 274, "Battle journal", 32, text);
    std::vector<std::string> lines;
    for (const auto& entry : history) {
        auto wrapped = draw.wrap(entry, 880, 16);
        lines.insert(lines.end(), wrapped.begin(), wrapped.end());
    }
    logOffset = std::clamp(logOffset, 0, std::max(0, static_cast<int>(lines.size()) - 16));
    const int end = static_cast<int>(lines.size()) - logOffset;
    for (int i = std::max(0, end - 16), y = 334; i < end; ++i, y += 25) draw.label(280, y, lines[i], 16, muted);
    button("log-older", {280, 778, 180, 42}, "Older", [this] { logOffset += 12; }, end > 16);
    button("log-newer", {478, 778, 180, 42}, "Newer", [this] { logOffset = std::max(0, logOffset - 12); }, logOffset > 0);
    button("log-close", {960, 778, 200, 42}, "Close journal", [this] { openModal(Modal::none); }, true, true);
}
void Gui::leaveWithoutSaving(bool toTitle) {
    game.reset(); beforeAnimation.reset(); history.clear(); recentLog.clear();
    presentation = Presentation::idle; openModal(Modal::none); notice.clear();
    if (toTitle) screen = Screen::title;
    else running = false;
}
void Gui::pauseModal() {
    draw.label(280, 280, "Paused", 32, text);
    draw.paragraph(280, 337, "Unsaved progress is lost when you leave without saving.", 870, 17, muted, 3);
    draw.label(280, 416, "Active slot: " + draw.elide(savePath, 690, 16), 16, gold);
    button("resume", {280, 458, 880, 46}, "Return to the watch", [this] { openModal(Modal::none); }, true, true);
    button("main-menu", {280, 522, 430, 44}, "Save and return to title", [this] { if (save()) leaveWithoutSaving(true); });
    button("title-unsaved", {730, 522, 430, 44}, "Return to title WITHOUT saving", [this] { leaveWithoutSaving(true); });
    button("quit", {280, 580, 430, 44}, "Save and quit game", [this] { quit(); });
    button("quit-unsaved", {730, 580, 430, 44}, "Quit game WITHOUT saving", [this] { leaveWithoutSaving(false); });
    button("pause-archive", {280, 642, 880, 42}, "Save / load a different campaign file", [this] { archivePath = savePath; openModal(Modal::archive); });
    button("autosave", {280, 706, 430, 42}, autoSave ? "Autosave: ON" : "Autosave: OFF (manual saves)", [this] { autoSave = !autoSave; }, true, autoSave);
    button("animation-speed", {730, 706, 430, 42}, fastAnimations ? "Animation speed: fast" : "Animation speed: normal", [this] { fastAnimations = !fastAnimations; });
    draw.paragraph(280, 772, autoSave ? "Autosaving cannot be undone by quitting without saving. It only discards changes after the last successful autosave." : "Manual saves are the default. F5 saves at any point. Closing the window opens this menu instead of saving automatically.", 880, 14, muted, 2);
}
void Gui::endOverlay() {
    buttons.clear();
    draw.fill({0, 0, Width, Height}, {0, 0, 0, 205});
    draw.fill({240, 220, 960, 570}, panel); draw.outline({240, 220, 960, 570}, gold);
    const bool won = game->getOutcome() != Outcome::defeat;
    draw.icon(720, 292, won ? Icon::insignia : Icon::skull, won ? gold : red, 2);
    const std::string title = won ? "THE CITY ENDURES" : "THE WATCH HAS FALLEN";
    draw.label(720 - draw.measure(title, 39) / 2, 352, title, 39, won ? gold : red);
    draw.paragraph(290, 423, game->getEnding(), 860, 21, text, 4);
    draw.label(290, 548, "PORTALS SEALED  " + num(game->getClosed()) + "       ENEMIES DEFEATED  " + num(game->getKills()) + "       ROUND  " + num(game->getRound()), 15, muted);
    button("ending-new", {290, 637, 410, 56}, "Another watch", [this] { screen = Screen::setup; clearFocus(); }, true, true);
    button("ending-journal", {730, 637, 410, 56}, "Read the battle journal", [this] { openModal(Modal::journal); });
    button("ending-title", {290, 711, 410, 42}, "Title without saving", [this] { leaveWithoutSaving(true); });
    button("ending-quit", {730, 711, 410, 42}, "Quit without saving", [this] { leaveWithoutSaving(false); });
}
void Gui::modalScreen() {
    buttons.clear();
    draw.fill({0, 0, Width, Height}, {0, 0, 0, 205});
    draw.fill({244, 216, 952, 640}, panel); draw.outline({244, 216, 952, 640}, border);
    draw.fill({244, 216, 3, 640}, gold);
    switch (modal) {
        case Modal::archive: archiveModal(); break;
        case Modal::help: helpModal(); break;
        case Modal::inventory: inventoryModal(); break;
        case Modal::custom: customModal(); break;
        case Modal::journal: journalModal(); break;
        case Modal::pause: pauseModal(); break;
        case Modal::trade: tradeModal(); break;
        case Modal::market: marketModal(); break;
        case Modal::bridge: bridgeModal(); break;
        case Modal::none: break;
    }
}
void Gui::render(bool present) {
    buttons.clear(); draw.backdrop();
    if (screen == Screen::title) titleScreen();
    if (screen == Screen::setup) setupScreen();
    if (screen == Screen::battle && game) battleScreen();
    if (screen == Screen::battle && game) {
        presentationOverlay();
        if (game->getOutcome() != Outcome::ongoing && presentation == Presentation::idle) endOverlay();
    }
    if (modal != Modal::none) modalScreen();
    if (tutorialStep >= 0) tutorialOverlay();
    draw.fill({0, 924, Width, 36}, {9, 14, 17});
    std::string footer = notice.empty() ? (tutorialStep >= 0 ? "TRAINING / Use the lesson panel to practice. Previous / Next changes lessons. Escape returns to the menu." :
        "H: manual   I: equipment   F5: save") : notice;
    if (notice.empty()) for (const auto& b : buttons) if (inside(b.rect, mouseX, mouseY) && !b.tip.empty()) { footer = b.tip; break; }
    draw.label(24, 931, draw.elide(footer, 1390, 14), 14, noticeError && !notice.empty() ? red : muted);
    if (present) SDL_RenderPresent(renderer);
    ++frames;
}
void Gui::handle(const SDL_Event& event) {
    if (tutorialStep >= 0 && (event.type == SDL_QUIT ||
        (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))) { exitTutorial(); return; }
    if (event.type == SDL_QUIT) { if (game) openModal(Modal::pause); else running = false; return; }
    if (event.type == SDL_MOUSEMOTION) { mouseX = event.motion.x; mouseY = event.motion.y; }
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        mouseX = event.button.x; mouseY = event.button.y;
        // Copy the callback before it changes screens, state, or the button list.
        for (auto it = buttons.rbegin(); it != buttons.rend(); ++it) if (inside(it->rect, mouseX, mouseY)) {
            if (it->enabled) { auto action = it->action; action(); buttons.clear(); }
            return;
        }
        clearFocus();
    }
    if (event.type == SDL_MOUSEWHEEL) {
        if (modal == Modal::journal || (modal == Modal::none && screen == Screen::battle && mouseX >= 380 && mouseX <= 984 && mouseY >= 698 && mouseY <= 912)) {
            const int delta = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -event.wheel.y : event.wheel.y;
            logOffset = std::clamp(logOffset + delta * 3, 0, static_cast<int>(history.size()));
        }
    }
    if (event.type == SDL_TEXTINPUT && focused) {
        const std::string incoming(event.text.text);
        if (focused->size() + incoming.size() <= static_cast<std::size_t>(focusLimit) &&
            std::none_of(incoming.begin(), incoming.end(), [](unsigned char c) { return c < 32 || c == 127; }))
            *focused += incoming;
    }
    if (event.type != SDL_KEYDOWN) return;
    const auto key = event.key.keysym.sym;
    if (event.key.repeat && !(focused && key == SDLK_BACKSPACE)) return;
    if (key == SDLK_ESCAPE) {
        clearFocus();
        if (modal != Modal::none) openModal(Modal::none);
        else if (screen == Screen::battle && game->getOutcome() == Outcome::ongoing) openModal(Modal::pause);
        else if (screen == Screen::setup) screen = Screen::title;
        return;
    }
    if (focused) {
        if (key == SDLK_BACKSPACE && !focused->empty()) {
            std::size_t start = focused->size() - 1;
            while (start > 0 && (static_cast<unsigned char>((*focused)[start]) & 0xC0) == 0x80) --start;
            focused->erase(start);
        }
        if (key == SDLK_a && (event.key.keysym.mod & KMOD_CTRL)) focused->clear();
        if (key == SDLK_v && (event.key.keysym.mod & KMOD_CTRL)) {
            char* clipboard = SDL_GetClipboardText();
            if (clipboard) {
                const std::string incoming(clipboard); SDL_free(clipboard);
                if (focused->size() + incoming.size() <= static_cast<std::size_t>(focusLimit) &&
                    std::none_of(incoming.begin(), incoming.end(), [](unsigned char c) { return c < 32 || c == 127; }))
                    *focused += incoming;
            }
        }
        if (key == SDLK_RETURN || key == SDLK_TAB) clearFocus();
        return;
    }
    if (modal != Modal::none) return;
    if (tutorialStep >= 0) return;
    if (key == SDLK_h) { openModal(Modal::help); return; }
    if (screen != Screen::battle || !game || game->getOutcome() != Outcome::ongoing) return;
    if (key == SDLK_F5) { save(true); return; }
    if (presentation != Presentation::idle) return;
    if (key >= SDLK_1 && key <= SDLK_9) selectSector(static_cast<int>(key - SDLK_1 + 1));
    if (key == SDLK_i) openModal(Modal::inventory);
    if (key == SDLK_j) openModal(Modal::journal);
}
void Gui::run() {
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) handle(event);
        updatePresentation(SDL_GetTicks());
        render(); SDL_Delay(16);
    }
}
void Gui::capture(const std::string& path) {
    int w = 0, h = 0; SDL_GetRendererOutputSize(renderer, &w, &h);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surface) throw std::runtime_error(SDL_GetError());
    const bool read = SDL_RenderReadPixels(renderer, nullptr, surface->format->format, surface->pixels, surface->pitch) == 0;
    const bool saved = read && SDL_SaveBMP(surface, path.c_str()) == 0;
    SDL_FreeSurface(surface);
    if (!saved) throw std::runtime_error("Could not capture GUI: " + std::string(SDL_GetError()));
}
void Gui::expect(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("GUI smoke test: " + message);
}
void Gui::clickTest(const std::string& id) {
    render();
    const auto found = std::find_if(buttons.begin(), buttons.end(), [&](const Button& b) { return b.id == id; });
    expect(found != buttons.end(), "missing button " + id);
    const auto box = found->rect;
    SDL_Event e {}; e.type = SDL_MOUSEBUTTONUP; e.button.button = SDL_BUTTON_LEFT;
    e.button.x = box.x + box.w / 2; e.button.y = box.y + box.h / 2;
    handle(e);
}
void Gui::smokeTest(const std::string& screenshotPath) {
    auto snapshot = [](const Game& g) { std::ostringstream out; g.save(out); return out.str(); };
    auto fileContents = [](const std::string& path) { std::ifstream in(path); return std::string(std::istreambuf_iterator<char>(in), {}); };
    savePath = "gui-smoke.sav";
    if (!screenshotPath.empty()) {
        std::vector<std::pair<Icon,std::string>> symbols;
        for (const auto& s : sectors()) symbols.push_back({sectorIcon(s.id), s.name});
        for (std::size_t i = 0; i < characters().size(); ++i) symbols.push_back({static_cast<Icon>(static_cast<int>(Icon::marine) + i), characters()[i].name});
        symbols.push_back({Icon::defender, "Custom defender"}); symbols.push_back({Icon::portal, "Warp portal"});
        for (std::size_t i = 0; i < enemyDefinitions().size(); ++i) symbols.push_back({enemyIcon(static_cast<int>(i)), enemyDefinitions()[i].name});
        for (const auto& b : bosses()) symbols.push_back({patronIcon(static_cast<int>(b.id)), b.name});
        for (const auto& entry : std::vector<std::pair<Icon,std::string>>{{Icon::insignia,"Watch insignia"},{Icon::supplies,"Supplies"},
            {Icon::search,"Search"},{Icon::shield,"Guard"},{Icon::clock,"Hold position"},{Icon::grenade,"Krak grenade"},{Icon::skull,"Defeat"}}) symbols.push_back(entry);
        draw.fill({0,0,Width,Height}, background);
        draw.label(32, 23, "THE LAST WATCH / COMPLETE ICON SET", 22, gold);
        for (std::size_t i = 0; i < symbols.size(); ++i) {
            const int x = 28 + static_cast<int>(i%7)*200, y = 78 + static_cast<int>(i/7)*142;
            draw.fill({x,y,188,128}, panel); draw.outline({x,y,188,128}, border);
            const Color ink = i < 9 ? sectorColor(static_cast<int>(i)+1) : i < 14 ? teal : i == 14 ? purple : i < 34 ? red : gold;
            draw.icon(x+94,y+48,symbols[i].first,ink,2.0F);
            draw.label(x+94-draw.measure(symbols[i].second,13)/2,y+99,symbols[i].second,13,text);
        }
        capture(screenshotPath + ".icons.bmp");
    }
    if (!screenshotPath.empty()) screenshot(screenshotPath + ".title.bmp");
    for (int count=1;count<=4;++count) {
        clickTest("quick-count-"+num(count)); clickTest("quick");
        expect(static_cast<int>(game->getPlayers().size())==count && static_cast<int>(game->getEnemies().size())==2*count,"quick player selector scales squad and initial enemies");
        std::set<WeaponId> presets;
        for (const auto& p:game->getPlayers()) presets.insert(p.getWeapon());
        expect(static_cast<int>(presets.size())==count,"quick squad uses distinct presets");
        if (count==4 && !screenshotPath.empty()) screenshot(screenshotPath + ".active-defender.bmp");
        clickTest("menu"); clickTest("title-unsaved");
    }
    clickTest("quick-count-1");
    clickTest("tutorial");
    for (int step = 0; step < 12; ++step) {
        expect(tutorialStep == step, "tutorial advances one lesson at a time");
        if (step >= 1 && step <= 9) {
            clickTest("tutorial-practice");
            if (step == 3) {
                clickTest("inventory-next"); clickTest("equip-9");
                expect(game->currentPlayer().getWeapon() == WeaponId::forceHalberd, "expanded inventory pages equip new weapons");
                if (!screenshotPath.empty()) screenshot(screenshotPath + ".weapons.bmp");
                clickTest("inventory-close");
            } else {
                if (presentation == Presentation::action) {
                    updatePresentation(animationClock + static_cast<Uint32>(550 * animationScale()));
                    if (!screenshotPath.empty()) screenshot(screenshotPath + ".lesson-" + num(step) + ".bmp");
                }
                settleTest(true, step == 9 && !screenshotPath.empty() ? screenshotPath + ".enemy-combat.bmp" : "");
            }
        }
        if (step == 0 && !screenshotPath.empty()) screenshot(screenshotPath + ".tutorial.bmp");
        clickTest("tutorial-next");
    }
    expect(tutorialStep == -1 && screen == Screen::title && !game && !std::filesystem::exists(savePath), "tutorial completes without creating a campaign save");
    inventoryPage = 0;
    clickTest("new"); clickTest("boss-0");
    clickTest("squad-plus"); clickTest("squad-plus"); clickTest("squad-plus");
    expect(squadSize == 4, "four-defender setup");
    if (!screenshotPath.empty()) screenshot(screenshotPath + ".setup.bmp");
    clickTest("class-prev-0"); expect(recruits[0].preset == 4, "previous arrow wraps to custom");
    clickTest("class-next-0"); expect(recruits[0].preset == 0, "next arrow returns to Marine");
    for (int i = 0; i < 4; ++i) clickTest("class-0");
    clickTest("custom-0"); clickTest("weapon-next"); expect(recruits[0].startingWeapon == 2, "custom selects force staff"); clickTest("weapon-prev"); expect(recruits[0].startingWeapon == 1, "custom weapon cycles backward"); clickTest("stat-plus-2");
    expect(recruits[0].stats[2] == 4, "increment is blocked at the point-buy limit");
    clickTest("stat-minus-2"); clickTest("stat-plus-3");
    expect(recruits[0].stats[3] == 4, "increment is blocked when its five-point cost exceeds four remaining points");
    clickTest("stat-plus-2");
    expect(recruits[0].stats[2] == 4, "affordable increment re-enables and can spend exactly the remaining points");
    clickTest("custom-close"); expect(modal == Modal::none, "valid fifty-point build confirms"); clickTest("class-0");
    clickTest("squad-minus"); clickTest("squad-minus");
    clickTest("name-0");
    SDL_Event key {}; key.type = SDL_KEYDOWN; key.key.keysym.sym = SDLK_a; key.key.keysym.mod = KMOD_CTRL; handle(key);
    SDL_Event typing {}; typing.type = SDL_TEXTINPUT; SDL_strlcpy(typing.text.text, "Test Commander", sizeof(typing.text.text)); handle(typing);
    SDL_strlcpy(typing.text.text, "XY\xC3\xA9", sizeof(typing.text.text)); handle(typing);
    key.key.keysym.sym = SDLK_BACKSPACE; key.key.keysym.mod = KMOD_NONE;
    handle(key); key.key.repeat = 1; handle(key); handle(key); key.key.repeat = 0;
    expect(recruits[0].name == "Test Commander", "holding Backspace repeats and deletes whole UTF-8 characters");
    clickTest("deploy");
    expect(game && game->getPlayers().size() == 2 && game->currentPlayer().getName() == "Test Commander", "deployment");
    expect(presentation == Presentation::handoff && !autoSave && !std::filesystem::exists(savePath), "manual saves default; deployment has a brief automatic phase banner");
    settleTest(); clickTest("save");
    expect(std::filesystem::exists(savePath), "explicit save creates file");
    { std::ofstream blocked(savePath + ".tmp"); blocked << "preserve"; }
    SDL_Event close {}; close.type = SDL_QUIT; handle(close);
    expect(running && modal == Modal::pause, "window close offers exit choices");
    clickTest("quit"); expect(running && noticeError && modal == Modal::pause, "failed save does not lose active game");
    std::filesystem::remove(savePath + ".tmp"); clickTest("resume");
    clickTest("sector-9"); clickTest("move");
    expect(game->currentPlayer().getSector() == 1 && presentation == Presentation::idle, "unreachable move disabled");
    clickTest("sector-2"); clickTest("move");
    expect(presentation == Presentation::action && game->getRound() == 1, "player action has its own animation");
    settleTest(false);
    expect(presentation == Presentation::idle && game->getPlayerIndex() == 1, "defender banner dismisses automatically");
    settleTest(); clickTest("wait");
    expect(game->getPhase() == TurnPhase::worldDraw && game->getRound() == 1 && game->getActiveCard() == -1, "world does not resolve inside player input");
    updatePresentation(animationClock + 10000);
    expect(presentation == Presentation::drawing, "distinct card draw stage");
    updatePresentation(animationClock + 10000);
    expect(presentation == Presentation::flipping && game->getPhase() == TurnPhase::worldResolve, "distinct card flip stage");
    updatePresentation(animationClock + static_cast<Uint32>(550 * animationScale()));
    if (!screenshotPath.empty()) screenshot(screenshotPath + ".flip.bmp");
    updatePresentation(animationClock + 10000);
    expect(presentation == Presentation::card, "revealed card waits for reader");
    const auto held = snapshot(*game);
    updatePresentation(animationClock + 30000);
    expect(snapshot(*game) == held, "reading a card never applies effects or advances time");
    if (!screenshotPath.empty()) screenshot(screenshotPath + ".card.bmp");
    const int card = game->getActiveCard();
    key.key.keysym.sym = SDLK_F5; key.key.keysym.mod = KMOD_NONE; handle(key);
    expect(fileContents(savePath) == held, "F5 saves the held card during presentation");
    clickTest("archive"); clickTest("archive-load");
    expect(game->getActiveCard() == card && game->getPhase() == TurnPhase::worldResolve && presentation == Presentation::card, "load resumes the same unplayed card");
    clickTest("card-resolve");
    expect(game->getPhase() == TurnPhase::enemies && presentation == Presentation::worldEffect && game->getRound() == 1, "world resolves before hostiles and upkeep");
    settleTest(false);
    expect(game->getRound() == 2 && presentation == Presentation::idle, "round processing hands back control automatically");
    settleTest();
    clickTest("equipment"); clickTest("equip-0");
    expect(game->currentPlayer().getWeapon() == WeaponId::pistol && game->getPlayerIndex() == 0, "equipment is a free action");
    clickTest("inventory-close"); clickTest("archive"); clickTest("archive-path");
    key.key.keysym.sym = SDLK_a; key.key.keysym.mod = KMOD_CTRL; handle(key);
    SDL_strlcpy(typing.text.text, "gui named patrol.sav", sizeof(typing.text.text)); handle(typing);
    clickTest("archive-save");
    const auto savedBytes = fileContents(savePath);
    const auto campaignBeforeTutorial = snapshot(*game);
    startTutorial(); clickTest("tutorial-next"); clickTest("tutorial-practice");
    key.key.keysym.sym = SDLK_ESCAPE; key.key.keysym.mod = KMOD_NONE; handle(key);
    expect(screen == Screen::title && tutorialStep == -1 && snapshot(*game) == campaignBeforeTutorial && fileContents(savePath) == savedBytes,
        "Escape exits a running tutorial animation and preserves the campaign and save");
    clickTest("continue"); settleTest();
    clickTest("guard"); settleTest();
    expect(fileContents(savePath) == savedBytes, "orders do not overwrite manual save");
    clickTest("menu");
    if (!screenshotPath.empty()) screenshot(screenshotPath + ".pause.bmp");
    clickTest("title-unsaved");
    expect(!game && screen == Screen::title && fileContents(savePath) == savedBytes, "return to title discards session without saving");
    clickTest("continue"); settleTest();
    expect(game && game->getRound() == 2 && game->getPlayerIndex() == 0, "Continue resumes saved state rather than discarded session");
    { std::ofstream invalid("bad-gui.sav"); invalid << "not a valid campaign"; }
    clickTest("archive"); clickTest("archive-path"); key.key.keysym.sym = SDLK_a; key.key.keysym.mod = KMOD_CTRL; handle(key);
    SDL_strlcpy(typing.text.text, "bad-gui.sav", sizeof(typing.text.text)); handle(typing); clickTest("archive-load");
    expect(modal == Modal::archive && game->getRound() == 2, "failed load preserves campaign"); clickTest("archive-close");
    clickTest("codex"); if (!screenshotPath.empty()) screenshot(screenshotPath + ".manual.bmp");
    for (int page = 1; page < 10; ++page) { clickTest("help-next"); expect(helpPage == page, "all manual pages reachable"); }
    clickTest("help-topic-7"); expect(helpPage==7,"bridge rules have a dedicated topic"); notice.clear();
    if (!screenshotPath.empty()) screenshot(screenshotPath + ".manual-bridge.bmp");
    clickTest("help-topic-3"); expect(helpPage==3,"manual topics jump directly to combat");
    if (!screenshotPath.empty()) screenshot(screenshotPath + ".manual-combat.bmp");
    clickTest("help-close"); clickTest("journal"); clickTest("log-close");
    // All tactical commands exist; enabling autosave is an explicit preference.
    clickTest("menu"); clickTest("autosave"); clickTest("resume"); clickTest("guard"); settleTest();
    expect(autoSave && loadCampaign(savePath).getPlayerIndex() == game->getPlayerIndex(), "optional autosave persists accepted order");
    clickTest("menu"); clickTest("autosave"); clickTest("resume");

    std::set<int> quickClasses;
    for (int i = 0; i < 4; ++i) { startQuick(); quickClasses.insert(lastQuickPreset); }
    expect(quickClasses.size() == 4, "GUI quick deployment cycles all four presets");
    fixedSeed = true; initialSeed = 42; lastQuickPreset = 3; startQuick();
    if (!screenshotPath.empty()) screenshot(screenshotPath);
    // Reach an initial portal using the actual routes even if a card reverses them.
    int portalSector = game->getPortals()[0].sector;
    for (int steps = 0; steps < 15 && game->currentPlayer().getSector() != portalSector; ++steps) {
        int dest = game->routeStep(game->currentPlayer().getSector(), portalSector);
        clickTest("sector-" + num(dest)); clickTest("move"); settleTest(true, screenshotPath.empty() ? "" : screenshotPath + ".enemy-combat.bmp");
    }
    expect(game->currentPlayer().getSector() == portalSector, "clickable movement reaches portal");
    auto selectTarget = [&](int id) {
        for (int page = 0; page < 40; ++page) {
            render();
            if (std::any_of(buttons.begin(), buttons.end(), [&](const Button& b) { return b.id == "target-" + num(id); })) { clickTest("target-" + num(id)); return; }
            clickTest("targets-next");
        }
        throw std::runtime_error("Could not page to test target");
    };
    selectTarget(-1);
    if (game->portalShielded(portalSector)) {
        const auto before = snapshot(*game); clickTest("attack");
        expect(snapshot(*game) == before, "Warden disables weapon attacks on its portal");
    }
    clickTest("grenade");
    expect(presentation == Presentation::action && !game->getCombatEvents().empty(), "grenade produces a combat animation");
    updatePresentation(animationClock + static_cast<Uint32>(780 * animationScale()));
    if (!screenshotPath.empty()) screenshot(screenshotPath + ".combat.bmp");
    expect(std::none_of(game->getPortals().begin(), game->getPortals().end(), [portalSector](const Portal& p) { return p.sector == portalSector; }), "grenade collapses portal even through shield");
    settleTest();
    // Reach the last stand through public gameplay with a cooperative squad.
    Game patrol(BossId::khorne, 42);
    for (int i = 0; i < 4; ++i) patrol.addPreset(1, "Rift hunter " + num(i + 1));
    patrol.start(); game = std::move(patrol); history.clear(); gatherLog(); syncSelection(); beginStage(Presentation::idle);
    for (int order = 0; order < 60 && game->getOutcome() == Outcome::ongoing && !game->isLastStand(); ++order) {
        const int here = game->currentPlayer().getSector();
        int destination = here, shortest = 100;
        for (const auto& rift : game->getPortals()) {
            int distance = 0, cursor = here;
            while (cursor != rift.sector && distance < 10) { cursor = game->routeStep(cursor, rift.sector); ++distance; }
            if (distance < shortest) { shortest = distance; destination = rift.sector; }
        }
        if (destination != here) perform({ActionType::move, game->routeStep(here, destination)});
        else if (game->currentPlayer().getSupply(Consumable::grenade)) perform({ActionType::grenadePortal});
        else if (!game->portalShielded(here)) perform({ActionType::attackPortal});
        else {
            for (const auto& enemy : game->getEnemies()) if (enemy.getSector() == here && enemy.definition().role == EnemyRole::warden) {
                const int id = enemy.getId(); perform({ActionType::attackEnemy, id}); break;
            }
        }
        if (!game->isLastStand()) settleTest();
    }
    expect(game->isLastStand() && game->getOutcome() == Outcome::ongoing, "GUI reaches last stand without declaring victory");
    if (game->getPhase() == TurnPhase::players) settleTest();
    render();
    expect(std::none_of(buttons.begin(), buttons.end(), [](const Button& b) { return b.id == "seal" || b.id == "pray" || b.id == "ward" || b.id == "begin-turn"; }), "retired actions and blocking handoff are absent");
    if (!screenshotPath.empty()) screenshot(screenshotPath + ".last-stand.bmp");
    save(); load(savePath);
    expect(game->isLastStand() && game->getOutcome() == Outcome::ongoing, "GUI load retains last-stand state");
    // Trade through actual inventory buttons; inspect does not spend an action.
    game = Game(BossId::khorne,42); game->addPreset(0,"Trader"); game->addPreset(1,"Recipient"); game->start();
    history.clear(); gatherLog(); syncSelection(); beginStage(Presentation::idle);
    clickTest("equipment"); render();
    expect(std::none_of(buttons.begin(),buttons.end(),[](const Button& b){return b.id=="inventory-next";}),"short inventory hides pagination");
    clickTest("inventory-trade");
    if (!screenshotPath.empty()) screenshot(screenshotPath + ".trade.bmp");
    clickTest("trade-give-2");
    expect(game->getPlayers()[0].getSupply(Consumable::medkit)==0 && game->getPlayers()[1].getSupply(Consumable::medkit)==2 && game->getPlayerIndex()==0,"GUI transfers one medkit for free");
    clickTest("trade-close"); clickTest("inventory-close");

    game = Game(BossId::khorne,42); game->addPreset(0,"Market visitor"); game->start();
    history.clear(); gatherLog(); syncSelection(); beginStage(Presentation::idle);
    for (int moves=0;moves<20 && game->currentPlayer().getSector()!=8 && game->getOutcome()==Outcome::ongoing;++moves) {
        const auto next=game->routeStep(game->currentPlayer().getSector(),8);
        clickTest("sector-"+num(next)); clickTest("move"); settleTest();
    }
    expect(game->currentPlayer().getSector()==8 && acceptsOrders(),"GUI reaches Market");
    clickTest("market"); const auto marketState=snapshot(*game);
    if (!screenshotPath.empty()) screenshot(screenshotPath + ".market.bmp");
    clickTest("market-close"); expect(snapshot(*game)==marketState,"viewing market costs no action"); clickTest("market"); render();
    std::string offerButton;
    for (const auto& b:buttons) if (b.id.rfind("market-take-",0)==0 && b.enabled) {offerButton=b.id;break;}
    expect(!offerButton.empty(),"market has an available offer"); clickTest(offerButton);
    expect(modal==Modal::none && presentation==Presentation::action && game->getPhase()==TurnPhase::worldDraw,"GUI market selection spends action and animates");
    settleTest();

    game = Game::tutorial(9); history.clear(); gatherLog(); syncSelection(); beginStage(Presentation::idle);
    const auto intact=snapshot(*game); clickTest("collapse-bridge");
    if (!screenshotPath.empty()) screenshot(screenshotPath + ".bridge-warning.bmp");
    clickTest("bridge-cancel"); expect(snapshot(*game)==intact,"cancelled bridge warning changes nothing");
    clickTest("collapse-bridge"); clickTest("bridge-confirm");
    expect(game->isBridgeCollapsed() && !game->currentPlayer().getAlive() && game->getOutcome()==Outcome::defeat && presentation==Presentation::action,"confirmed collapse permanently kills final defender");
    updatePresentation(animationClock + static_cast<Uint32>(550*animationScale()));
    if (!screenshotPath.empty()) screenshot(screenshotPath + ".bridge-collapse.bmp");
    settleTest();

    // The complete phased wait loop reaches a real ending, then supports restart.
    startQuick();
    int actions = 0;
    while (game->getOutcome() == Outcome::ongoing && actions++ < 100) { clickTest("wait"); settleTest(); }
    expect(game->getOutcome() == Outcome::defeat, "phased GUI reaches defeat");
    clickTest("ending-journal"); clickTest("log-close"); clickTest("ending-new");
    expect(screen == Screen::setup, "debrief restart");
    startQuick(); clickTest("save"); const auto finalSave = fileContents(savePath);
    clickTest("guard"); settleTest(false); clickTest("menu"); clickTest("quit-unsaved");
    expect(!running && !game && fileContents(savePath) == finalSave, "full quit without saving preserves last file");
    SDL_SetWindowSize(window, 1000, 667); render();
    std::cout << "PASS GUI card draw/flip/hold, real phases, tactical orders, optional autosave, both no-save exits, saves and complete campaign\n";
}
