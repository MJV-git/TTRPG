#include "game.h"
#include <algorithm>
#include <iomanip>
#include <istream>
#include <limits>
#include <numeric>
#include <ostream>
#include <set>
#include <stdexcept>
#include <utility>

namespace {
bool validName(const std::string& name) {
    return !name.empty() && name.size() <= 32 &&
        std::any_of(name.begin(), name.end(), [](unsigned char c) { return c > 32; }) &&
        std::none_of(name.begin(), name.end(), [](unsigned char c) { return c < 32 || c == 127; });
}
std::string location(int sector) { return sectorAt(sector).name; }
}

const std::vector<CharacterDef>& characters() {
    static const std::vector<CharacterDef> values {
        {"Bulwark", 16, 3, 4, 3, 1, WeaponId::rotaryRiveter, "Heavy firepower and armor; vulnerable to psychic attacks."},
        {"Trailguard", 16, 2, 2, 5, 2, WeaponId::beamCarbine, "Accurate ranged fire; starts with an extra breach grenade."},
        {"Resonant", 13, 1, 1, 3, 5, WeaponId::staff, "Psychic attacks and strong resistance to hostile signals."},
        {"Duskblade", 16, 2, 3, 3, 4, WeaponId::sword, "Balanced melee fighter; strong physical and psychic defenses."}
    };
    return values;
}
QuickDefender quickDefender(std::uint32_t seed, int previous) {
    std::mt19937 names(seed);
    const int preset = previous < 0 ? std::uniform_int_distribution<int>(0, 3)(names) : (previous + 1) % 4;
    static const std::array<const char*, 4> titles {{"Shield", "Scout", "Listener", "Blade"}};
    static const std::array<const char*, 8> callsigns {{"Kestrel", "Flint", "Lumen", "Tarn", "Cairn", "Sol", "Vale", "Rook"}};
    return {preset, std::string(titles[preset]) + " " + callsigns[std::uniform_int_distribution<int>(0, 7)(names)]};
}
const std::vector<EventCard>& eventCards() {
    static const std::vector<EventCard> cards {
        {"A Moment of Silence", "RESPITE", "For one breath, even the guns are still.", "No additional board effect. Take stock before the enemy phase.", 0},
        {"Relay Airlift", "SUPPLY", "Supply skiffs follow the last working beacon.", "Every defender receives one random consumable.", 1},
        {"Signal Procession", "REINFORCEMENT", "A broken carrier signal assembles itself into a machine.", "A Breach tuner spawns in Market (8). It attacks local defenders with psychic damage.", 2},
        {"Breach in the Veil", "SIGNAL BREACH", "A new wound opens in the world.", "Open a 6 HP portal in a random eligible sector. Barracks and Infirmary are protected.", 3},
        {"Twisting Streets", "TERRAIN", "The road home now leads away from it.", "Reverse all one-way roads for this round and the next. Two-way roads stay two-way. All movement follows the changed arrows.", 4},
        {"The Beacon Holds", "RESPITE", "The district reconnects its relays, one rooftop at a time.", "Reduce instability by 1, to a minimum of zero.", 0},
        {"Lost Hours", "OMEN", "The cathedral bell tolls twice for an hour that never was.", "Advance doom by an extra 2. The normal round advance still happens afterward.", 5},
        {"Hunting Pack", "REINFORCEMENT", "Something has caught your scent.", "The sovereign sends a hunting construct to a random portal, or Slums (7) if none remain. It pursues defenders along outgoing roads.", 2},
        {"Field Hospital", "SUPPLY", "The wounded return to the line.", "Every defender heals 4 HP and is cured of corrosion.", 1},
        {"Arsenal Unsealed", "SUPPLY", "A maintenance key opens a forgotten equipment vault.", "One random defender receives a weapon. Duplicate weapons become a consumable.", 1},
        {"Supply Convoy", "WORLD CHANGE", "Supplies reach the district under cover of smoke.", "Every defender receives one medkit.", 1},
        {"The Long March", "MANEUVER", "They are moving. All of them.", "Every regular enemy moves one random outgoing road, except during the last stand. Current road reversals apply. The boss stays put.", 4},
        {"Anchor Deployment", "REINFORCEMENT", "The gate has found its guardian.", "Add a Gate anchor at a portal without one. Its shield blocks weapons, but not breach grenades. No eligible portal: no effect.", 3},
        {"Sovereign Emissary", "REINFORCEMENT", "An unfamiliar command signature takes physical form.", "A construct controlled by the invading sovereign appears in Slums (7). Inspect its tactics before engaging.", 2},
        {"Breach Collapse", "RIFT DESTRUCTION", "One wound in reality finally closes.", "Destroy one random portal. Before the sovereign manifests, surviving enemies enter last stand if no portals remain.", 3},
        {"Orbital Pinpoint", "ENEMY DESTRUCTION", "A precise beam finds its mark through the smoke.", "Destroy one random regular enemy. Bosses are unaffected.", 2},
        {"Quarantine Cordon", "SECTOR CLOSURE", "Blast doors descend across the district.", "Close one eligible sector to entry through the next round. Occupants may leave. Other open sectors remain connected.", 4},
        {"Emergency Teleport", "TRANSPORT", "The relay locks onto every defender's transponder.", "Transport each defender to a random different open sector. This costs no player action and causes no damage.", 3},
        {"Ashfall", "DEFENDER ACCURACY", "A curtain of ash swallows every sight line.", "Defender weapon and suppression hit chances fall by 20 percentage points through the next round. Grenades are unaffected.", 5},
        {"Blinding Flare", "ENEMY ACCURACY", "White light burns across the enemy's vision.", "Enemy and boss attack rolls take -4 (20 percentage points) through the next round. Ritual chances are unaffected.", 0},
        {"Purge the Streets", "ENEMY DESTRUCTION", "District artillery answers the distress signal.", "Destroy all regular enemies in one random occupied sector. Defenders and bosses are unaffected.", 2},
        {"Null Resonance", "RIFT DESTRUCTION", "The city falls silent as every rift implodes.", "Destroy every portal. Before the sovereign manifests, surviving enemies enter last stand and attempt to summon new rifts.", 3},
        {"Transport crew", "TRANSPORT", "Volunteer pilots offer a way above the broken streets.", "During the next defender phase, each defender may spend their move action to travel to any sector, ignoring roads and sector closures. Enemies still follow roads.", 4}
    };
    return cards;
}
const std::vector<std::string>& eventNames() {
    static const std::vector<std::string> names = [] {
        std::vector<std::string> result;
        for (const auto& card : eventCards()) result.push_back(card.name);
        return result;
    }();
    return names;
}
Game::Game(BossId selected, std::uint32_t randomSeed) : patron(selected), seed(randomSeed), rng(seed) {
    bossDefinition(selected); // Validate before starting a game.
}
int Game::roll(int low, int high) { return std::uniform_int_distribution<int>(low, high)(rng); }
Game Game::tutorial(int sector) {
    Game g(BossId::emberRegent, 42);
    g.addPreset(0, "Training Defender"); g.start();
    g.enemies.clear(); g.portals = {{2, 6}, {9, 6}};
    g.spawnEnemy(2, 0); g.players[0].moveTo(sector);
    g.players[0].takeDamage(damageType::trueDamage, 6);
    for (const auto& w : weapons()) g.players[0].giveWeapon(w.id);
    g.deck = {0}; g.log.clear();
    g.record("TRAINING / Practice orders here. Your campaign and saves are untouched.");
    return g;
}
void Game::record(const std::string& message) { log.push_back(message); }
int Game::statCost(int value) { return value < 0 || value > 9 ? 1000 : value * (value + 1) / 2; }

