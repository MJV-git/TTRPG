#include "game.h"
#include "campaign_io.h"
#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>
#ifndef _WIN32
#include <unistd.h>
#endif

namespace {
bool color = false;
const char* ink(const char* code) { return color ? code : ""; }
const char* gold() { return ink("\033[1;33m"); }
const char* dim() { return ink("\033[2m"); }
const char* red() { return ink("\033[1;31m"); }
const char* cyan() { return ink("\033[1;36m"); }
const char* reset() { return ink("\033[0m"); }
const std::string defaultSave = "campaign.sav";

std::string trim(std::string s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
std::optional<std::string> readLine(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) return std::nullopt;
    return trim(line);
}
bool integer(const std::string& value, int& result) {
    if (value.empty()) return false;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    return parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size();
}
std::optional<int> choose(const std::string& prompt, int low, int high) {
    while (auto line = readLine(prompt)) {
        int value = 0;
        if (integer(*line, value) && value >= low && value <= high) return value;
        std::cout << "Enter a number from " << low << " to " << high << ".\n";
    }
    return std::nullopt;
}
std::string healthBar(int hp, int maximum) {
    const int fill = hp * 10 / maximum;
    return "[" + std::string(static_cast<std::size_t>(fill), '#') + std::string(static_cast<std::size_t>(10 - fill), '.') + "]";
}
void title() {
    std::cout << '\n' << gold()
              << "  4 0 K R P G  /  T H E  L A S T  W A T C H\n"
              << "  ========================================\n" << reset()
              << "  Nine sectors. One city. A dying light.\n\n"
              << "  The perimeter relays have failed. Beneath the streets, the breach stirs.\n"
              << "  Lead the last defenders, close the rifts, and hold back a sovereign.\n\n";
}
void help() {
    std::cout << cyan() << "  ORDERS / one action per defender per round\n" << reset()
              << "  move N          Follow a road; exits from Streets are free\n"
              << "  attack ID       Fire at / strike an enemy #ID in your sector\n"
              << "  portal          Attack your sector's portal (6 HP)\n"
              << "  boss            Attack the manifested boss in your sector\n"
              << "  search          Search for loot; Armory favors weapons\n"
              << "  rest            Recover 5 HP; Infirmary fully heals and cures corrosion\n"
              << "  medkit          Consume a medkit: recover 8 HP and cure corrosion\n"
              << "  grenade TARGET  TARGET: enemy ID, portal, or boss; always hits\n"
              << "  guard           Reduce incoming enemy damage by 3 this round\n"
              << "  suppress ID     Ranged hit: enemy skips its action, no damage dealt\n"
              << "  wait            Hold position and spend your action\n"
              << "  market [1-3]    View offers; take one for an action at Market\n"
              << "  collapse confirm Destroy Bridge; PERMANENT death to defenders there\n"
              << "  trade P ITEM    Free transfer to co-located player P; ITEM: medkit, grenade, weapon N\n\n"
              << "  FREE: equip N | inventory | map | status | help | rules\n"
              << "        save [path] | load [path] | quit\n"
              << "  Weapon numbers are shown in inventory. Invalid orders cost nothing.\n"
              << "  Paths may contain spaces; type them without quotes.\n\n";
}
void rules() {
    std::cout << gold() << "  FIELD MANUAL\n" << reset()
              << "  Win by closing ALL portals and defeating ALL enemies, including\n"
              << "  the boss if present. Slaying the sovereign also wins immediately.\n"
              << "  If doom reaches the boss deadline, defeat its avatar within 8 rounds.\n"
              << "  Instability 6 or the end of that final countdown means defeat.\n\n"
              << "  Starting enemies: two per defender (2/4/6/8 total).\n"
              << "  Each player takes ONE action. Then: one shared event, corrosion damage,\n"
              << "  enemy attacks, and doom advances by 1 + instability. Lost Hours adds 2.\n"
              << "  Every fifth round, portals spawn one enemy per defender GLOBALLY.\n"
              << "  Portals without defenders take priority. New enemies act next round.\n"
              << "  Breach tuners attack local defenders with psychic pulses.\n\n"
              << "  Routes are directional. A sector lists only its outgoing exits;\n"
              << "  entering from another sector does not guarantee a return route.\n"
              << "  Twisting Streets temporarily reverses one-way roads. All movement\n"
              << "  follows current exits, except Transport crew flights to any sector.\n\n"
              << "  Player weapons: 80% base hit chance, minus 20 percentage points\n"
              << "  per unmet requirement; +5 per surplus aiming stat point (max 100%).\n"
              << "  Ranged weapons use Precision; heavy weapons also require Strength.\n"
              << "  Melee uses Strength; psychic weapons use Will. Inspect requirements.\n"
              << "  Armor reduces physical damage; Will reduces psychic damage.\n"
              << "  Unguarded portals are automatic hits; guarded ones use your normal\n"
              << "  weapon chance. World conditions can temporarily reduce accuracy.\n"
              << "  Grenades destroy portals, or deal 5 to enemies and bosses,\n"
              << "  ignoring resistance. Attacks and items target only your sector.\n"
              << "  Gate anchors block weapon damage to portals until defeated.\n"
              << "  Grenades bypass the shield; suppression does not break it.\n\n"
              << "  Enemies roll to hit and independently choose a local player. Instability\n"
              << "  adds to their raw damage. At zero HP you recover in the Infirmary\n"
              << "  with full HP, lose corrosion, and add 1 instability. You cannot be attacked again\n"
              << "  that phase. You can act normally next round.\n"
              << "  The boss starts at the Bridge. If no eligible defender is there,\n"
              << "  it moves toward the closest one, spending its attacks to move.\n\n"
              << "  Searching spends an action to find loot. Enemy kills may drop loot; closing\n"
              << "  a portal grants a medkit. Only world events can lower instability. Each round\n"
              << "  with a non-quiet event has a 25% bonus supply chance. Equipping is free.\n"
              << "  Half of card draws are quiet; 22 other events form a shuffled deck.\n\n"
              << "  Guard reduces incoming hits by 3 before resistance this round.\n"
              << "  Suppress with a ranged weapon: roll weapon accuracy to skip\n"
              << "  its next activation without damage, including a last-stand ritual.\n"
              << "  Rust drones inflict corrosion on damaging hits. Suppress or eliminate\n"
              << "  them early; medkits and Infirmary rest cure infections.\n\n"
              << "  FIRST PATROL: Bulwark + Ember Regent. Look for a nearby rift, move\n"
              << "  there, and use 'portal' or 'grenade portal'. Don't spend all five\n"
              << "  opening rounds scavenging. Save grenades for guarded portals or\n"
              << "  armored enemies. Protect defenders: instability increases are lasting.\n\n"
              << "  BEFORE THE SOVEREIGN: With no portals, enemies hold position and each\n"
              << "  has a 10% summon chance per hostile activation instead of acting.\n"
              << "  Suppression skips the activation. A new portal restores normal\n"
              << "  behavior. Boss arrival ends rituals permanently, even with no portals.\n\n";
}
void inventory(const Game& game) {
    const auto& p = game.currentPlayer();
    std::cout << cyan() << "  " << p.getName() << " / ARMAMENT\n" << reset();
    for (WeaponId id : p.getWeapons()) {
        const auto& w = weapon(id);
        const auto profile = weaponProfile(id, p);
        std::cout << "  " << static_cast<int>(id) + 1 << ". " << std::left << std::setw(15) << w.name
                  << "  " << game.playerDamage(p,id) << (profile.type == damageType::psychic ? " psychic " : " physical")
                  << (game.playerTrueBonus(p) ? " +5 true" : "") << "  hit chance " << game.playerHitChance(p,id) << "% / requires " << weaponRequirements(id);
        if (profile.underRequirement) std::cout << " [unmet requirement: -20 percentage points each]";
        if (id == p.getWeapon()) std::cout << gold() << "  EQUIPPED" << reset();
        std::cout << '\n';
    }
    std::cout << "  Medkits: " << p.getSupply(Consumable::medkit)
              << " | Breach grenades: " << p.getSupply(Consumable::grenade) << "\n\n";
}
void board(const Game& game) {
    std::cout << '\n' << gold() << "  SECTOR COMMAND / " << game.getBossDef().title << reset() << '\n'
              << "  Round " << game.getRound() << "  |  Doom " << game.getDoom() << '/' << game.getBossDef().deadline
              << "  |  Instability " << game.getInstability() << "/6  |  Portals " << game.getPortals().size();
    if (game.hasLivingBoss()) std::cout << red() << "  |  FINAL ROUNDS " << game.getFinalRounds() << reset();
    else std::cout << "  |  Next wave after round " << ((game.getRound() - 1) / 5 + 1) * 5;
    std::cout << "\n  " << dim() << game.getBossDef().rule << reset() << '\n'
              << "  -----------------------------------------------------------------------------\n"
              << "     SECTOR          EXITS ->        ACCESS CONTACTS\n";
    for (const auto& sector : sectors()) {
        std::ostringstream roads, contacts;
        const auto routes = game.getRoutes(sector.id);
        for (std::size_t i = 0; i < routes.size(); ++i) {
            if (i) roads << ',';
            roads << routes[i];
        }
        for (std::size_t i = 0; i < game.getPlayers().size(); ++i)
            if (game.getPlayers()[i].getAlive() && game.getPlayers()[i].getSector() == sector.id) contacts << "P" << i + 1 << ' ';
        for (const auto& p : game.getPortals()) if (p.sector == sector.id) contacts << "RIFT:" << p.health << "hp ";
        int count = 0;
        for (const auto& e : game.getEnemies()) if (e.getSector() == sector.id) ++count;
        if (count) contacts << "ENEMIES:" << count << ' ';
        if (game.hasLivingBoss() && game.getBoss()->getSector() == sector.id) contacts << "BOSS:" << game.getBoss()->getHealth() << "hp";
        const bool current = game.currentPlayer().getSector() == sector.id;
        if (current) std::cout << cyan();
        std::cout << "  " << (current ? '>' : ' ') << sector.id << ' ' << std::left << std::setw(15) << sector.name
                  << std::setw(16) << roads.str() << std::setw(7) << (game.isBridgeCollapsed() && sector.id==9 ? "GONE" : game.getClosure(sector.id) ? "CLOSED" : "OPEN") << contacts.str() << reset() << '\n';
    }
    std::cout << "  -----------------------------------------------------------------------------\n";
    for (std::size_t i = 0; i < game.getPlayers().size(); ++i) {
        const auto& p = game.getPlayers()[i];
        std::cout << "  P" << i + 1 << ' ' << p.getName() << ' ' << healthBar(p.getHealth(), p.getMaxHealth())
                  << ' ' << p.getHealth() << '/' << p.getMaxHealth() << " HP  /  " << weapon(p.getWeapon()).name;
        if (p.getPoison()) std::cout << red() << "  CORROSION:" << p.getPoison() << reset();
        std::cout << '\n';
    }
}
void status(const Game& game) {
    const auto& p = game.currentPlayer();
    if (game.isLastStand()) std::cout << red() << "  ENEMY LAST STAND: 10% summon chance per enemy activation. Clear all enemies and portals.\n" << reset();
    std::cout << '\n' << cyan() << "  YOUR ORDER / P" << game.getPlayerIndex() + 1 << ' ' << p.getName()
              << " / " << sectorAt(p.getSector()).name << reset() << '\n'
              << "  HP " << p.getHealth() << '/' << p.getMaxHealth() << "  ARM " << p.getArmor()
              << "  STR " << p.getStrength() << "  PRE " << p.getPrecision() << "  WILL " << p.getWill() << '\n';
    const auto profile = weaponProfile(p.getWeapon(), p);
    std::cout << "  " << weapon(p.getWeapon()).name << ": " << game.playerDamage(p,p.getWeapon())
              << (profile.type == damageType::psychic ? " psychic" : " physical") << (game.playerTrueBonus(p) ? " +5 true" : "") << " damage, accuracy "
              << game.playerHitChance(p, p.getWeapon()) << "%" << '\n';
    for (const auto& e : game.getEnemies()) if (e.getSector() == p.getSector())
        std::cout << red() << "  TARGET #" << e.getId() << " " << e.getName() << reset() << "  HP " << e.getHealth()
                  << '/' << e.getMaxHealth() << "  ARM " << e.getArmor() << "  WILL " << e.getWill() << '\n';
    for (const auto& rift : game.getPortals()) if (rift.sector == p.getSector()) std::cout << red() << "  TARGET portal" << reset() << "  HP " << rift.health << "/6\n";
    if (game.hasLivingBoss() && game.getBoss()->getSector() == p.getSector())
        std::cout << red() << "  TARGET boss / " << game.getBoss()->getName() << reset() << "  HP " << game.getBoss()->getHealth()
                  << '/' << game.getBoss()->getMaxHealth() << "  ARM " << game.getBoss()->getArmor() << "  WILL " << game.getBoss()->getWill() << '\n';
    std::cout << "  Outgoing routes ->";
    for (int destination : game.getRoutes(p.getSector())) std::cout << "  " << destination << '=' << sectorAt(destination).name;
    std::cout << "\n  Supplies: medkit " << p.getSupply(Consumable::medkit) << " / grenade " << p.getSupply(Consumable::grenade)
              << "\n"
              << dim() << "  move N | attack ID | portal | search | rest | grenade TARGET | help" << reset() << '\n';
}
void journal(Game& game) {
    for (const auto& line : game.takeLog()) std::cout << "  " << dim() << "| " << reset() << line << '\n';
}
std::optional<Game> createGame(std::uint32_t seed) {
    std::cout << gold() << "  CHOOSE THE ADVERSARY\n" << reset();
    for (const auto& boss : bosses())
        std::cout << "  " << static_cast<int>(boss.id) + 1 << ". " << boss.name << " / Doom limit " << boss.deadline << '\n'
                  << "     " << boss.rule << '\n';
    auto selected = choose("\n  Boss [1-4]: ", 1, 4);
    if (!selected) return std::nullopt;
    Game game(static_cast<BossId>(*selected - 1), seed);
    auto count = choose("  Defenders [1-4, shared keyboard co-op]: ", 1, 4);
    if (!count) return std::nullopt;
    for (int i = 0; i < *count; ++i) {
        std::cout << '\n' << cyan() << "  RECRUIT DEFENDER " << i + 1 << reset() << '\n';
        for (std::size_t j = 0; j < characters().size(); ++j) {
            const auto& c = characters()[j];
            std::cout << "  " << j + 1 << ". " << c.name << " / " << weapon(c.weapon).name << '\n'
                      << "     HP " << c.health << " ARM " << c.armor << " STR " << c.strength << " PRE " << c.precision << " WILL " << c.will
                      << "\n     " << c.description << '\n';
        }
        std::cout << "  5. Custom / 50-point budget / Choose starting weapon\n";
        auto preset = choose("  Character [1-5]: ", 1, 5);
        if (!preset) return std::nullopt;
        std::optional<std::string> name;
        while (true) {
            name = readLine("  Callsign (1-32 characters, Enter for default): ");
            if (!name) return std::nullopt;
            if (name->empty()) *name = *preset == 5 ? "Defender " + std::to_string(i + 1) : characters().at(*preset - 1).name;
            if (name->size() <= 32 && std::none_of(name->begin(), name->end(), [](unsigned char c) { return c < 32 || c == 127; })) break;
            std::cout << "  Use 1-32 printable characters.\n";
        }
        if (*preset != 5) game.addPreset(*preset - 1, *name);
        else {
            std::cout << "  Starting weapon: 1 Rotary riveter, 2 Beam carbine, 3 Resonance staff, 4 Arc blade\n";
            const auto starting = choose("  Weapon [1-4]: ", 1, 4);
            if (!starting) return std::nullopt;
            std::cout << "  Enter five stats: Vitality Armor Strength Precision Will.\n"
                      << "  Cost per stat = n*(n+1)/2; budget 50. Vitality 1-9, Armor 0-4,\n"
                      << "  others 0-9. HP = 1 + 3*Vitality. Example: 4 4 4 4 4 costs 50.\n";
            while (true) {
                auto stats = readLine("  Stats: ");
                if (!stats) return std::nullopt;
                std::istringstream input(*stats);
                int v = 0, a = 0, s = 0, p = 0, w = 0;
                std::string extra;
                if ((input >> v >> a >> s >> p >> w) && !(input >> extra) && game.addCustom(v, a, s, p, w, *name, characters()[*starting - 1].weapon)) break;
                std::cout << "  Invalid stats or budget exceeded. Try 4 4 4 4 4.\n";
            }
        }
    }
    game.start();
    return game;
}

void play(Game game, std::string savePath) {
    game.resolvePending();
    std::cout << "\n  Deployment seed: " << game.getSeed() << " / 'save' preserves your campaign.\n";
    journal(game); board(game);
    bool showStatus = true;
    while (true) {
        if (game.getOutcome() != Outcome::ongoing) {
            std::cout << '\n' << gold() << "  " << game.getEnding() << reset() << '\n'
                      << "  Debrief: " << game.getClosed() << " portals sealed, " << game.getKills() << " enemies defeated, "
                      << "round " << game.getRound() << ", instability " << game.getInstability() << ".\n"
                      << "  Start another patrol with ./play.sh\n\n";
            return;
        }
        if (showStatus) status(game);
        showStatus = false;
        auto line = readLine("\n  Order > ");
        std::string normalized = line.value_or("");
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (!line || normalized == "quit" || normalized == "exit" || normalized == "q") {
            try { saveCampaign(game, savePath); std::cout << "  Campaign saved to " << savePath << ". Resume with --load or the menu.\n"; }
            catch (const std::exception& e) {
                std::cout << "  Save failed: " << e.what() << '\n';
                if (line) { std::cout << "  Still in game. Use 'save another-path' before quitting.\n"; continue; }
                std::cout << "  Input ended; unsaved progress could not be preserved.\n";
            }
            return;
        }
        std::istringstream input(*line);
        std::string command, argument;
        input >> command;
        std::getline(input, argument); argument = trim(argument);
        std::transform(command.begin(), command.end(), command.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (command.empty()) continue;
        if (command == "save" || command == "load") {
            const auto path = argument.empty() ? savePath : argument;
            try {
                if (command == "save") { saveCampaign(game, path); std::cout << "  Saved to " << path << ".\n"; }
                else { auto restored = loadCampaign(path); restored.resolvePending(); game = std::move(restored); journal(game); board(game); showStatus = true; }
                savePath = path;
            } catch (const std::exception& e) { std::cout << "  " << e.what() << '\n'; }
            continue;
        }
        if (command == "trade") {
            std::istringstream args(argument); int recipient=0, id=0; std::string type, extra;
            args >> recipient >> type;
            Item item {type=="weapon",0};
            bool valid = recipient>0;
            if (type=="weapon") { valid = valid && static_cast<bool>(args>>id) && id>0; item.id=id>0 ? id-1 : -1; }
            else if (type=="medkit" || type=="grenade") item.id=type=="grenade" ? 1 : 0;
            else valid=false;
            if (args>>extra) valid=false;
            if (!valid) std::cout << "  Use: trade PLAYER medkit|grenade|weapon NUMBER\n";
            else { const auto result=game.trade(static_cast<std::size_t>(recipient-1),item);
                if (!result.accepted) std::cout << "  " << result.error << '\n';
                else { journal(game); showStatus=true; }
            }
            continue;
        }
        if (command=="market" && argument.empty()) {
            for (int i=0;i<3;++i) std::cout << "  " << i+1 << ". " << itemName(game.getMarketOffers()[i]) << '\n';
            std::cout << "  At Market: market 1-3 takes one item for an action. Offers refresh each defender turn.\n";
            continue;
        }
        if (command=="collapse" && argument!="confirm") {
            std::cout << "  Destroying Bridge costs a breach grenade and PERMANENTLY KILLS all defenders there.\n"
                      << "  Enemies on Bridge die. ONLY a sovereign on Bridge loses half current HP and moves to Slums.\n"
                      << "  Bridge stays inaccessible. Stand on Bridge and type: collapse confirm\n";
            continue;
        }
        if (command == "equip") {
            int id = 0;
            if (!integer(argument, id) || id < 1 || id > static_cast<int>(weapons().size()) || !game.equip(static_cast<WeaponId>(id - 1)))
                std::cout << "  Choose a weapon number you own; type 'inventory'.\n";
            else { std::cout << "  Equipped " << weapon(game.currentPlayer().getWeapon()).name << ". No action spent.\n"; showStatus = true; }
            continue;
        }
        if (argument.empty()) {
            if (command == "help" || command == "?") { help(); continue; }
            if (command == "rules") { rules(); continue; }
            if (command == "map") { board(game); continue; }
            if (command == "status") { status(game); continue; }
            if (command == "inventory" || command == "inv") { inventory(game); continue; }
        }
        std::optional<Action> action;
        int target = 0;
        if ((command == "move" || command == "attack" || command == "suppress") && integer(argument, target))
            action = Action{command == "move" ? ActionType::move : command == "suppress" ? ActionType::suppress : ActionType::attackEnemy, target};
        else if (command=="market" && integer(argument,target) && target>=1 && target<=3) action=Action{ActionType::market,target-1};
        else if (command=="collapse" && argument=="confirm") action=Action{ActionType::collapseBridge};
        else if (command == "grenade") {
            if (argument == "portal") action = Action{ActionType::grenadePortal};
            else if (argument == "boss") action = Action{ActionType::grenadeBoss};
            else if (integer(argument, target)) action = Action{ActionType::grenadeEnemy, target};
        } else if (argument.empty()) {
            if (command == "portal") action = Action{ActionType::attackPortal};
            if (command == "boss") action = Action{ActionType::attackBoss};
            if (command == "search") action = Action{ActionType::search};
            if (command == "rest") action = Action{ActionType::rest};
            if (command == "medkit") action = Action{ActionType::medkit};
            if (command == "wait") action = Action{ActionType::wait};
            if (command == "guard") action = Action{ActionType::guard};
        }
        if (!action) { std::cout << "  Unknown order or extra arguments. Type 'help' for commands.\n"; continue; }
        const auto result = game.act(*action);
        if (!result.accepted) std::cout << "  " << result.error << " No action spent.\n";
        else { journal(game); board(game); showStatus = true; }
    }
}
}

int main(int argc, char** argv) {
    try {
#ifndef _WIN32
        color = isatty(STDOUT_FILENO) && std::getenv("NO_COLOR") == nullptr;
#endif
        bool quick = false;
        std::string loadPath;
        std::uint32_t seed = std::random_device{}();
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--no-color") color = false;
            else if (arg == "--quick") quick = true;
            else if (arg == "--load" && i + 1 < argc) loadPath = argv[++i];
            else if (arg == "--seed" && i + 1 < argc) {
                const std::string value = argv[++i];
                const auto parsed = std::from_chars(value.data(), value.data() + value.size(), seed);
                if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) throw std::runtime_error("Seed must be an unsigned 32-bit integer.");
            } else if (arg == "--help" || arg == "-h") {
                std::cout << "Veyra / The Last Watch\nUsage: ttrpg [--quick] [--seed N] [--load PATH] [--no-color]\n"
                          << "  --quick     Deploy a random preset defender against Ember Regent\n"
                          << "  --seed N    Repeat a starting scenario (same build and commands)\n"
                          << "  --load PATH Resume a saved campaign\n"
                          << "  --no-color  Plain terminal output\n";
                return 0;
            } else throw std::runtime_error("Unknown or incomplete option: " + arg + ". Use --help.");
        }
        if (quick && !loadPath.empty()) throw std::runtime_error("Choose either --quick or --load.");
        title();
        if (!loadPath.empty()) { play(loadCampaign(loadPath), loadPath); return 0; }
        if (quick) {
            Game game(BossId::emberRegent, seed);
            const auto recruit = quickDefender(seed); game.addPreset(recruit.preset, recruit.name); game.start();
            play(std::move(game), defaultSave); return 0;
        }
        while (true) {
            std::cout << "  1. New campaign\n  2. Resume campaign.sav\n  3. Field manual\n  4. Exit\n";
            auto choice = choose("\n  Command [1-4]: ", 1, 4);
            if (!choice || *choice == 4) return 0;
            if (*choice == 3) { rules(); continue; }
            if (*choice == 2) {
                try { play(loadCampaign(defaultSave), defaultSave); return 0; }
                catch (const std::exception& e) { std::cout << "  " << e.what() << "\n\n"; continue; }
            }
            auto game = createGame(seed);
            if (!game) return 0;
            help(); play(std::move(*game), defaultSave); return 0;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