bool Game::addPreset(int preset, const std::string& name) {
    if (started || players.size() >= 4 || preset < 0 || preset >= static_cast<int>(characters().size()) || !validName(name)) return false;
    const auto& c = characters().at(preset);
    players.emplace_back(c.health, c.armor, c.strength, c.precision, c.will, 1, name);
    auto& p = players.back();
    p.giveWeapon(c.weapon);
    p.equip(c.weapon);
    if (preset == 1) ++p.supplies[1];
    return true;
}
bool Game::addCustom(int vitality, int armor, int strength, int precision, int will, const std::string& name, WeaponId startingWeapon) {
    if (started || players.size() >= 4 || !validName(name) || vitality < 1 || armor > 4 ||
        statCost(vitality) + statCost(armor) + statCost(strength) + statCost(precision) + statCost(will) > 50) return false;
    if (std::none_of(characters().begin(), characters().end(), [startingWeapon](const CharacterDef& c) { return c.weapon == startingWeapon; })) return false;
    players.emplace_back(1 + 3 * vitality, armor, strength, precision, will, 1, name);
    players.back().giveWeapon(startingWeapon); players.back().equip(startingWeapon);
    return true;
}
void Game::start() {
    if (started || players.empty()) throw std::logic_error("Create one to four characters before starting once.");
    started = true;
    spawnPortal(); spawnPortal();
    if (patron == BossId::prismOracle) { spawnPortal(); spawnPortal(); }
    for (std::size_t i = 0; i < 2 * players.size(); ++i) {
        const auto& rift = portals.at(static_cast<std::size_t>(roll(0, static_cast<int>(portals.size()) - 1)));
        spawnEnemy(rift.sector, i % 2 == 0 ? patronEnemy() : 4);
    }
    record("Veyra's relay network falls silent. " + getBossDef().name + " is reaching for this city.");
    record("Close every portal and defeat every enemy, OR defeat the manifested sovereign to save the city. First portal wave: end of round 5.");
    refreshMarket();
}
void Game::spawnEnemy(int sector, int kind) {
    if (enemies.size() >= 64) {
        instability = std::min(6, instability + 1);
        record("The streets overflow with enemies. Instability rises.");
        return;
    }
    enemies.emplace_back(nextEnemyId++, kind, sector);
    record(enemies.back().getName() + " arrives in " + location(sector) + ".");
}
void Game::spawnPortal(int preferredSector) {
    std::vector<int> available;
    for (int i : {2, 3, 4, 5, 7, 8, 9}) {
        if (!getClosure(i) && !(bridgeCollapsed && i == 9) && std::none_of(portals.begin(), portals.end(), [i](const Portal& p) { return p.sector == i; })) available.push_back(i);
    }
    if (available.empty()) { record("The veil strains, but no further rift can open."); return; }
    const int sector = std::find(available.begin(), available.end(), preferredSector) != available.end() ? preferredSector :
        available.at(static_cast<std::size_t>(roll(0, static_cast<int>(available.size()) - 1)));
    portals.push_back({sector, 6});
    record("A 6 HP portal opens in " + location(sector) + ".");
}
void Game::summonPortal(int sector, const std::string& source) {
    const int die = roll(1, 10);
    record(source + " attempts a last-stand ritual: d10 " + std::to_string(die) + " (1 summons a portal).");
    if (die == 1) spawnPortal(sector);
    else record("The ritual fails. No portal opens.");
}
void Game::announceLastStand(bool previous) {
    if (!previous && isLastStand())
        record("ENEMY LAST STAND / All portals are closed. Each surviving enemy stops fighting and moving to attempt a portal summon: 10% per hostile activation. Suppression skips an activation. A new portal restores normal behavior. Defeat every enemy and close every portal to win.");
    else if (previous && boss && outcome == Outcome::ongoing)
        record("LAST STAND ENDS / The sovereign has manifested. Enemies resume normal behavior; summoning rituals are disabled for the rest of the campaign.");
    else if (previous && !isLastStand() && !portals.empty() && outcome == Outcome::ongoing)
        record("THE RIFT REOPENS / The last stand ends. Remaining enemies resume normal behavior immediately.");
}
void Game::grantSupply(Player& player, Consumable item) {
    if (!player.getAlive()) return;
    auto& count = player.supplies.at(static_cast<std::size_t>(item));
    if (count < 99) ++count;
    record(player.getName() + " receives a " + consumableName(item) + ".");
}
void Game::grantLoot(Player& player, bool weaponPreferred) {
    if (!player.getAlive()) return;
    if (weaponPreferred || roll(1, 4) == 1) {
        WeaponId found;
        switch (player.getSector()) {
            case 2: found = static_cast<WeaponId>(roll(2, static_cast<int>(weapons().size()) - 1)); break;
            case 5: found = roll(0, 1) ? WeaponId::staff : WeaponId::resonanceGlaive; break;
            case 1: found = roll(1, 3) == 1 ? WeaponId::needleRifle : WeaponId::rivetRifle; break;
            default: found = static_cast<WeaponId>(roll(1, static_cast<int>(weapons().size()) - 1)); break;
        }
        if (player.giveWeapon(found)) {
            record(player.getName() + " finds a " + weapon(found).name + ". Equip it for free on your turn.");
            return;
        }
        record("Duplicate weapon salvaged for supplies.");
    }
    grantSupply(player, player.getSector() == 6 ? Consumable::medkit : static_cast<Consumable>(roll(0, 1)));
}
bool Game::attackHits(int bonus, int difficulty) {
    const int die = roll(1, 20);
    const bool hit = die == 20 || (die != 1 && die + bonus >= difficulty);
    record("  d20 " + std::to_string(die) + (bonus >= 0 ? " + " : " - ") + std::to_string(std::abs(bonus)) +
           " vs " + std::to_string(difficulty) + (hit ? ": HIT." : ": MISS."));
    return hit;
}
bool Game::equip(WeaponId id) {
    if (!started || outcome != Outcome::ongoing || phase != TurnPhase::players) return false;
    return players.at(playerIdx).equip(id);
}
std::string itemName(Item item) {
    return item.isWeapon ? weapon(static_cast<WeaponId>(item.id)).name : consumableName(static_cast<Consumable>(item.id));
}
ActionResult Game::trade(std::size_t recipient, Item item) {
    if (!started || outcome != Outcome::ongoing || phase != TurnPhase::players) return {false, "Trade during your turn."};
    if (recipient >= players.size() || recipient == playerIdx) return {false, "Choose another defender."};
    auto& giver = players[playerIdx]; auto& receiver = players[recipient];
    if (!giver.getAlive() || !receiver.getAlive() || giver.getSector() != receiver.getSector()) return {false, "Both defenders must be in the same sector."};
    if (item.id < 0 || item.id >= (item.isWeapon ? static_cast<int>(weapons().size()) : 2)) return {false, "Unknown item."};
    if (item.isWeapon) {
        const auto id = static_cast<WeaponId>(item.id);
        const auto owned = std::find(giver.arsenal.begin(), giver.arsenal.end(), id);
        if (owned == giver.arsenal.end()) return {false, "You do not own that weapon."};
        if (giver.arsenal.size() == 1) return {false, "Keep at least one weapon."};
        if (std::find(receiver.arsenal.begin(), receiver.arsenal.end(), id) != receiver.arsenal.end()) return {false, "That defender already owns this weapon."};
        receiver.arsenal.push_back(id); giver.arsenal.erase(owned);
        if (giver.equipped == id) giver.equipped = giver.arsenal.front();
    } else {
        if (!giver.supplies[item.id]) return {false, "No item remaining."};
        if (receiver.supplies[item.id] >= 99) return {false, "Recipient cannot carry more."};
        --giver.supplies[item.id]; ++receiver.supplies[item.id];
    }
    record(giver.getName() + " gives " + itemName(item) + " to " + receiver.getName() + ".");
    return {true, {}};
}

ActionResult Game::act(Action action, bool automatic) {
    combatEvents.clear();
    if (!started || outcome != Outcome::ongoing) return {false, "This campaign is not active."};
    if (phase != TurnPhase::players) return {false, "Resolve the current phase before issuing a defender order."};
    const bool wasLastStand = isLastStand();
    auto& p = players.at(playerIdx);
    const int here = p.getSector();
    auto enemy = std::find_if(enemies.begin(), enemies.end(), [&](const Enemy& e) { return e.getId() == action.target && e.getSector() == here; });
    auto portal = std::find_if(portals.begin(), portals.end(), [here](const Portal& r) { return r.sector == here; });
    const bool guarded = std::any_of(enemies.begin(), enemies.end(), [here](const Enemy& e) { return e.getSector() == here; }) ||
                         (boss && boss->getAlive() && boss->getSector() == here);
    const bool bossHere = boss && boss->getAlive() && boss->getSector() == here;
    const auto profile = weaponProfile(p.getWeapon(), p);
    const bool movingFree = action.type == ActionType::move && freeMove(here, action.target);
    const auto style = profile.type == damageType::psychic ? CombatStyle::psychic :
        weapon(p.getWeapon()).accuracy == Accuracy::precision ? CombatStyle::ranged : CombatStyle::melee;
    auto strike = [&](Entity& target, bool grenade) {
        const bool hit = grenade || playerHits(p);
        const int raw = grenade ? 5 : playerDamage(p, p.getWeapon());
        int dealt = hit ? target.takeDamage(grenade ? damageType::trueDamage : profile.type, raw) : 0;
        if (hit) dealt += target.takeDamage(damageType::trueDamage, playerTrueBonus(p));
        combatEvents.push_back({p.getName(), target.getName(), here, dealt, grenade ? CombatStyle::grenade : style, hit, true, !target.getAlive()});
        if (hit) record(p.getName() + " deals " + std::to_string(dealt) + " damage to " + target.getName() + ".");
    };
    switch (action.type) {
        case ActionType::move:
            if (!canMove(here, action.target)) return {false, "Choose an outgoing route from your sector. Some roads are one-way."};
            p.moveTo(action.target);
            record(p.getName() + " moves to " + location(action.target) + (movingFree ? " for free." : "."));
            break;
        case ActionType::attackEnemy:
        case ActionType::grenadeEnemy:
            if (enemy == enemies.end()) return {false, "That enemy is not in your sector."};
            if (action.type == ActionType::grenadeEnemy) {
                if (p.supplies[1] == 0) return {false, "No breach grenades remaining."};
                --p.supplies[1];
                strike(*enemy, true);
            } else strike(*enemy, false);
            break;
        case ActionType::attackPortal:
        case ActionType::grenadePortal:
            if (portal == portals.end()) return {false, "There is no portal in your sector."};
            if (action.type == ActionType::attackPortal && portalShielded(here)) return {false, "A Gate anchor shields this portal. Defeat the Anchor first."};
            if (action.type == ActionType::grenadePortal) {
                if (p.supplies[1] == 0) return {false, "No breach grenades remaining."};
                --p.supplies[1];
                const int remaining = portal->health; portal->health = 0;
                combatEvents.push_back({p.getName(), "Breach portal", here, remaining, CombatStyle::grenade, true, true, true});
                record("Breach grenade collapses the portal.");
            } else {
                const bool hit = !guarded || playerHits(p);
                const int dealt = hit ? std::min(portal->health, playerDamage(p,p.getWeapon()) + playerTrueBonus(p)) : 0;
                if (hit) {
                    if (!guarded) record("Unguarded portal: automatic hit.");
                    portal->health -= dealt;
                    record("Portal takes " + std::to_string(dealt) + " damage; " + std::to_string(portal->health) + " HP remains.");
                }
                combatEvents.push_back({p.getName(), "Breach portal", here, dealt, style, hit, true, portal->health == 0});
            }
            break;
        case ActionType::attackBoss:
        case ActionType::grenadeBoss:
            if (!bossHere) return {false, "The boss is not in your sector."};
            if (action.type == ActionType::grenadeBoss) {
                if (p.supplies[1] == 0) return {false, "No breach grenades remaining."};
                --p.supplies[1];
            }
            strike(*boss, action.type == ActionType::grenadeBoss);
            break;
        case ActionType::search:
            record(p.getName() + " searches " + location(here) + ".");
            grantLoot(p, here == 1 || here == 2 || here == 5);
            break;
        case ActionType::rest:
            if (p.getHealth() == p.getMaxHealth() && p.poison == 0) return {false, "You are already at full health and free of corrosion."};
            record(p.getName() + " rests and restores " + std::to_string(p.heal(here == 6 ? p.getMaxHealth() : 5)) + " HP.");
            if (here == 6) p.poison = 0;
            break;
        case ActionType::medkit:
            if (p.supplies[0] == 0) return {false, "No medkits remaining."};
            if (p.getHealth() == p.getMaxHealth() && p.poison == 0) return {false, "You do not need a medkit."};
            --p.supplies[0]; p.poison = 0;
            record(p.getName() + " uses a medkit: +" + std::to_string(p.heal(8)) + " HP, corrosion cured.");
            break;
        case ActionType::guard:
            p.guarding = true;
            record(p.getName() + " takes cover. Incoming enemy damage reduced by 3 this round.");
            break;
        case ActionType::suppress:
            if (enemy == enemies.end()) return {false, "Choose a local enemy to suppress."};
            if (weapon(p.getWeapon()).accuracy != Accuracy::precision) return {false, "Suppressing fire requires a ranged weapon."};
            if (enemy->suppressed) return {false, "That enemy is already suppressed for this round."};
            if (const bool hit = playerHits(p); hit) {
                enemy->suppressed = true;
                combatEvents.push_back({p.getName(), enemy->getName(), here, 0, CombatStyle::suppression, true, true, false});
                record(enemy->getName() + " is suppressed: it will skip its next enemy-phase action. No damage dealt.");
            } else combatEvents.push_back({p.getName(), enemy->getName(), here, 0, CombatStyle::suppression, false, true, false});
            break;
        case ActionType::wait:
            record(p.getName() + " holds position.");
            break;
        case ActionType::market: {
            if (here != 8 || action.target < 0 || action.target >= 3) return {false,"Choose an offer at the Market."};
            const auto item = marketOffers[action.target];
            if (item.isWeapon) {
                if (!p.giveWeapon(static_cast<WeaponId>(item.id))) return {false,"You already own that weapon."};
                record(p.getName() + " takes " + itemName(item) + " from the Market.");
            } else {
                if (p.supplies[item.id] >= 99) return {false,"You cannot carry more of that item."};
                grantSupply(p,static_cast<Consumable>(item.id));
            }
            break;
        }
        case ActionType::collapseBridge: {
            if (here != 9 || bridgeCollapsed) return {false,"Collapse the Bridge while standing on it."};
            if (!p.supplies[1]) return {false,"A breach grenade is required."};
            --p.supplies[1]; bridgeCollapsed = true; sectorClosures[8] = 0;
            record("BRIDGE COLLAPSED / The crossing is permanently destroyed.");
            for (auto& e : enemies) if (e.getSector() == 9) e.takeDamage(damageType::trueDamage,e.getHealth());
            for (auto& rift : portals) if (rift.sector == 9) rift.health = 0;
            if (bossHere) {
                const int dealt = boss->takeDamage(damageType::trueDamage,(boss->getHealth()+1)/2);
                boss->moveTo(7);
                combatEvents.push_back({p.getName(),boss->getName(),9,dealt,CombatStyle::grenade,true,true,!boss->getAlive()});
                record(boss->getName() + " loses " + std::to_string(dealt) + " HP and falls into the Slums.");
            }
            for (auto& defender : players) if (defender.getSector() == 9) {
                defender.takeDamage(damageType::trueDamage,defender.getHealth());
                defender.poison = 0; defender.recovering = false; defender.guarding = false;
                record(defender.getName() + " dies in the collapse. This death is permanent.");
            }
            break;
        }
        default: return {false, "Unknown action."};
    }
    cleanBattlefield(p);
    checkOutcome();
    announceLastStand(wasLastStand);
    if (outcome == Outcome::ongoing && !movingFree) finishAction();
    if (automatic) resolvePending();
    return {true, {}};
}
void Game::cleanBattlefield(Player& player) {
    for (const auto& e : enemies) if (!e.getAlive()) {
        ++kills;
        record(e.getName() + " defeated.");
        if (roll(1, 2) == 1) grantLoot(player);
    }
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& e) { return !e.getAlive(); }), enemies.end());
    for (const auto& p : portals) if (p.health <= 0) {
        ++closed;
        record("PORTAL SEALED in " + location(p.sector) + ".");
        grantSupply(player, Consumable::medkit);
    }
    portals.erase(std::remove_if(portals.begin(), portals.end(), [](const Portal& p) { return p.health <= 0; }), portals.end());
}
int Game::playerHitChance(const Player& player, WeaponId id) const {
    return std::clamp(weaponProfile(id, player).hitChance - (playerAimRounds ? 20 : 0) +
        (player.getSector() == 3 && weapon(id).accuracy == Accuracy::precision ? 20 : 0), 0, 100);
}
int Game::playerDamage(const Player& player, WeaponId id) const {
    return weapon(id).damage + (player.getSector() == 7 && weapon(id).accuracy == Accuracy::strength ? 2 : 0);
}
bool Game::freeMove(int from, int to) const {
    if (from != 4) return false;
    const auto routes = getRoutes(from);
    return std::find(routes.begin(),routes.end(),to) != routes.end();
}
void Game::refreshMarket() {
    // Independent turn seed: visiting the market or loading cannot reroll combat.
    const auto turnPlayer = playerIdx;
    std::seed_seq offerSeed {seed,static_cast<std::uint32_t>(round),static_cast<std::uint32_t>(turnPlayer),0x4D41524BU};
    std::mt19937 offerRng(offerSeed);
    std::vector<Item> pool;
    for (const auto& w : weapons()) pool.push_back({true,static_cast<int>(w.id)});
    pool.push_back({false,0}); pool.push_back({false,1});
    std::shuffle(pool.begin(),pool.end(),offerRng);
    std::copy_n(pool.begin(),3,marketOffers.begin());
}
bool Game::playerHits(const Player& player) {
    const int chance = playerHitChance(player, player.getWeapon()), die = roll(1, 100);
    const bool hit = die <= chance;
    record("  d100 " + std::to_string(die) + " / hit chance " + std::to_string(chance) + "%: " + (hit ? "HIT." : "MISS."));
    return hit;
}
std::vector<int> Game::getRoutes(int sector) const {
    auto roads = outgoingRoutes(sector, reversedRounds > 0);
    roads.erase(std::remove_if(roads.begin(), roads.end(), [this](int dest) { return getClosure(dest) > 0 || (bridgeCollapsed && dest == 9); }), roads.end());
    if (bridgeCollapsed && sector == 9) roads.clear();
    return roads;
}
int Game::routeStep(int from, int to) const {
    if (from == to) return from;
    std::array<int, 10> first {}; std::vector<int> queue {from}; first[from] = from;
    for (std::size_t i = 0; i < queue.size(); ++i) for (int dest : getRoutes(queue[i])) {
        if (first[dest]) continue;
        first[dest] = queue[i] == from ? dest : first[queue[i]];
        if (dest == to) return first[dest];
        queue.push_back(dest);
    }
    return from;
}
void Game::closeSector() {
    if (std::any_of(sectorClosures.begin(), sectorClosures.end(), [](int n) { return n > 0; })) { record("An existing cordon remains in effect."); return; }
    bool connected[10][10] {};
    for (int from=1; from<=9; ++from) for (int to=1; to<=9; ++to)
        connected[from][to] = from!=to && routeStep(from,to)!=from;
    std::vector<int> eligible;
    for (int sector : {2,3,4,5,7,8,9}) {
        if (getClosure(sector) || (bridgeCollapsed && sector == 9)) continue;
        sectorClosures[sector - 1] = 2;
        bool reachable = true;
        for (int from = 1; from <= 9; ++from) for (int to = 1; to <= 9; ++to)
            if (connected[from][to] && !getClosure(to) && routeStep(from, to) == from) reachable = false;
        sectorClosures[sector - 1] = 0;
        if (reachable) eligible.push_back(sector);
    }
    if (eligible.empty()) { record("The cordon cannot safely close another sector."); return; }
    const int sector = eligible.at(roll(0, static_cast<int>(eligible.size()) - 1)); sectorClosures[sector - 1] = 2;
    record(location(sector) + " closes to entry through the next round. Occupants may leave; all actors follow the cordon.");
}
bool Game::canMove(int from, int to) const {
    if (from < 1 || from > 9 || to < 1 || to > 9 || from == to) return false;
    if (bridgeCollapsed && (from == 9 || to == 9)) return false;
    if (transportRounds > 0) return true;
    const auto routes = getRoutes(from);
    return std::find(routes.begin(), routes.end(), to) != routes.end();
}
bool Game::portalShielded(int sector) const {
    return std::any_of(enemies.begin(), enemies.end(), [sector](const Enemy& e) {
        return e.getAlive() && e.getSector() == sector && e.definition().role == EnemyRole::anchor;
    });
}
void Game::finishAction() {
    ++playerIdx;
    while (playerIdx < players.size() && !players[playerIdx].getAlive()) ++playerIdx;
    if (playerIdx < players.size()) { refreshMarket(); return; }
    playerIdx = 0;
    while (playerIdx < players.size() && !players[playerIdx].getAlive()) ++playerIdx;
    phase = TurnPhase::worldDraw;
    activeCard = -1; enemyCursor = 0; rotResolved = false; bossResolved = false;
}
bool Game::advancePhase() {
    if (!started || outcome != Outcome::ongoing || phase == TurnPhase::players) return false;
    combatEvents.clear();
    const bool wasLastStand = isLastStand();
    switch (phase) {
        case TurnPhase::worldDraw:
            drawWorldCard(); phase = TurnPhase::worldResolve; break;
        case TurnPhase::worldResolve:
            applyWorldCard(); phase = TurnPhase::enemies;
            record("ENEMY PHASE"); break;
        case TurnPhase::enemies:
            resolveEnemyStep();
            if (rotResolved && enemyCursor >= static_cast<int>(enemies.size()) && bossResolved) phase = TurnPhase::roundEnd;
            break;
        case TurnPhase::roundEnd:
            finishRound(); phase = TurnPhase::players; break;
        case TurnPhase::players: return false;
    }
    checkOutcome();
    announceLastStand(wasLastStand);
    return true;
}
void Game::resolvePending() {
    while (outcome == Outcome::ongoing && phase != TurnPhase::players) advancePhase();
}
void Game::drawWorldCard() {
    // Quiet draws leave the event deck untouched, keeping meaningful events varied.
    if (roll(1, 2) == 1) {
        activeCard = 0;
    } else {
    if (deck.empty()) {
        deck.resize(eventCards().size() - 1);
        std::iota(deck.begin(), deck.end(), 1);
        std::shuffle(deck.begin(), deck.end(), rng);
    }
    activeCard = deck.back(); deck.pop_back();
    }
    record("WORLD EVENT / " + eventCards().at(activeCard).name);
    record(eventCards().at(activeCard).effect);
}
void Game::resolveWorld() { drawWorldCard(); applyWorldCard(); }
void Game::applyWorldCard() {
    auto randomPlayer = [&]() -> Player& {
        std::vector<std::size_t> living;
        for (std::size_t i=0; i<players.size(); ++i) if (players[i].getAlive()) living.push_back(i);
        return players.at(living.at(roll(0,static_cast<int>(living.size())-1)));
    };
    switch (activeCard) {
        case 0: record("The guns fall quiet. No additional effect."); break;
        case 1: for (auto& p : players) grantSupply(p, static_cast<Consumable>(roll(0, 1))); break;
        case 2: spawnEnemy(8, 3); break;
        case 3: spawnPortal(); break;
        case 4: reversedRounds = 2; record("ROAD REVERSAL / All one-way roads reverse through the next round. Follow the new arrows."); break;
        case 5: instability = std::max(0, instability - 1); record("Instability falls by up to 1."); break;
        case 6: doom += 2; record("Two hours vanish. Doom advances by an extra 2."); break;
        case 7: {
            const int sector = portals.empty() ? 7 : portals.at(static_cast<std::size_t>(roll(0, static_cast<int>(portals.size()) - 1))).sector;
            spawnEnemy(sector, patronEnemy(true)); break;
        }
        case 8: for (auto& p : players) if (p.getAlive()) { p.poison = 0; record(p.getName() + " recovers " + std::to_string(p.heal(4)) + " HP; corrosion cured."); } break;
        case 9: grantLoot(randomPlayer(), true); break;
        case 10:
            for (auto& p : players) grantSupply(p, Consumable::medkit);
            record("The convoy delivers medical supplies to every defender."); break;
        case 11:
            if (isLastStand()) { record("The last stand holds the invaders in place. Their rituals continue."); break; }
            for (auto& e : enemies) {
                const auto roads = getRoutes(e.getSector());
                if (!roads.empty()) e.moveTo(roads.at(static_cast<std::size_t>(roll(0, static_cast<int>(roads.size()) - 1))));
            }
            record("Every enemy follows one outgoing road. Check the changed board."); break;
        case 12: {
            std::vector<int> eligible;
            for (const auto& portal : portals) if (!portalShielded(portal.sector)) eligible.push_back(portal.sector);
            if (!eligible.empty()) spawnEnemy(eligible.at(static_cast<std::size_t>(roll(0, static_cast<int>(eligible.size()) - 1))), 4);
            else record("No unshielded portal remains. No Anchor arrives.");
            break;
        }
        case 13: spawnEnemy(7, patronEnemy()); break;
        case 14:
            if (!portals.empty()) { portals.at(roll(0, static_cast<int>(portals.size()) - 1)).health = 0; cleanBattlefield(randomPlayer()); }
            else record("No portal remains to collapse.");
            break;
        case 15:
            if (!enemies.empty()) { auto& e = enemies.at(roll(0, static_cast<int>(enemies.size()) - 1)); e.takeDamage(damageType::trueDamage, e.getHealth()); cleanBattlefield(randomPlayer()); }
            else record("No regular enemy remains in the orbital array's sights.");
            break;
        case 16: closeSector(); break;
        case 17:
            for (auto& p : players) if (p.getAlive()) {
                std::vector<int> destinations;
                for (int sector = 1; sector <= 9; ++sector) if (sector != p.getSector() && !getClosure(sector) && !(bridgeCollapsed && sector == 9)) destinations.push_back(sector);
                if (!destinations.empty()) { p.moveTo(destinations.at(roll(0, static_cast<int>(destinations.size()) - 1))); record(p.getName() + " teleports to " + location(p.getSector()) + "."); }
            }
            break;
        case 18: playerAimRounds = 2; record("ASHFALL / Defender accuracy -20 percentage points through the next round."); break;
        case 19: enemyAimRounds = 2; record("BLINDING FLARE / Enemy attack rolls -4 through the next round."); break;
        case 20:
            if (!enemies.empty()) {
                const int sector = enemies.at(roll(0, static_cast<int>(enemies.size()) - 1)).getSector();
                for (auto& e : enemies) if (e.getSector() == sector) e.takeDamage(damageType::trueDamage, e.getHealth());
                record("Artillery purges enemies in " + location(sector) + "."); cleanBattlefield(randomPlayer());
            } else record("No occupied enemy sector remains.");
            break;
        case 21:
            for (auto& p : portals) p.health = 0;
            cleanBattlefield(randomPlayer()); break;
        case 22:
            transportRounds = 2;
            record("TRANSPORT CREW / Through the next defender phase, move to any sector for one action. Flights bypass roads and cordons."); break;
        default: throw std::logic_error("No world card drawn.");
    }
    if (activeCard != 0 && roll(1, 4) == 1) { record("A supply drop reaches the squad."); grantSupply(randomPlayer(), static_cast<Consumable>(roll(0, 1))); }
}
std::vector<std::size_t> Game::targetsAt(int sector) const {
    std::vector<std::size_t> result;
    for (std::size_t i = 0; i < players.size(); ++i)
        if (players[i].getAlive() && !players[i].recovering && players[i].getSector() == sector) result.push_back(i);
    return result;
}
void Game::recover(Player& player) {
    player.moveTo(6); player.heal(); player.poison = 0; player.recovering = true; player.guarding = false;
    instability = std::min(6, instability + 1);
    record(player.getName() + " falls! Evacuated to the Infirmary at full HP, protected for this phase. Instability +1.");
}
int Game::harm(Player& player, damageType type, int damage, const std::string& source, bool infect) {
    if (!player.getAlive() || player.recovering) return 0;
    const int dealt = player.takeDamage(type, std::max(0, damage - (player.guarding && type != damageType::trueDamage ? 3 : 0)));
    record(source + " hits " + player.getName() + " for " + std::to_string(dealt) + " damage.");
    if (!player.getAlive()) recover(player);
    else if (dealt > 0 && infect) { player.poison = 2; record(player.getName() + " suffers corrosion (2 rounds)."); }
    return dealt;
}
int Game::nearestDestination(int from) const {
    int destination = from, shortest = 100;
    for (int sector = 1; sector <= 9; ++sector) {
        if (targetsAt(sector).empty()) continue;
        int distance = 0, cursor = from;
        while (cursor != sector && distance < 10) { cursor = routeStep(cursor, sector); ++distance; }
        if (cursor == sector && distance < shortest) { shortest = distance; destination = sector; }
    }
    return destination;
}
EnemyIntent Game::enemyIntent() const {
    if (!rotResolved) {
        const bool active = std::any_of(players.begin(), players.end(), [](const Player& p) { return p.getPoison() > 0 && !p.isRecovering(); });
        return {"Lingering corrosion", "Existing infections tick once before enemy actions.", 0, 0, active};
    }
    if (enemyCursor < static_cast<int>(enemies.size())) {
        const auto& e = enemies.at(enemyCursor);
        const int here = e.getSector();
        const std::string name = e.getName();
        if (e.suppressed) return {name, "Suppressed: skips its action this phase.", here, here, true};
        if (isLastStand()) return {name, "LAST STAND / Holds position and attempts to summon a portal (10% chance).", here, here, true};
        if (!targetsAt(here).empty()) return {name, "Attacks a defender in " + location(here) + ". " + std::to_string(e.getAttackDamage() + instability + (here == 7 && e.getAttackType() == damageType::physical ? 2 : 0)) + (e.getAttackType() == damageType::psychic ? " psychic" : " physical") + " damage before resistance.", here, here, true};
        if (e.definition().role == EnemyRole::hunter || e.definition().role == EnemyRole::rustHunter) {
            const int dest = nearestDestination(here);
            const int step = routeStep(here, dest);
            if (step != here) return {name, "Moves toward " + location(dest) + ". Cannot attack while moving.", here, step, true};
        }
        return {name, "Holds position.", here, here, true};
    }
    if (!bossResolved && boss && boss->getAlive()) {
        const int here = boss->getSector();
        if (!targetsAt(here).empty()) return {boss->getName(), "The avatar attacks the defenders in " + location(here) + ".", here, here, true};
        const int dest = nearestDestination(here);
        if (dest != here) return {boss->getName(), "The avatar advances toward " + location(dest) + ".", here, routeStep(here, dest), true};
        return {boss->getName(), "Holds position.", here, here, true};
    }
    return {"All clear", "No eligible targets.", 0, 0, false};
}
void Game::attackAt(int sector, int damage, damageType type, int accuracy, const std::string& source, bool infect) {
    const int attacks = patron == BossId::echoWeaver ? 2 : 1;
    for (int i = 0; i < attacks && instability < 6; ++i) {
        const auto targets = targetsAt(sector);
        if (targets.empty()) break;
        auto& p = players.at(targets.at(static_cast<std::size_t>(roll(0, static_cast<int>(targets.size()) - 1))));
        record(source + " targets " + p.getName() + ".");
        const bool hit = attackHits(accuracy - (enemyAimRounds ? 4 : 0), 12);
        const int dealt = hit ? harm(p, type, damage + instability + (sector == 7 && type == damageType::physical ? 2 : 0), source, infect) : 0;
        combatEvents.push_back({source, p.getName(), sector, dealt,
            type == damageType::psychic ? CombatStyle::psychic : CombatStyle::melee, hit, false, p.isRecovering()});
    }
}
void Game::resolveEnemyStep() {
    const auto intent = enemyIntent();
    if (!rotResolved) {
        for (auto& p : players) if (p.poison > 0 && !p.recovering) { --p.poison; harm(p, damageType::trueDamage, 1, "Corrosion", false); }
        rotResolved = true; return;
    }
    if (enemyCursor < static_cast<int>(enemies.size())) {
        auto& e = enemies.at(enemyCursor++);
        if (e.suppressed) { e.suppressed = false; record(intent.name + " cannot act: suppressed."); return; }
        if (isLastStand()) { summonPortal(e.getSector(), intent.name); return; }
        if (!targetsAt(e.getSector()).empty()) {
            attackAt(e.getSector(), e.getAttackDamage(), e.getAttackType(),
                     e.getAttackType() == damageType::psychic ? e.getWill() : e.getStrength(), intent.name,
                     patron == BossId::rustGardener || e.definition().role == EnemyRole::infector || e.definition().role == EnemyRole::rustHunter);
        } else if (intent.active && intent.destination != intent.sector) {
            e.moveTo(intent.destination); record(intent.name + " advances to " + location(e.getSector()) + ".");
        }
        return;
    }
    if (!bossResolved && boss && boss->getAlive()) {
        if (!targetsAt(boss->getSector()).empty()) {
            const int rage = patron == BossId::emberRegent && boss->getHealth() * 2 <= boss->getMaxHealth() ? 2 : 0;
            attackAt(boss->getSector(), getBossDef().damage + rage, getBossDef().type, 4, boss->getName(), patron == BossId::rustGardener);
        } else if (intent.active && intent.destination != intent.sector) {
            boss->moveTo(intent.destination); record(boss->getName() + " advances to " + location(boss->getSector()) + ". It cannot attack while moving.");
        }
    }
    bossResolved = true;
}
void Game::resolveEnemies() {
    record("ENEMY PHASE"); enemyCursor = 0; rotResolved = false; bossResolved = false;
    while (instability < 6 && (!rotResolved || enemyCursor < static_cast<int>(enemies.size()) || !bossResolved)) resolveEnemyStep();
}
void Game::finishRound() {
    doom += 1 + instability;
    record("ROUND " + std::to_string(round) + " COMPLETE / Doom +" + std::to_string(1 + instability) + " -> " + std::to_string(doom) + ".");
    if (boss && boss->getAlive()) --finalRounds;
    if (round % 5 == 0) {
        std::vector<int> preferred, occupied;
        for (const auto& portal : portals) {
            const bool defended = std::any_of(players.begin(), players.end(), [&](const Player& p) {
                return p.getAlive() && p.getSector() == portal.sector;
            });
            (defended ? occupied : preferred).push_back(portal.sector);
        }
        auto& destinations = preferred.empty() ? occupied : preferred;
        if (destinations.empty()) record("PORTAL WAVE / No open portals. No reinforcements arrive.");
        else {
            record("PORTAL WAVE / " + std::to_string(livingDefenders()) + " total reinforcements. Portals without defenders take priority.");
            // Spread a global squad-sized wave across preferred portals, reusing
            // them if necessary. Defended portals are a fallback only.
            std::shuffle(destinations.begin(), destinations.end(), rng);
            for (int i = 0; i < livingDefenders(); ++i) {
                // Preserve the previous mix: half Scavengers, half specialists.
                const int kindRoll = roll(1, 10);
                const int kind = kindRoll <= 5 ? 0 : kindRoll <= 8 ? patronEnemy(roll(0, 1) != 0) : roll(1, 4);
                spawnEnemy(destinations[i % destinations.size()], kind);
            }
        }
    }
    if (!boss && doom >= getBossDef().deadline) {
        boss.emplace(patron, static_cast<int>(players.size()));
        if (bridgeCollapsed) boss->moveTo(7);
        finalRounds = 8;
        record("MANIFESTATION / " + boss->getName() + " emerges in " + location(boss->getSector()) + ". Defeat it within 8 more rounds!");
    }
    for (int& duration : sectorClosures) if (duration > 0) --duration;
    if (playerAimRounds > 0) --playerAimRounds;
    if (enemyAimRounds > 0) --enemyAimRounds;
    if (transportRounds > 0 && --transportRounds == 0) record("Transport crew departs. Movement follows roads again.");
    for (auto& p : players) { p.recovering = false; p.guarding = false; }
    if (reversedRounds > 0 && --reversedRounds == 0) record("The streets settle. Original road directions restored.");
    ++round;
    refreshMarket();
    checkOutcome();
}
void Game::checkOutcome() {
    if (outcome != Outcome::ongoing) return;
    if (started && boss && !boss->getAlive()) {
        outcome = Outcome::bossSlain;
        ending = "THE SOVEREIGN FALLS. The avatar is slain and its invasion breaks. The city is saved.";
    } else if (started && livingDefenders() == 0) {
        outcome = Outcome::defeat;
        ending = "THE WATCH HAS FALLEN. No defender survived.";
    } else if (started && portals.empty() && enemies.empty() && !boss) {
        outcome = Outcome::banished;
        ending = "THE CITY HOLDS. Every enemy is defeated and every portal is closed. The city is saved.";
    } else if (instability >= 6) {
        outcome = Outcome::defeat;
        ending = "THE CITY FALLS. Instability reaches 6. The defenders break and the breach consumes the district.";
    } else if (boss && boss->getAlive() && finalRounds <= 0) {
        outcome = Outcome::defeat;
        ending = "THE LAST BELL. Eight rounds after manifestation, the city is consumed. The siege is lost.";
    }
    if (outcome != Outcome::ongoing) record(ending);
}
int Game::livingDefenders() const {
    return static_cast<int>(std::count_if(players.begin(),players.end(),[](const Player& p) { return p.getAlive(); }));
}
std::vector<std::string> Game::takeLog() { auto result = std::move(log); log.clear(); return result; }

void Game::save(std::ostream& out) const {
    if (!started) throw std::logic_error("Cannot save before deployment.");
    out << "VEYRA_SAVE 6\n" << static_cast<int>(patron) << ' ' << seed << ' ' << round << ' ' << doom << ' ' << instability << ' '
        << finalRounds << ' ' << nextEnemyId << ' ' << playerIdx << ' ' << kills << ' ' << closed << ' ' << static_cast<int>(outcome) << '\n';
    out << std::quoted(ending) << '\n' << players.size() << '\n';
    for (const auto& p : players) {
        out << std::quoted(p.getName()) << ' ' << p.getMaxHealth() << ' ' << p.getHealth() << ' ' << p.getBaseArmor() << ' '
            << p.getStrength() << ' ' << p.getPrecision() << ' ' << p.getBaseWill() << ' ' << p.getSector() << ' '
            << p.poison << ' ' << p.recovering << ' ' << static_cast<int>(p.equipped) << ' ' << p.arsenal.size();
        for (auto w : p.arsenal) out << ' ' << static_cast<int>(w);
        for (int count : p.supplies) out << ' ' << count;
        out << '\n';
    }
    out << enemies.size() << '\n';
    for (const auto& e : enemies) out << e.getId() << ' ' << e.getKind() << ' ' << e.getSector() << ' ' << e.getHealth() << '\n';
    out << portals.size() << '\n';
    for (const auto& p : portals) out << p.sector << ' ' << p.health << '\n';
    out << boss.has_value();
    if (boss) out << ' ' << boss->getSector() << ' ' << boss->getHealth();
    out << '\n';
    for (int i = 0; i < 9; ++i) out << 0 << ' '; // Reserved legacy board fields.
    out << '\n' << deck.size();
    for (int card : deck) out << ' ' << card;
    out << "\nTACTICS " << static_cast<int>(phase) << ' ' << activeCard << ' ' << enemyCursor << ' '
        << reversedRounds << ' ' << rotResolved << ' ' << bossResolved;
    for (const auto& p : players) out << ' ' << p.guarding;
    for (const auto& e : enemies) out << ' ' << e.suppressed;
    out << "\nWORLD " << playerAimRounds << ' ' << enemyAimRounds;
    for (int duration : sectorClosures) out << ' ' << duration;
    out << ' ' << transportRounds;
    out << "\nSECTORS " << bridgeCollapsed;
    for (const auto item : marketOffers) out << ' ' << item.isWeapon << ' ' << item.id;
    out << '\n' << rng << "\nEND\n";
    if (!out) throw std::runtime_error("Could not write the save file.");
}

Game Game::load(std::istream& in) {
    // Load into a candidate; malformed files cannot mutate the running campaign.
    auto number = [&](int low, int high) {
        long long value = 0;
        if (!(in >> value) || value < low || value > high) throw std::runtime_error("Save contains an invalid or missing number.");
        return static_cast<int>(value);
    };
    // The old marker is accepted only for importing existing local campaigns.
    std::string magic;
    if (!(in >> magic) || (magic != "VEYRA_SAVE" && magic != "FORTY_K_RPG")) throw std::runtime_error("Not a supported Veyra save.");
    const int version = number(1, 6);
    const auto patron = static_cast<BossId>(number(0, 3));
    std::uint64_t seedValue = 0;
    if (!(in >> seedValue) || seedValue > std::numeric_limits<std::uint32_t>::max()) throw std::runtime_error("Invalid save seed.");
    Game game(patron, static_cast<std::uint32_t>(seedValue));
    game.round = number(1, 10000); game.doom = number(0, 100000); game.instability = number(0, 6);
    game.finalRounds = number(0, 8); game.nextEnemyId = number(1, 100000);
    game.playerIdx = static_cast<std::size_t>(number(0, 3));
    game.kills = number(0, 100000); game.closed = number(0, 100000);
    game.outcome = static_cast<Outcome>(number(0, 3));
    if (!(in >> std::quoted(game.ending)) || game.ending.size() > 512) throw std::runtime_error("Invalid ending text.");
    const int count = number(1, 4);
    if (game.playerIdx >= static_cast<std::size_t>(count)) throw std::runtime_error("Invalid active player.");
    for (int i = 0; i < count; ++i) {
        std::string name;
        if (!(in >> std::quoted(name)) || !validName(name)) throw std::runtime_error("Invalid character name.");
        const int maxHP = number(1, 28), hp = number(version >= 6 ? 0 : 1, maxHP), armor = number(0, 4);
        const int strength = number(0, 9), precision = number(0, 9), will = number(0, 9), sector = number(1, 9);
        Player p(maxHP, armor, strength, precision, will, sector, name);
        p.takeDamage(damageType::trueDamage, maxHP - hp);
        p.poison = number(0, 2); p.recovering = number(0, 1) != 0;
        p.equipped = static_cast<WeaponId>(number(0, static_cast<int>(weapons().size()) - 1));
        p.arsenal.clear();
        const int arms = number(1, static_cast<int>(weapons().size()));
        for (int j = 0; j < arms; ++j) {
            if (!p.giveWeapon(static_cast<WeaponId>(number(0, static_cast<int>(weapons().size()) - 1)))) throw std::runtime_error("Duplicate weapon in save.");
        }
        if (!p.equip(p.equipped)) throw std::runtime_error("Equipped weapon not owned.");
        for (int& amount : p.supplies) amount = number(0, 99);
        if (version <= 2) (void)number(0, 99); // Discard retired legacy tokens from legacy saves.
        game.players.push_back(std::move(p));
    }
    std::set<int> ids;
    const int enemyCount = number(0, 64);
    for (int i = 0; i < enemyCount; ++i) {
        const int id = number(1, game.nextEnemyId - 1), kind = number(0, version == 1 ? 2 : static_cast<int>(enemyDefinitions().size()) - 1), sector = number(1, 9);
        if (!ids.insert(id).second) throw std::runtime_error("Duplicate enemy ID.");
        Enemy enemy(id, kind, sector);
        enemy.takeDamage(damageType::trueDamage, enemy.getMaxHealth() - number(1, enemy.getMaxHealth()));
        game.enemies.push_back(enemy);
    }
    std::set<int> locations;
    const int portalCount = number(0, 7);
    for (int i = 0; i < portalCount; ++i) {
        const int sector = number(2, 9), hp = number(1, 6);
        if (sector == 6 || !locations.insert(sector).second) throw std::runtime_error("Invalid portal placement.");
        game.portals.push_back({sector, hp});
    }
    if (number(0, 1) != 0) {
        game.boss.emplace(patron, count);
        game.boss->moveTo(number(1, 9));
        const std::array<int, 4> oldBase {{20, 18, 18, 24}};
        const int oldMax = oldBase[static_cast<int>(patron)] + 6 * count;
        int hp = number(0, version < 5 ? oldMax : game.boss->getMaxHealth());
        if (version < 5 && hp > 0) {
            hp = std::max(1, (hp * game.boss->getMaxHealth() + oldMax - 1) / oldMax);
        }
        game.boss->takeDamage(damageType::trueDamage, game.boss->getMaxHealth() - hp);
    }
    for (int i = 0; i < 9; ++i) (void)number(0, i == 1 || i == 7 ? 3 : 2); // Retired finite scavenging resources.
    const int cards = number(0, static_cast<int>(eventNames().size()));
    std::set<int> uniqueCards;
    for (int i = 0; i < cards; ++i) {
        const int card = number(0, static_cast<int>(eventNames().size()) - 1);
        if (!uniqueCards.insert(card).second) throw std::runtime_error("Duplicate event card.");
        game.deck.push_back(card);
    }
    if (version >= 2) {
        std::string tag;
        if (!(in >> tag) || tag != "TACTICS") throw std::runtime_error("Missing tactical state.");
        game.phase = static_cast<TurnPhase>(number(0, 4));
        game.activeCard = number(-1, static_cast<int>(eventCards().size()) - 1);
        game.enemyCursor = number(0, 64); game.reversedRounds = number(0, 2);
        game.rotResolved = number(0, 1) != 0; game.bossResolved = number(0, 1) != 0;
        if (version == 2) for (int i = 0; i < 9; ++i) (void)number(0, 3); // Retired sector wards.
        for (auto& p : game.players) p.guarding = number(0, 1) != 0;
        for (auto& e : game.enemies) e.suppressed = number(0, 1) != 0;
        const auto firstLiving = std::find_if(game.players.begin(),game.players.end(),[](const Player& p) { return p.getAlive(); });
        const auto firstIndex = firstLiving == game.players.end() ? game.playerIdx : static_cast<std::size_t>(firstLiving-game.players.begin());
        if ((game.phase != TurnPhase::players && game.playerIdx != firstIndex) ||
            ((game.phase == TurnPhase::worldResolve || game.phase == TurnPhase::enemies || game.phase == TurnPhase::roundEnd) && game.activeCard < 0) ||
            (game.phase == TurnPhase::enemies && game.enemyCursor > static_cast<int>(game.enemies.size())) ||
            (game.phase == TurnPhase::worldDraw && game.activeCard != -1))
            throw std::runtime_error("Invalid phase state.");
    }
    if (version >= 4) {
        std::string tag; if (!(in >> tag) || tag != "WORLD") throw std::runtime_error("Missing world conditions.");
        game.playerAimRounds = number(0, 2); game.enemyAimRounds = number(0, 2);
        for (int& duration : game.sectorClosures) duration = number(0, 2);
        if (version >= 5) game.transportRounds = number(0, 2);
        if (game.sectorClosures[0] || game.sectorClosures[5]) throw std::runtime_error("Protected sectors cannot close.");
    }
    if (version >= 6) {
        std::string tag; if (!(in >> tag) || tag != "SECTORS") throw std::runtime_error("Missing sector state.");
        game.bridgeCollapsed = number(0,1) != 0;
        std::set<std::pair<bool,int>> offers;
        for (auto& item : game.marketOffers) {
            item.isWeapon = number(0,1) != 0;
            item.id = number(0,item.isWeapon ? static_cast<int>(weapons().size())-1 : 1);
            if (!offers.insert({item.isWeapon,item.id}).second) throw std::runtime_error("Duplicate market offer.");
        }
        if (game.bridgeCollapsed && (game.sectorClosures[8] ||
            std::any_of(game.players.begin(),game.players.end(),[](const Player& p) { return p.getAlive() && p.getSector()==9; }) ||
            std::any_of(game.enemies.begin(),game.enemies.end(),[](const Enemy& e) { return e.getSector()==9; }) ||
            std::any_of(game.portals.begin(),game.portals.end(),[](const Portal& p) { return p.sector==9; }) ||
            (game.boss && game.boss->getSector()==9))) throw std::runtime_error("Entities cannot occupy the destroyed bridge.");
    } else game.refreshMarket();
    for (const auto& p : game.players) if (!p.getAlive() && (!game.bridgeCollapsed || p.getSector()!=9 || p.poison || p.guarding || p.recovering))
        throw std::runtime_error("Invalid fallen defender state.");
    std::string end;
    if (!(in >> game.rng >> end) || end != "END") throw std::runtime_error("Save is truncated or damaged.");
    in >> std::ws;
    if (!in.eof()) throw std::runtime_error("Unexpected content after save.");
    game.started = true;
    const Outcome savedOutcome = game.outcome;
    // Keep completed legacy victories completed under the new clear-the-board rule.
    if (version < 3 && ((savedOutcome == Outcome::banished && game.portals.empty() && !game.boss) ||
        (savedOutcome == Outcome::bossSlain && game.boss && !game.boss->getAlive()))) {
        game.enemies.clear(); game.portals.clear();
    }
    game.outcome = Outcome::ongoing;
    game.checkOutcome();
    if (game.outcome == Outcome::ongoing && !game.currentPlayer().getAlive()) throw std::runtime_error("A fallen defender cannot take a turn.");
    const bool migratedBossWin = version < 5 && game.boss && !game.boss->getAlive() && game.outcome == Outcome::bossSlain;
    if ((!migratedBossWin && savedOutcome != game.outcome) || (!game.boss && game.finalRounds != 0) ||
        (game.boss && game.doom < game.getBossDef().deadline) ||
        (game.outcome == Outcome::ongoing && !game.boss && game.doom >= game.getBossDef().deadline && game.phase == TurnPhase::players))
        throw std::runtime_error("Save has inconsistent campaign state.");
    if (game.outcome == Outcome::ongoing) game.ending.clear();
    game.log.clear();
    game.record("Campaign restored. The watch continues.");
    game.announceLastStand(false);
    return game;
}
