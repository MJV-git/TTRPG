#include "game.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

// Scenario setup is private to this test binary; the interactive game has no cheats.
struct GameTestAccess {
    static auto& players(Game& g) { return g.players; }
    static auto& enemies(Game& g) { return g.enemies; }
    static auto& portals(Game& g) { return g.portals; }
    static auto& boss(Game& g) { return g.boss; }
    static void seed(Game& g, unsigned seed) { g.rng.seed(seed); }
    static void reverse(Game& g, int rounds) { g.reversedRounds = rounds; }
    static void physical(Game& g, int damage) { g.harm(g.players[0], damageType::physical, damage, "Test attack", false); }
    static void drawCard(Game& g) { g.drawWorldCard(); }
    static void calm(Game& g) { g.deck = {0}; }
    static void nextCard(Game& g, int id) { g.deck = {id}; g.rng.seed(0); }
    static void card(Game& g, int id) { g.activeCard = id; g.record("WORLD EVENT / " + eventCards().at(id).name); g.applyWorldCard(); }
    static void instability(Game& g, int value) { g.instability = value; }
    static void doom(Game& g, int value) { g.doom = value; }
    static void endRound(Game& g) { g.finishRound(); }
    static void enemyPhase(Game& g) { g.resolveEnemies(); }
    static void harm(Game& g, std::size_t p, int amount, bool infect = false) {
        g.harm(g.players.at(p), damageType::trueDamage, amount, "Test hit", infect);
    }
    static auto targets(Game& g, int sector) { return g.targetsAt(sector); }
    static void check(Game& g) { g.checkOutcome(); }
    static void spawn(Game& g, int sector, int kind) { g.spawnEnemy(sector, kind); }
    static void manifest(Game& g) {
        g.doom = g.getBossDef().deadline;
        g.boss.emplace(g.patron, static_cast<int>(g.players.size()));
        g.finalRounds = 8;
    }
    static bool playerHit(Game& g) { return g.playerHits(g.players[0]); }
    static bool hit(Game& g, int bonus, int difficulty) { return g.attackHits(bonus, difficulty); }
};

namespace {
int assertions = 0;
void require(bool condition, const std::string& description) {
    ++assertions;
    if (!condition) throw std::runtime_error(description);
}
Game scenario(int count = 1, BossId boss = BossId::emberRegent, unsigned seed = 42) {
    Game g(boss, seed);
    for (int i = 0; i < count; ++i) require(g.addPreset(0, "Defender " + std::to_string(i + 1)), "preset creation");
    g.start(); g.takeLog();
    return g;
}
std::string snapshot(const Game& g) { std::ostringstream out; g.save(out); return out.str(); }
Game restore(const std::string& text) { std::istringstream in(text); return Game::load(in); }
void rejects(const std::string& text) {
    bool rejected = false;
    try { auto g = restore(text); (void)g; } catch (const std::exception&) { rejected = true; }
    require(rejected, "corrupt save rejected");
}
void accepted(Game& g, Action action) { const auto result = g.act(action); require(result.accepted, "action accepted: " + result.error); }
int logCount(Game& g, const std::string& needle) {
    const auto log = g.takeLog();
    return static_cast<int>(std::count_if(log.begin(), log.end(), [&](const auto& s) { return s.find(needle) != std::string::npos; }));
}
std::string offers(const Game& g) {
    std::string result;
    for (const auto item : g.getMarketOffers()) result += std::to_string(item.isWeapon)+":"+std::to_string(item.id)+" ";
    return result;
}
void trading() {
    auto g = scenario(2);
    const auto initial = snapshot(g);
    for (auto recipient : {0U, 5U}) require(!g.trade(recipient,{false,0}).accepted && snapshot(g)==initial,"invalid recipient is atomic");
    require(!g.trade(1,{true,99}).accepted && snapshot(g)==initial,"invalid item is atomic");
    require(!g.trade(1,{true,static_cast<int>(WeaponId::rotaryRiveter)}).accepted && snapshot(g)==initial,"duplicate weapon cannot transfer");
    GameTestAccess::players(g)[1].moveTo(2);
    const auto apart = snapshot(g);
    require(!g.trade(1,{false,0}).accepted && snapshot(g)==apart,"trade requires same sector");
    GameTestAccess::players(g)[1].moveTo(1);
    auto& giver = GameTestAccess::players(g)[0];
    giver.giveWeapon(WeaponId::sword); g.equip(WeaponId::sword);
    require(g.trade(1,{true,static_cast<int>(WeaponId::sword)}).accepted,"weapon transfer");
    require(giver.getWeapon()!=WeaponId::sword && std::find(g.getPlayers()[1].getWeapons().begin(),g.getPlayers()[1].getWeapons().end(),WeaponId::sword)!=g.getPlayers()[1].getWeapons().end(),"equipped transfer selects remaining weapon");
    const auto oldOffers = offers(g);
    require(g.trade(1,{false,1}).accepted && giver.getSupply(Consumable::grenade)==0 && g.getPlayers()[1].getSupply(Consumable::grenade)==2,"one grenade transfers");
    const auto empty = snapshot(g);
    require(!g.trade(1,{false,1}).accepted && snapshot(g)==empty,"empty supply rejected without mutation");
    require(g.getPlayerIndex()==0 && g.getRound()==1 && offers(g)==oldOffers,"trading spends no turn or market refresh");
    require(snapshot(restore(snapshot(g)))==snapshot(g),"traded inventories persist");
    auto single = scenario(2);
    GameTestAccess::players(single)[0] = Player(16,3,3,3,2,1,"Single weapon");
    const auto last = snapshot(single);
    require(!single.trade(1,{true,0}).accepted && snapshot(single)==last,"last weapon cannot transfer");
    require(g.act({ActionType::wait},false).accepted && g.act({ActionType::wait},false).accepted,"end trading turn");
    const auto phased = snapshot(g);
    require(!g.trade(1,{false,0}).accepted && snapshot(g)==phased,"world phase blocks trading");
}
void sectorBonuses() {
    auto g = scenario(2); auto& p = GameTestAccess::players(g)[0];
    const int armor=p.getBaseArmor(), will=p.getBaseWill();
    require(p.getArmor()==armor+2,"Barracks armor bonus");
    require(p.takeDamage(damageType::physical,10)==10-armor-2,"Barracks resists actual damage"); p.heal();
    p.moveTo(5);
    require(p.getArmor()==armor && p.getWill()==will+4,"sector bonuses move with defender");
    require(p.takeDamage(damageType::psychic,10)==std::max(0,10-will-4),"Cathedral resists psychic attacks"); p.heal();
    require(weaponProfile(WeaponId::staff,p).hitChance > weaponProfile(WeaponId::staff,Entity(16,armor,3,3,will,2,"Base")).hitChance,"Cathedral Will improves psychic accuracy");
    require(snapshot(restore(snapshot(g)))==snapshot(g),"effective Will does not accumulate in saves");
    p.moveTo(1); require(snapshot(restore(snapshot(g)))==snapshot(g),"effective armor does not accumulate in saves");
    p.moveTo(3); require(g.playerHitChance(p,WeaponId::needleRifle)==80,"Alley adds twenty points to ranged accuracy");
    const auto meleeChance=g.playerHitChance(p,WeaponId::sword); p.moveTo(7);
    require(g.playerHitChance(p,WeaponId::sword)==meleeChance,"Alley does not alter melee accuracy");
    require(g.playerDamage(p,WeaponId::sword)==weapon(WeaponId::sword).damage+2 && g.playerDamage(p,WeaponId::rivetRifle)==weapon(WeaponId::rivetRifle).damage,"Slums bonus affects melee only");
    for (int sector : {2,7}) for (unsigned seed=0; seed<20; ++seed) {
        auto fight=scenario(2); auto& fighter=GameTestAccess::players(fight)[0]; fighter.moveTo(sector);
        fighter.giveWeapon(WeaponId::sword); fight.equip(sector==7 ? WeaponId::sword : WeaponId::pistol);
        GameTestAccess::enemies(fight).clear(); GameTestAccess::spawn(fight,sector,1); GameTestAccess::seed(fight,seed);
        const auto target=fight.getEnemies()[0];
        require(fight.act({ActionType::attackEnemy,target.getId()},false).accepted,"sector attack");
        const auto beat=fight.getCombatEvents()[0];
        const int expected=std::min(target.getHealth(),std::max(0,fight.playerDamage(fighter,fighter.getWeapon())-target.getArmor())+(sector==2 ? 5 : 0));
        require(beat.damage==(beat.hit ? expected : 0),"sector damage applies after armor and only on hits");
    }
    auto grenade=scenario(2); GameTestAccess::players(grenade)[0].moveTo(2); GameTestAccess::enemies(grenade).clear(); GameTestAccess::spawn(grenade,2,1);
    require(grenade.act({ActionType::grenadeEnemy,grenade.getEnemies()[0].getId()},false).accepted && grenade.getCombatEvents()[0].damage==10,"Armory adds true damage to grenades");
    int physicalHits=0;
    for (unsigned seed=0; seed<20; ++seed) {
        auto enemy=scenario(); GameTestAccess::players(enemy)[0].moveTo(7); GameTestAccess::enemies(enemy).clear(); GameTestAccess::spawn(enemy,7,0); GameTestAccess::seed(enemy,seed);
        const int expected=std::max(0,enemy.getEnemies()[0].getAttackDamage()+2-enemy.currentPlayer().getArmor());
        GameTestAccess::enemyPhase(enemy);
        if (enemy.getCombatEvents()[0].hit) { ++physicalHits; require(enemy.getCombatEvents()[0].damage==expected,"enemy melee also gains Slums damage"); }
    }
    require(physicalHits>0,"enemy melee sector bonus exercised");
}
void marketAndFreeMoves() {
    auto g=scenario(2); GameTestAccess::players(g)[0].moveTo(4);
    const auto old=offers(g); const auto original=snapshot(g);
    require(g.act({ActionType::move,8},false).accepted && g.getPlayerIndex()==0 && g.getPhase()==TurnPhase::players && offers(g)==old,"Street exit is free and preserves market");
    auto loaded=restore(snapshot(g)); require(offers(loaded)==old,"reload cannot reroll market");
    std::set<std::pair<bool,int>> unique;
    for (const auto item:g.getMarketOffers()) unique.insert({item.isWeapon,item.id});
    require(unique.size()==3,"three distinct offers");
    int choice=-1;
    for (int i=0;i<3;++i) { const auto item=g.getMarketOffers()[i]; if (!item.isWeapon || std::find(g.currentPlayer().getWeapons().begin(),g.currentPlayer().getWeapons().end(),static_cast<WeaponId>(item.id))==g.currentPlayer().getWeapons().end()) {choice=i;break;} }
    require(choice>=0,"initial defender has a usable offer");
    const auto item=g.getMarketOffers()[choice]; const int prior=item.isWeapon ? 0 : g.currentPlayer().getSupply(static_cast<Consumable>(item.id));
    require(g.act({ActionType::market,choice},false).accepted && g.getPlayerIndex()==1,"market choice spends action");
    const auto& buyer=g.getPlayers()[0];
    require(item.isWeapon ? std::find(buyer.getWeapons().begin(),buyer.getWeapons().end(),static_cast<WeaponId>(item.id))!=buyer.getWeapons().end() : buyer.getSupply(static_cast<Consumable>(item.id))==prior+1,"chosen item granted");
    require(offers(g)!=old,"offers refresh for next defender");
    auto invalid=snapshot(g); require(!g.act({ActionType::market,0},false).accepted && snapshot(g)==invalid,"market requires presence");
    auto flight=restore(original); GameTestAccess::card(flight,22);
    require(flight.act({ActionType::move,6},false).accepted && flight.getPlayerIndex()==1,"Transport flight from Streets to non-neighbor costs action");
    auto reversed=restore(original); GameTestAccess::reverse(reversed,2);
    require(reversed.freeMove(4,3) && !reversed.freeMove(4,7),"free moves follow current reversed roads");
    auto owned=scenario(2); GameTestAccess::players(owned)[0].moveTo(8);
    for (int i=0;i<3;++i) if (owned.getMarketOffers()[i].isWeapon) {
        GameTestAccess::players(owned)[0].giveWeapon(static_cast<WeaponId>(owned.getMarketOffers()[i].id));
        const auto before=snapshot(owned); require(!owned.act({ActionType::market,i},false).accepted && snapshot(owned)==before,"owned market offer spends no action");
    }
}
void bridgeSacrifice() {
    auto g=scenario(4); const auto before=snapshot(g);
    require(!g.act({ActionType::collapseBridge},false).accepted && snapshot(g)==before,"bridge collapse requires local defender");
    GameTestAccess::players(g)[0].moveTo(9); GameTestAccess::players(g)[1].moveTo(9); GameTestAccess::players(g)[2].moveTo(7);
    GameTestAccess::enemies(g).clear(); GameTestAccess::spawn(g,9,1); GameTestAccess::spawn(g,7,0);
    GameTestAccess::portals(g)={{9,6},{8,6}}; GameTestAccess::manifest(g);
    GameTestAccess::boss(g)->takeDamage(damageType::trueDamage,9); const int oldHP=g.getBoss()->getHealth();
    require(g.act({ActionType::collapseBridge},false).accepted,"collapse accepted");
    require(g.isBridgeCollapsed() && g.livingDefenders()==2 && !g.getPlayers()[0].getAlive() && !g.getPlayers()[1].getAlive(),"all bridge defenders die permanently");
    require(g.getPlayerIndex()==2 && g.getPhase()==TurnPhase::players && g.getInstability()==0,"fallen defenders skipped without ordinary recovery");
    require(g.getBoss()->getSector()==7 && g.getBoss()->getHealth()==oldHP/2,"boss loses rounded-up half current HP and falls into Slums");
    require(g.getKills()==1 && g.getClosed()==1 && g.getEnemies().size()==1 && g.getPortals().size()==1,"bridge enemies and portals destroyed");
    require(g.getRoutes(9).empty() && g.getRoutes(7).empty(),"collapsed bridge traps normal Slums exits");
    require(snapshot(restore(snapshot(g)))==snapshot(g),"collapsed bridge and deaths persist");
    require(!g.trade(0,{false,0}).accepted,"fallen defender cannot receive trades");
    GameTestAccess::card(g,8); GameTestAccess::card(g,17);
    require(!g.getPlayers()[0].getAlive() && !g.getPlayers()[1].getAlive() && g.getPlayers()[0].getSector()==9,"world healing and transport cannot revive or move corpses");
    GameTestAccess::harm(g,0,1); require(!g.getPlayers()[0].getAlive(),"later damage cannot revive a permanent casualty");
    GameTestAccess::card(g,22);
    for(int from=1;from<=9;++from) require(!g.canMove(from,9),"Transport cannot enter destroyed bridge");
    for(int i=0;i<15;++i) GameTestAccess::card(g,3);
    require(std::none_of(g.getPortals().begin(),g.getPortals().end(),[](const Portal& r){return r.sector==9;}),"world rifts never reopen destroyed bridge");
    require(g.act({ActionType::wait},false).accepted && g.act({ActionType::wait},false).accepted && g.getPlayerIndex()==2,"round wraps to first surviving defender");
    require(snapshot(restore(snapshot(g)))==snapshot(g),"mid-phase collapsed campaign reloads");

    // Collapse is local: damage and relocation must never affect a sovereign elsewhere.
    for (int patron=0;patron<4;++patron) for (int sector : {7,8,9}) for (int hp : {50,51}) {
        auto local=scenario(2,static_cast<BossId>(patron));
        GameTestAccess::players(local)[0].moveTo(9); GameTestAccess::manifest(local);
        GameTestAccess::boss(local)->moveTo(sector);
        GameTestAccess::boss(local)->takeDamage(damageType::trueDamage,local.getBoss()->getHealth()-hp);
        const auto grenade=local.currentPlayer().getSupply(Consumable::grenade);
        require(local.act({ActionType::collapseBridge},false).accepted,"bridge collapse for each sovereign and location");
        require(local.getBoss()->getHealth()==(sector==9 ? hp/2 : hp),"only sovereign on Bridge loses half current HP regardless of resistance");
        require(local.getBoss()->getSector()==(sector==9 ? 7 : sector),"only sovereign on Bridge is relocated to Slums");
        require(local.getPlayers()[0].getSupply(Consumable::grenade)==grenade-1 && local.getPlayerIndex()==1,"collapse consumes one grenade and ends actor's turn");
        const auto collapsed=snapshot(local);
        require(snapshot(restore(collapsed))==collapsed,"all sovereign locations and half-health results persist");
        require(!local.act({ActionType::collapseBridge},false).accepted && snapshot(local)==collapsed,"Bridge cannot be destroyed twice");
    }
    auto noGrenade=scenario(2); GameTestAccess::players(noGrenade)[0].moveTo(9); GameTestAccess::players(noGrenade)[1].moveTo(9);
    require(noGrenade.trade(1,{false,1}).accepted,"give away last grenade");
    const auto unarmed=snapshot(noGrenade);
    require(!noGrenade.act({ActionType::collapseBridge},false).accepted && snapshot(noGrenade)==unarmed,"collapse without a grenade changes nothing");

    auto alone=scenario(); GameTestAccess::players(alone)[0].moveTo(9);
    require(alone.act({ActionType::collapseBridge},false).accepted && alone.getOutcome()==Outcome::defeat,"whole squad sacrifice ends campaign");
    require(snapshot(restore(snapshot(alone)))==snapshot(alone),"permanent defeat saves");
    auto god=scenario(); GameTestAccess::players(god)[0].moveTo(9); GameTestAccess::manifest(god); GameTestAccess::boss(god)->takeDamage(damageType::trueDamage,god.getBoss()->getHealth()-1);
    require(god.act({ActionType::collapseBridge},false).accepted && god.getOutcome()==Outcome::bossSlain,"boss death wins even during final defender sacrifice");
    require(snapshot(restore(snapshot(god)))==snapshot(god),"sacrificial boss victory saves");
    auto early=scenario(2); GameTestAccess::players(early)[0].moveTo(9);
    require(early.act({ActionType::collapseBridge},false).accepted,"early collapse");
    GameTestAccess::doom(early,early.getBossDef().deadline); GameTestAccess::endRound(early);
    require(early.getBoss()->getSector()==7,"boss manifests in Slums after early collapse");
    auto wave=scenario(3); GameTestAccess::players(wave)[0].moveTo(9);
    require(wave.act({ActionType::collapseBridge},false).accepted,"wave sacrifice");
    GameTestAccess::portals(wave)={{8,6}}; GameTestAccess::enemies(wave).clear();
    for (int round=1;round<=5;++round) GameTestAccess::endRound(wave);
    require(wave.getEnemies().size()==2,"portal wave scales to surviving defenders");
    auto legacy=scenario(); auto bytes=snapshot(legacy); bytes.replace(0,bytes.find('\n')+1,"FORTY_K_RPG 5\n");
    const auto at=bytes.find("SECTORS "); bytes.erase(at,bytes.find('\n',at)-at+1);
    require(snapshot(restore(bytes))==snapshot(legacy),"v5 saves migrate with stable market and intact bridge");
}

void damage() {
    Entity e(10, 4, 1, 1, 2, 1, "Target");
    require(e.takeDamage(damageType::physical, 2) == 0 && e.getHealth() == 10, "armor cannot heal");
    require(e.takeDamage(damageType::psychic, 5) == 3 && e.getHealth() == 7, "Will resists psychic");
    require(e.takeDamage(damageType::trueDamage, 2) == 2, "true damage bypasses armor");
    require(e.takeDamage(damageType::physical, -1) == 0, "negative damage ignored");
    require(e.takeDamage(damageType::physical, 100) == 5 && !e.getAlive(), "death clamps HP");
    e.heal(); require(e.getAlive() && e.getHealth() == 10, "healing revives");
    require(e.heal(-10) == 0 && e.heal(10) == 0, "heal is bounded");
}
void mapRoutes() {
    require(sectors().size() == 9, "nine sectors");
    const std::vector<std::vector<int>> exits {
        {2, 6, 5}, {1, 3}, {4}, {5, 7, 8}, {6, 7, 4}, {1, 5}, {9}, {4, 3, 9}, {7, 8}
    };
    for (const auto& sector : sectors()) {
        for (int destination = 1; destination <= 9; ++destination) {
            const auto& allowed = exits.at(sector.id - 1);
            const bool expected = std::find(allowed.begin(), allowed.end(), destination) != allowed.end();
            require(sector.checkConnecting(destination) == expected, "original directed access preserved");
        }
        for (int destination = 1; destination <= 9; ++destination) {
            int cursor = sector.id, steps = 0;
            while (cursor != destination && steps < 10) {
                const int next = nextStep(cursor, destination);
                require(sectorAt(cursor).checkConnecting(next), "path uses real road");
                cursor = next; ++steps;
            }
            require(cursor == destination, "all sectors reachable");
        }
    }
    require(nextStep(7, 5) == 9, "Slums must go through Bridge to return to Cathedral");
    require(nextStep(3, 2) == 4, "Alley cannot reverse directly into Armory");
    require(nextStep(5, 1) == 6, "Cathedral must return to Barracks via Infirmary");
}
void directedMovement() {
    auto g = scenario(2);
    GameTestAccess::players(g)[0].moveTo(3);
    const auto before = snapshot(g);
    require(!g.act({ActionType::move, 2}).accepted, "Alley to Armory forbidden");
    require(!g.act({ActionType::move, 8}).accepted, "Alley to Market forbidden");
    require(snapshot(g) == before, "forbidden reverse moves spend no action or randomness");
    accepted(g, {ActionType::move, 4});
    require(g.getPlayers()[0].getSector() == 4 && g.getPlayerIndex() == 1, "outgoing move succeeds and spends action");

    auto pursuit = scenario();
    GameTestAccess::enemies(pursuit).clear();
    GameTestAccess::manifest(pursuit);
    GameTestAccess::boss(pursuit)->moveTo(7);
    GameTestAccess::players(pursuit)[0].moveTo(5);
    GameTestAccess::enemyPhase(pursuit);
    require(pursuit.getBoss()->getSector() == 9, "boss follows directed detour instead of reversing a one-way road");

    auto march = scenario();
    GameTestAccess::enemies(march).clear();
    for (int sector = 1; sector <= 9; ++sector) GameTestAccess::spawn(march, sector, 0);
    for (int turn = 0; turn < 20; ++turn) {
        const auto previous = march.getEnemies();
        GameTestAccess::card(march, 11);
        for (std::size_t i = 0; i < previous.size(); ++i)
            require(sectorAt(previous[i].getSector()).checkConnecting(march.getEnemies()[i].getSector()),
                    "world-event movement follows outgoing routes");
    }
}
void creation() {
    Game g;
    require(!g.addCustom(0, 4, 4, 4, 4, "No HP"), "zero vitality rejected");
    require(!g.addCustom(4, -1, 4, 4, 4, "Negative"), "negative stats rejected");
    require(!g.addCustom(9, 4, 9, 9, 9, "Over budget"), "budget enforced");
    require(!g.addCustom(4, 5, 0, 0, 0, "Armor"), "armor cap");
    require(!g.addCustom(2147483647, 4, 4, 4, 4, "Overflow"), "huge stats rejected safely");
    require(!g.addPreset(0, "\033[2J"), "terminal controls rejected");
    require(!g.addPreset(0, "   "), "blank name rejected");
    require(g.addCustom(4, 4, 4, 4, 4, "Jeff"), "original 50-point build works");
    require(g.currentPlayer().getHealth() == 13, "vitality conversion");
    require(g.currentPlayer().getWeapon() == WeaponId::beamCarbine, "custom default starting weapon");
    for (int i = 0; i < 3; ++i) require(g.addPreset(i, "Preset"), "preset");
    require(!g.addPreset(0, "Fifth"), "four-player maximum");
    g.start(); require(!g.addPreset(0, "Late"), "setup closed after start");
    bool repeated = false;
    try { g.start(); } catch (const std::logic_error&) { repeated = true; }
    require(repeated, "cannot reset started game by starting twice");
    Game empty;
    bool refused = false;
    try { empty.start(); } catch (const std::logic_error&) { refused = true; }
    require(refused, "empty campaign rejected");
}
void setup() {
    for (auto boss : {BossId::emberRegent, BossId::prismOracle, BossId::echoWeaver, BossId::rustGardener}) for (int squad = 1; squad <= 4; ++squad) {
        auto g = scenario(squad, boss);
        require(g.getEnemies().size() == static_cast<std::size_t>(2 * squad), "two starting enemies per defender for every boss and squad size");
        require(std::count_if(g.getEnemies().begin(), g.getEnemies().end(), [](const Enemy& e) { return e.getKind() == 4; }) == squad,
            "starting wave includes one Anchor per defender");
        for (const auto& enemy : g.getEnemies()) {
            require(enemy.getKind() == 4 || enemy.getKind() == 7 + 2 * static_cast<int>(boss), "starting mix retains patron constructs and Anchors");
            require(std::any_of(g.getPortals().begin(), g.getPortals().end(), [&](const Portal& p) { return p.sector == enemy.getSector(); }),
                "all initial enemies spawn at open portals");
        }
        require(g.getPortals().size() == (boss == BossId::prismOracle ? 4U : 2U), "boss portal count");
        std::set<int> places;
        for (const auto& p : g.getPortals()) {
            require(p.sector != 1 && p.sector != 6, "safe starting sectors");
            require(places.insert(p.sector).second, "unique portal locations");
        }
    }
}
void turnOrderAndInvalidActions() {
    auto g = scenario(2);
    GameTestAccess::calm(g);
    const std::string initial = snapshot(g);
    for (Action a : {Action{ActionType::move, 9}, {ActionType::attackEnemy, 999}, {ActionType::attackPortal},
                     {ActionType::attackBoss}, {ActionType::medkit},
                     {ActionType::rest}, {ActionType::grenadeEnemy, 99}, {static_cast<ActionType>(999)}})
        require(!g.act(a).accepted && snapshot(g) == initial, "invalid action changes no state or RNG");
    require(!g.equip(WeaponId::staff), "cannot equip unowned weapon");
    require(g.equip(WeaponId::pistol) && g.getPlayerIndex() == 0 && g.getDoom() == 0, "equip is free");
    accepted(g, {ActionType::move, 2});
    require(g.getPlayerIndex() == 1 && g.getRound() == 1 && g.getDoom() == 0, "first action does not advance round");
    require(logCount(g, "WORLD EVENT") == 0, "no world card until all act");
    accepted(g, {ActionType::wait});
    require(g.getPlayerIndex() == 0 && g.getRound() == 2 && g.getDoom() == 1, "one round for two actions");
    require(logCount(g, "WORLD EVENT") == 1, "exactly one shared card");
}
void accuracy() {
    Player weak(10, 0, 1, 5, 2, 1, "Weak");
    require(weaponProfile(WeaponId::rivetRifle, weak).hitChance == 95, "ordinary ranged weapon rewards surplus Precision without requiring Strength");
    require(weaponProfile(WeaponId::sword, weak).hitChance == 60, "one unmet Strength requirement costs twenty points");
    require(weaponProfile(WeaponId::staff, weak).hitChance == 60 && weaponProfile(WeaponId::staff, weak).type == damageType::psychic, "unmet Will reduces accuracy rather than changing damage type");
    Player untrained(10, 0, 0, 0, 0, 1, "Untrained");
    require(weaponProfile(WeaponId::rotaryRiveter, untrained).hitChance == 40, "two unmet requirements cost forty points regardless of missing stat count");
    require(weaponProfile(WeaponId::sword, untrained).hitChance == 60, "melee ignores Precision");
    Player trained(10, 0, 4, 2, 3, 1, "Trained");
    require(weaponProfile(WeaponId::rotaryRiveter, trained).hitChance == 80, "exact requirements give eighty percent");
    require(weaponProfile(WeaponId::sword, trained).hitChance == 90, "surplus Strength adds five points each");
    auto g = scenario();
    std::mt19937 expected(123);
    // Reset the test RNG independently of campaign setup.
    Game rollGame(BossId::emberRegent, 123);
    for (int i = 0; i < 200; ++i) {
        const int die = std::uniform_int_distribution<int>(1, 20)(expected);
        const int bonus = i % 2 == 0 ? 100 : -100;
        require(GameTestAccess::hit(rollGame, bonus, 12) == (die == 20 || (die != 1 && die + bonus >= 12)), "natural 1/20 and threshold");
    }
}
void portalsAndLoot() {
    auto g = scenario(2);
    GameTestAccess::players(g)[0].moveTo(2);
    GameTestAccess::portals(g) = {{2, 6}, {9, 6}};
    GameTestAccess::enemies(g).clear();
    const int meds = g.currentPlayer().getSupply(Consumable::medkit);
    accepted(g, {ActionType::attackPortal});
    require(g.getClosed() == 1 && g.getPortals().size() == 1, "unguarded portal automatically hit with rotary riveter");
    require(g.getPlayers()[0].getSupply(Consumable::medkit) == meds + 1, "portal reward");
    require(g.getPlayerIndex() == 1, "portal attack uses action");
    auto win = scenario(2);
    GameTestAccess::players(win)[0].moveTo(2);
    GameTestAccess::portals(win) = {{2, 6}};
    GameTestAccess::enemies(win).clear();
    accepted(win, {ActionType::grenadePortal});
    require(win.getOutcome() == Outcome::banished, "last portal wins only on an otherwise empty battlefield");
    require(win.getRound() == 1 && win.getDoom() == 0, "victory immediate without extra enemy phase");
    require(!win.act({ActionType::wait}).accepted, "ended game rejects action");
    auto restored = restore(snapshot(win));
    require(restored.getOutcome() == Outcome::banished, "completed victory saves load");
}
void itemsAndSearch() {
    auto g = scenario(2);
    GameTestAccess::enemies(g).clear();
    GameTestAccess::spawn(g, 1, 1);
    const int id = g.getEnemies().front().getId();
    accepted(g, {ActionType::grenadeEnemy, id});
    require(g.getKills() == 0 && g.getEnemies().front().getHealth() == 5, "grenade deals exactly five damage through enemy armor");
    require(g.getPlayers()[0].getSupply(Consumable::grenade) <= 1, "grenade consumed (loot may replace it)");
    auto searching = scenario(2); GameTestAccess::calm(searching);
    for (int i = 0; i < 5; ++i) { GameTestAccess::calm(searching); accepted(searching, {ActionType::search}); }
    require(searching.getPlayers()[0].getWeapons().size() >= 3, "search grants loot without finite resource counters");
    auto healing = scenario(2);
    GameTestAccess::harm(healing, 0, 10, true);
    GameTestAccess::instability(healing, 3);
    accepted(healing, {ActionType::medkit});
    require(healing.getPlayers()[0].getHealth() == 14 && healing.getPlayers()[0].getPoison() == 0, "medkit heals 8 and cures corrosion");
    require(healing.getInstability() == 3, "medkit does not reduce instability");
    GameTestAccess::players(healing)[1].moveTo(2);
    GameTestAccess::portals(healing) = {{2,6}, {9,6}};
    GameTestAccess::enemies(healing).clear();
    require(healing.act({ActionType::grenadePortal}, false).accepted && healing.getInstability() == 3, "closing a portal cannot lower instability");
    GameTestAccess::card(healing, 5);
    require(healing.getInstability() == 2, "only a world event lowers instability");
}
void recoveryAndRetargeting() {
    auto g = scenario(2);
    GameTestAccess::enemies(g).clear();
    GameTestAccess::harm(g, 0, 100);
    require(g.getPlayers()[0].getSector() == 6 && g.getPlayers()[0].getHealth() == 16, "immediate infirmary recovery");
    require(g.getInstability() == 1 && g.getPlayers()[0].isRecovering(), "recovery adds instability and immunity");
    require(GameTestAccess::targets(g, 6).empty(), "recovered player not targetable in infirmary");
    const auto eligible = GameTestAccess::targets(g, 1);
    require(eligible.size() == 1 && eligible.front() == 1, "subsequent enemies target remaining player");
    GameTestAccess::spawn(g, 6, 2);
    GameTestAccess::enemyPhase(g);
    require(g.getPlayers()[0].getHealth() == 16 && g.getInstability() == 1, "infirmary enemy cannot hit recovered player");
    GameTestAccess::harm(g, 0, 100);
    require(g.getInstability() == 1, "no repeated knockout in phase");
    GameTestAccess::endRound(g);
    require(!g.getPlayers()[0].isRecovering(), "immunity expires after phase");
    require(GameTestAccess::targets(g, 6).size() == 1, "player eligible next round");
}
void wavesAndDoom() {
    auto g = scenario();
    GameTestAccess::instability(g, 2);
    for (int i = 0; i < 4; ++i) GameTestAccess::endRound(g);
    require(g.getEnemies().size() == 2 && g.getDoom() == 12, "instability advances clock but not wave cadence");
    GameTestAccess::endRound(g);
    require(g.getPortals().size() == 2 && g.getEnemies().size() == 3, "solo wave adds one enemy globally despite two portals");
    require(g.getDoom() == 15 && g.getRound() == 6, "one round advancement");
    GameTestAccess::card(g, 6);
    require(g.getDoom() == 17, "Lost Hours adds exactly two doom");
    const std::array<int,7> sectors {{2,3,4,5,7,8,9}};
    for (int squad = 1; squad <= 4; ++squad) for (int portalCount : {1,2,4,7}) {
        auto wave = scenario(squad);
        GameTestAccess::enemies(wave).clear(); GameTestAccess::portals(wave).clear();
        for (int i = 0; i < portalCount; ++i) GameTestAccess::portals(wave).push_back({sectors[i],6});
        for (int i = 0; i < 4; ++i) GameTestAccess::endRound(wave);
        require(wave.getEnemies().empty(), "portal waves wait until the fifth round");
        const auto resumed = snapshot(wave); auto copy = restore(resumed);
        GameTestAccess::endRound(wave); GameTestAccess::endRound(copy);
        require(wave.getEnemies().size() == static_cast<std::size_t>(squad), "global wave scales to one enemy per defender for every portal count");
        require(snapshot(wave) == snapshot(copy), "wave locations and enemy types are deterministic after loading");
        std::map<int,int> counts;
        for (int i = 0; i < portalCount; ++i) counts[sectors[i]] = 0;
        for (const auto& enemy : wave.getEnemies()) {
            require(counts.count(enemy.getSector()) == 1, "wave spawns only at an open portal");
            ++counts[enemy.getSector()];
        }
        for (const auto& entry : counts) require(entry.second >= squad / portalCount && entry.second <= (squad + portalCount - 1) / portalCount,
            "wave spreads evenly across equally preferred portals");
    }
    for (int squad = 1; squad <= 4; ++squad) {
        auto preferred = scenario(squad);
        GameTestAccess::portals(preferred) = {{2,6},{8,6}}; GameTestAccess::enemies(preferred).clear();
        for (auto& p : GameTestAccess::players(preferred)) p.moveTo(2);
        for (int i = 0; i < 5; ++i) GameTestAccess::endRound(preferred);
        require(preferred.getEnemies().size() == static_cast<std::size_t>(squad), "priority does not change global spawn budget");
        for (const auto& enemy : preferred.getEnemies()) require(enemy.getSector() == 8, "undefended portal takes priority for the whole wave");
    }
    auto occupied = scenario(4);
    GameTestAccess::portals(occupied) = {{2,6},{8,6}}; GameTestAccess::enemies(occupied).clear();
    GameTestAccess::players(occupied)[0].moveTo(2); GameTestAccess::players(occupied)[1].moveTo(8);
    for (int i = 0; i < 5; ++i) GameTestAccess::endRound(occupied);
    require(occupied.getEnemies().size() == 4 && std::count_if(occupied.getEnemies().begin(), occupied.getEnemies().end(),
        [](const Enemy& e) { return e.getSector() == 2; }) == 2, "all-defended portals still receive an evenly shared global wave");
    auto empty = scenario(4); GameTestAccess::portals(empty).clear();
    const auto existing = empty.getEnemies().size();
    for (int i = 0; i < 5; ++i) GameTestAccess::endRound(empty);
    require(empty.getEnemies().size() == existing, "no open portals means no wave spawns");
}
void bossRules() {
    auto s = scenario(1, BossId::echoWeaver);
    GameTestAccess::enemies(s).clear(); GameTestAccess::spawn(s, 1, 0); s.takeLog();
    GameTestAccess::enemyPhase(s);
    require(logCount(s, "targets Defender") == 2, "Echo Weaver extra attack");
    auto n = scenario(2, BossId::rustGardener);
    GameTestAccess::enemies(n).clear();
    GameTestAccess::harm(n, 0, 2, true);
    require(n.getPlayers()[0].getPoison() == 2, "infection");
    GameTestAccess::enemyPhase(n);
    require(n.getPlayers()[0].getPoison() == 1 && n.getPlayers()[0].getHealth() == 13, "corrosion ticks once");
    GameTestAccess::enemyPhase(n);
    require(n.getPlayers()[0].getPoison() == 0 && n.getPlayers()[0].getHealth() == 12, "corrosion expires after two ticks");
    auto boss = scenario();
    GameTestAccess::enemies(boss).clear();
    GameTestAccess::doom(boss, boss.getBossDef().deadline - 1);
    GameTestAccess::endRound(boss);
    require(boss.getBoss().has_value() && boss.getFinalRounds() == 8, "manifestation at deadline with full countdown");
    require(boss.getBoss()->getSector() == 9, "boss emerges at Bridge");
    GameTestAccess::enemyPhase(boss);
    require(boss.getBoss()->getSector() == nextStep(9, 1) && boss.currentPlayer().getHealth() == 16, "boss walks without attacking");
    for (int i = 0; i < 7; ++i) GameTestAccess::endRound(boss);
    require(boss.getOutcome() == Outcome::ongoing && boss.getFinalRounds() == 1, "seven final rounds still playable");
    GameTestAccess::endRound(boss);
    require(boss.getOutcome() == Outcome::defeat, "eighth final round loses");
    require(restore(snapshot(boss)).getOutcome() == Outcome::defeat, "defeat save loads");
    auto slain = scenario(2);
    GameTestAccess::manifest(slain);
    auto& avatar = *GameTestAccess::boss(slain);
    avatar.moveTo(1); avatar.takeDamage(damageType::trueDamage, avatar.getHealth() - 5);
    accepted(slain, {ActionType::grenadeBoss});
    require(slain.getOutcome() == Outcome::bossSlain && !slain.hasLivingBoss(), "boss death immediately wins with remaining threats");
    require(!slain.getEnemies().empty() && !slain.getPortals().empty(), "boss victory does not require clearing the board");
    require(restore(snapshot(slain)).getOutcome() == Outcome::bossSlain, "boss victory with remaining threats round trips");
    auto ruined = scenario();
    GameTestAccess::instability(ruined, 5); GameTestAccess::harm(ruined, 0, 100); GameTestAccess::check(ruined);
    require(ruined.getOutcome() == Outcome::defeat, "six instability loses");
}
void worldEvents() {
    auto g = scenario(4);
    GameTestAccess::enemies(g).clear();
    for (int id = 0; id < static_cast<int>(eventCards().size()); ++id) {
        GameTestAccess::card(g, id);
        require(logCount(g, "WORLD EVENT / " + eventNames().at(id)) == 1, "each world event resolves");
    }
    auto cycle = scenario();
    GameTestAccess::enemies(cycle).clear();
    std::set<std::string> events;
    int draws = 0, quiet = 0;
    while (events.size() < eventCards().size() - 1 && draws++ < 200) {
        GameTestAccess::drawCard(cycle);
        if (cycle.getActiveCard() == 0) { ++quiet; continue; }
        require(events.insert(eventNames().at(cycle.getActiveCard())).second, "meaningful events do not repeat before exhaustion");
    }
    require(events.size() == eventCards().size() - 1, "every meaningful event drawn once");
    require(quiet > 0, "quiet cards interrupt events");
    quiet = 0;
    for (int i = 0; i < 10000; ++i) { GameTestAccess::drawCard(cycle); quiet += cycle.getActiveCard() == 0; cycle.takeLog(); }
    require(quiet > 4700 && quiet < 5300, "roughly half of world draws are quiet");

}
void persistence() {
    auto original = scenario(3, BossId::rustGardener, 218);
    GameTestAccess::harm(original, 2, 3, true);
    accepted(original, {ActionType::move, 2});
    const auto saved = snapshot(original);
    auto copy = restore(saved);
    require(snapshot(copy) == saved, "all state round-trips including mid-round player, corrosion, inventories and RNG");
    original.takeLog(); copy.takeLog();
    for (int i = 0; i < 150 && original.getOutcome() == Outcome::ongoing; ++i) {
        accepted(original, {ActionType::wait}); accepted(copy, {ActionType::wait});
        require(snapshot(original) == snapshot(copy), "resumed simulation exactly matches uninterrupted game");
        require(original.takeLog() == copy.takeLog(), "resumed dice, events and target choices match");
    }
    require(original.getOutcome() != Outcome::ongoing, "waiting campaign terminates");
    rejects("not a save");
    rejects(saved + "garbage");
    for (std::size_t size : {std::size_t{0}, std::size_t{20}, saved.size() / 2, saved.size() - 5}) rejects(saved.substr(0, size));
    auto bad = saved;
    bad.replace(0, bad.find('\n'), "VEYRA_SAVE 99"); rejects(bad);
    bad = saved;
    const auto name = bad.find("Defender 1"); bad.replace(name, 10, "\033[2J"); rejects(bad);
}

void legacyBrandImport() {
    for (int kind = 0; kind < 4; ++kind) {
        auto original = scenario(2, static_cast<BossId>(kind), 218);
        // Import at every phase, including a card already drawn but not resolved.
        for (int step = 0; step < 12; ++step) {
            const auto current = snapshot(original);
            require(current.rfind("VEYRA_SAVE 6\n", 0) == 0, "new saves use the Veyra marker");
            auto legacy = current;
            legacy.replace(0, legacy.find('\n'), "FORTY_K_RPG 6");
            auto imported = restore(legacy);
            require(snapshot(imported) == current, "old marker imports every field without changing player names or numeric IDs");
            original.takeLog(); imported.takeLog();
            if (original.getPhase() == TurnPhase::players) {
                require(original.act({ActionType::wait}, false).accepted, "original accepts test order");
                require(imported.act({ActionType::wait}, false).accepted, "import accepts test order");
            } else {
                require(original.advancePhase() && imported.advancePhase(), "both advance the saved phase");
            }
            require(snapshot(original) == snapshot(imported), "legacy import retains RNG and phase continuation");
            require(original.takeLog() == imported.takeLog(), "imported actions use current names and identical results");
        }
    }
}

void phasedResolution() {
    auto stepped = scenario(2);
    GameTestAccess::calm(stepped);
    auto automatic = restore(snapshot(stepped));
    accepted(automatic, {ActionType::wait}); accepted(automatic, {ActionType::wait});
    require(stepped.act({ActionType::wait}, false).accepted, "first player phased action");
    require(stepped.getPhase() == TurnPhase::players && stepped.getRound() == 1, "other defender still acts first");
    require(stepped.act({ActionType::wait}, false).accepted, "last player phased action");
    require(stepped.getPhase() == TurnPhase::worldDraw && stepped.getActiveCard() == -1, "world card not drawn during player action");
    const auto paused = snapshot(stepped);
    require(!stepped.act({ActionType::wait}, false).accepted && !stepped.equip(WeaponId::pistol), "orders and equipment blocked outside defender phase");
    require(snapshot(stepped) == paused, "blocked phase actions change nothing");
    int steps = 0;
    while (stepped.getPhase() != TurnPhase::players && steps++ < 100) {
        const auto before = snapshot(stepped);
        auto resumed = restore(before);
        require(resumed.getPhase() == stepped.getPhase(), "phase persists at every boundary");
        require(stepped.advancePhase() && resumed.advancePhase(), "explicit phase advance");
        require(snapshot(stepped) == snapshot(resumed), "save resume never repeats or skips a phase effect");
        if (stepped.getPhase() == TurnPhase::worldResolve)
            require(stepped.getDoom() == 0 && stepped.getActiveCard() == 0, "revealing card applies no effects");
    }
    require(steps < 100 && stepped.getRound() == 2, "round ends only after upkeep");
    require(snapshot(stepped) == snapshot(automatic), "phased and automatic frontends resolve identically");

    auto deadline = scenario();
    GameTestAccess::doom(deadline, deadline.getBossDef().deadline - 1);
    GameTestAccess::nextCard(deadline, 6); // Lost Hours crosses the deadline before upkeep.
    require(deadline.act({ActionType::wait}, false).accepted, "deadline scenario action");
    deadline.advancePhase(); deadline.advancePhase();
    require(deadline.getDoom() > deadline.getBossDef().deadline && !deadline.getBoss(), "manifestation waits for upkeep");
    auto deadlineResume = restore(snapshot(deadline));
    deadline.resolvePending(); deadlineResume.resolvePending();
    require(snapshot(deadline) == snapshot(deadlineResume) && deadline.getBoss(), "mid-phase deadline save resumes and manifests exactly once");

    auto legacy = scenario(); GameTestAccess::enemies(legacy).clear(); GameTestAccess::calm(legacy);
    auto withoutWorld = [](std::string value) {
        const auto sector = value.find("SECTORS "); value.erase(sector,value.find('\n',sector)-sector+1);
        const auto at = value.find("WORLD "); value.erase(at, value.find('\n', at) - at + 1); return value;
    };
    std::string versionThree = withoutWorld(snapshot(legacy)); versionThree.replace(0, versionThree.find('\n')+1, "FORTY_K_RPG 3\n");
    require(snapshot(restore(versionThree)) == snapshot(legacy), "v3 campaign migrates without changing state or RNG");
    std::string versionTwo = withoutWorld(snapshot(legacy));
    versionTwo.replace(0, versionTwo.find('\n')+1, "FORTY_K_RPG 2\n");
    const auto legacyPlayer = versionTwo.find("\"Defender 1\"");
    versionTwo.insert(versionTwo.find('\n', legacyPlayer), " 4"); // Retired seals are discarded.
    const auto tacticsStart = versionTwo.find("TACTICS "), tacticsEnd = versionTwo.find('\n', tacticsStart);
    std::istringstream tactics(versionTwo.substr(tacticsStart, tacticsEnd - tacticsStart));
    std::ostringstream migratedTactics; std::string token;
    for (int i = 0; i < 7; ++i) { tactics >> token; migratedTactics << (i ? " " : "") << token; }
    for (int i = 0; i < 9; ++i) migratedTactics << " 3"; // Retired wards are discarded.
    while (tactics >> token) migratedTactics << ' ' << token;
    versionTwo.replace(tacticsStart, tacticsEnd - tacticsStart, migratedTactics.str());
    require(snapshot(restore(versionTwo)) == snapshot(legacy), "v2 seals and wards retire without changing other state or RNG");
    std::string old = withoutWorld(snapshot(legacy));
    old.replace(0, old.find('\n')+1, "FORTY_K_RPG 1\n");
    const auto playerLine = old.find("\"Defender 1\"");
    old.insert(old.find('\n', playerLine), " 0");
    const auto begin = old.find("TACTICS "); const auto end = old.find('\n', begin);
    old.erase(begin, end - begin + 1);
    auto migrated = restore(old);
    require(migrated.getPhase() == TurnPhase::players && migrated.getReversedRounds() == 0, "legacy v1 saves migrate safely");
}
void expandedEvents() {
    auto g = scenario(4);
    GameTestAccess::card(g, 16);
    int closed = 0;
    for (int i = 1; i <= 9; ++i) if (g.getClosure(i)) closed = i;
    require(closed > 0 && closed != 1 && closed != 6, "closure selects an eligible sector");
    for (int from = 1; from <= 9; ++from) {
        require(!g.canMove(from, closed), "closed sector cannot be entered");
        for (int to = 1; to <= 9; ++to) if (from != to && to != closed) require(g.routeStep(from, to) != from, "closure never traps occupants or isolates open sectors");
    }
    GameTestAccess::players(g)[0].moveTo(closed);
    require(!g.getRoutes(closed).empty(), "occupants can leave closed sector");
    GameTestAccess::card(g, 17);
    for (const auto& p : g.getPlayers()) require(p.getSector() != closed && p.getHealth() == p.getMaxHealth(), "teleport avoids closed sector without damage");
    GameTestAccess::card(g, 18); GameTestAccess::card(g, 19);
    require(g.playerHitChance(g.currentPlayer(), WeaponId::rotaryRiveter) == 65 && g.getEnemyAimRounds() == 2, "accuracy effects apply to both sides");
    auto copy = restore(snapshot(g)); require(snapshot(copy) == snapshot(g), "closures and accuracy conditions persist exactly");
    GameTestAccess::endRound(g); require(g.getClosure(closed) == 1 && g.getPlayerAimRounds() == 1, "conditions survive into next defender round");
    GameTestAccess::endRound(g); require(!g.getClosure(closed) && !g.getPlayerAimRounds() && !g.getEnemyAimRounds(), "conditions expire automatically");
    int reducedHits = 0;
    for (unsigned seed = 0; seed < 60; ++seed) {
        auto normal = scenario(); GameTestAccess::enemies(normal).clear(); GameTestAccess::spawn(normal, 1, 2);
        auto flare = normal; GameTestAccess::card(flare, 19);
        GameTestAccess::seed(normal, seed); GameTestAccess::seed(flare, seed);
        GameTestAccess::enemyPhase(normal); GameTestAccess::enemyPhase(flare);
        const bool normalHit = normal.getCombatEvents().back().hit, flareHit = flare.getCombatEvents().back().hit;
        require(!flareHit || normalHit, "flare never improves an enemy roll"); reducedHits += normalHit && !flareHit;
    }
    require(reducedHits > 0, "flare reduces actual enemy hits");
    auto destruction = scenario(); const auto initial = destruction.getPortals().size();
    GameTestAccess::card(destruction, 14); require(destruction.getPortals().size() == initial - 1, "collapse destroys one portal");
    const auto enemies = destruction.getEnemies().size();
    GameTestAccess::card(destruction, 15); require(destruction.getEnemies().size() == enemies - 1, "lance destroys one enemy");
    GameTestAccess::card(destruction, 21); require(destruction.getPortals().empty() && destruction.isLastStand(), "null resonance closes all portals and triggers last stand");
    GameTestAccess::enemies(destruction).clear(); GameTestAccess::spawn(destruction, 8, 0); GameTestAccess::spawn(destruction, 8, 4);
    GameTestAccess::manifest(destruction); GameTestAccess::boss(destruction)->moveTo(8);
    GameTestAccess::card(destruction, 20); require(destruction.getEnemies().empty() && destruction.hasLivingBoss(), "purge removes regular enemies but spares boss");
    auto replacement = scenario(); GameTestAccess::card(replacement, 2);
    require(replacement.getEnemies().back().getName() == "Breach tuner", "procession replaces obsolete saboteur event");
}
void recruitmentAndHitChance() {
    int previous = -1; std::set<int> classes;
    for (int i = 0; i < 4; ++i) { const auto quick = quickDefender(42, previous); classes.insert(quick.preset); previous = quick.preset; require(quick.name != "Brother Castor", "quick recruit has varied callsign"); }
    require(classes.size() == 4, "four quick deployments rotate through every preset");
    for (const auto& c : characters()) {
        Game g; require(g.addCustom(4,4,4,4,4,"Custom",c.weapon), "custom can select each preset starting weapon");
        require(g.currentPlayer().getWeapon() == c.weapon && g.currentPlayer().getWeapons().size() == 2, "custom equips selection and retains pistol");
    }
    Game invalid; require(!invalid.addCustom(4,4,4,4,4,"Custom",WeaponId::rivetRifle), "non-starting weapon rejected atomically");
    auto g = scenario(); std::mt19937 expected(123); GameTestAccess::seed(g, 123);
    for (int i = 0; i < 200; ++i) require(GameTestAccess::playerHit(g) == (std::uniform_int_distribution<int>(1,100)(expected) <= 85), "surplus Precision applies to actual attack rolls");
    GameTestAccess::card(g, 18); GameTestAccess::seed(g, 456); expected.seed(456);
    for (int i = 0; i < 200; ++i) require(GameTestAccess::playerHit(g) == (std::uniform_int_distribution<int>(1,100)(expected) <= 65), "Ashfall subtracts twenty percentage points from actual rolls");
}

void newRules() {
    Player swordsman(16, 2, 5, 2, 3, 1, "Swordsman");
    require(weaponProfile(WeaponId::sword, swordsman).hitChance == 95, "Strength 5 sword user has 95 percent accuracy");
    Player marksman(16, 2, 4, 7, 5, 1, "Marksman");
    require(weaponProfile(WeaponId::needleRifle, marksman).hitChance == 90, "marksman surplus uses its higher Precision requirement");
    require(weaponProfile(WeaponId::rivetRifle, marksman).hitChance == 100, "accuracy caps at one hundred");
    Player weakMarksman(16, 2, 1, 5, 1, 1, "Weak marksman");
    require(weaponProfile(WeaponId::rotaryRiveter, weakMarksman).hitChance == 75, "Precision surplus combines with one unmet Strength penalty");
    auto g = scenario(2);
    GameTestAccess::card(g, 16);
    int closedSector = 0; for (int sector = 1; sector <= 9; ++sector) if (g.getClosure(sector)) closedSector = sector;
    require(closedSector != 0, "transport test has a cordon");
    const auto roads = g.getRoutes(1);
    GameTestAccess::card(g, 22);
    require(g.getTransportRounds() == 2 && g.getRoutes(1) == roads, "transport does not alter roads or enemy pathfinding");
    require(!g.canMove(1, 0) && !g.canMove(1, 10) && !g.canMove(1, 1), "transport still validates destinations");
    for (const auto& w : weapons()) GameTestAccess::players(g)[0].giveWeapon(w.id);
    require(g.equip(WeaponId::needleRifle), "new loot weapon equips");
    const auto copy = restore(snapshot(g));
    require(snapshot(copy) == snapshot(g) && copy.canMove(1, closedSector), "new weapons and transport survive save and load");
    GameTestAccess::endRound(g);
    require(g.getTransportRounds() == 1, "transport survives into next defender phase");
    require(g.act({ActionType::move, closedSector}, false).accepted, "flight bypasses a sector cordon");
    require(g.act({ActionType::move, 9}, false).accepted, "each squad member can fly in the same phase");
    GameTestAccess::endRound(g);
    require(g.getTransportRounds() == 0 && !g.canMove(1, 9), "transport expires after that defender phase");
    for (int id = 0; id < 4; ++id) {
        auto patron = scenario(1, static_cast<BossId>(id));
        require(patron.getEnemies()[0].getKind() == 7 + 2 * id, "starting construct belongs to selected sovereign");
        GameTestAccess::card(patron, 7);
        require(patron.getEnemies().back().getKind() == 8 + 2 * id, "hunting event summons patron's hunter");
        GameTestAccess::manifest(patron);
        require(patron.getBoss()->getMaxHealth() >= 50 && patron.getBossDef().damage >= 7, "all sovereigns have stronger health and attacks");
        require(snapshot(restore(snapshot(patron))) == snapshot(patron), "new enemies and boss HP round trip");
    }
    auto quiet = scenario(); const auto before = quiet.getPlayers()[0].getSupply(Consumable::medkit);
    for (int i = 0; i < 30; ++i) GameTestAccess::card(quiet, 0);
    require(quiet.getPlayers()[0].getSupply(Consumable::medkit) == before, "quiet cards do not trigger bonus supply events");
    auto versionFour = [](const Game& state) {
        auto bytes = snapshot(state);
        const auto sector = bytes.find("SECTORS "); bytes.erase(sector,bytes.find('\n',sector)-sector+1);
        bytes.replace(0, bytes.find('\n')+1, "FORTY_K_RPG 4\n");
        const auto end = bytes.find('\n', bytes.find("WORLD "));
        const auto transport = bytes.rfind(' ', end);
        bytes.erase(transport, end - transport); return bytes;
    };
    auto legacy = scenario();
    require(snapshot(restore(versionFour(legacy))) == snapshot(legacy), "v4 saves default to no transport");
    GameTestAccess::manifest(legacy);
    auto oldBoss = versionFour(legacy);
    const auto bossLine = oldBoss.find("\n1 9 56\n");
    require(bossLine != std::string::npos, "legacy boss fixture found");
    oldBoss.replace(bossLine, 8, "\n1 9 13\n");
    auto scaled = restore(oldBoss);
    require(scaled.getBoss()->getHealth() == 28 && scaled.getBoss()->getMaxHealth() == 56, "older bosses retain health percentage under stronger rules");
    GameTestAccess::boss(legacy)->takeDamage(damageType::trueDamage, 1000);
    require(restore(versionFour(legacy)).getOutcome() == Outcome::bossSlain, "older ongoing dead-boss campaigns migrate to immediate victory");
}
void worldWithoutDamage() {
    for (int card = 0; card < static_cast<int>(eventCards().size()); ++card) {
        auto g = scenario(4);
        for (auto& p : GameTestAccess::players(g)) p.takeDamage(damageType::trueDamage, 3);
        std::vector<int> hp; for (const auto& p : g.getPlayers()) hp.push_back(p.getHealth());
        GameTestAccess::card(g, card);
        for (std::size_t i = 0; i < hp.size(); ++i) require(g.getPlayers()[i].getHealth() >= hp[i], "world cards never inflict direct player damage");
    }
    auto g = scenario(2); GameTestAccess::players(g)[0].moveTo(3);
    GameTestAccess::card(g, 4);
    require(g.getReversedRounds() == 2 && g.canMove(3, 2) && !g.canMove(3, 4), "one-way routes actually reverse");
    require(g.canMove(1, 2) && g.canMove(2, 1), "two-way routes remain two-way");
    for (int from = 1; from <= 9; ++from) for (int to = 1; to <= 9; ++to) {
        int cursor = from, count = 0;
        while (cursor != to && count++ < 10) { int next = g.routeStep(cursor, to); require(g.canMove(cursor, next), "reversed path uses a legal outgoing road"); cursor = next; }
        require(cursor == to, "reversed map remains traversable");
    }
    require(g.act({ActionType::move, 2}, false).accepted, "players follow reversed access");
    GameTestAccess::endRound(g); require(g.getReversedRounds() == 1, "reversal persists through next player round");
    GameTestAccess::endRound(g); require(g.getReversedRounds() == 0 && !g.canMove(3, 2), "original access restored after duration");
}
void tacticalRoles() {
    auto g = scenario(2); GameTestAccess::enemies(g).clear();
    GameTestAccess::players(g)[0].moveTo(2); GameTestAccess::portals(g) = {{2, 6}, {9, 6}};
    GameTestAccess::spawn(g, 2, 4);
    auto before = snapshot(g);
    require(!g.act({ActionType::attackPortal}, false).accepted, "Anchor shields portal from weapon attacks");
    require(snapshot(g) == before, "shield rejection spends no ammo or action");
    require(g.act({ActionType::grenadePortal}, false).accepted && g.portalShielded(2) && g.getPortals().size() == 1, "grenade destroys portal through Anchor shield");
    require(g.getCombatEvents().size() == 1 && g.getCombatEvents()[0].defeated && g.getCombatEvents()[0].style == CombatStyle::grenade, "portal combat event records explosion and destruction");

    auto guard = scenario(2); accepted(guard, {ActionType::guard});
    GameTestAccess::physical(guard, 10);
    require(guard.getPlayers()[0].getHealth() == 14 && guard.getPlayers()[0].isGuarding(), "guard reduces incoming raw damage by three before armor");
    GameTestAccess::endRound(guard); require(!guard.getPlayers()[0].isGuarding(), "guard expires at upkeep");

    auto suppression = scenario(2); GameTestAccess::enemies(suppression).clear();
    GameTestAccess::spawn(suppression, 1, 5); GameTestAccess::seed(suppression, 0);
    const int wireStalker = suppression.getEnemies()[0].getId();
    require(suppression.act({ActionType::suppress, wireStalker}, false).accepted, "suppress action");
    require(suppression.getEnemies()[0].isSuppressed() && suppression.getEnemies()[0].getHealth() == 4, "suppression hits but deals no damage");
    auto copy = restore(snapshot(suppression));
    require(copy.getEnemies()[0].isSuppressed(), "suppression saves");
    GameTestAccess::enemyPhase(suppression);
    require(!suppression.getEnemies()[0].isSuppressed() && suppression.getPlayers()[0].getHealth() == 16 && suppression.getPlayers()[1].getHealth() == 16, "suppressed enemy skips exactly one activation");

    auto hunter = scenario(); GameTestAccess::enemies(hunter).clear(); GameTestAccess::players(hunter)[0].moveTo(5);
    GameTestAccess::spawn(hunter, 7, 5); GameTestAccess::enemyPhase(hunter);
    require(hunter.getEnemies()[0].getSector() == 9 && hunter.currentPlayer().getHealth() == 16, "Hound pursues without attacking during movement");
    GameTestAccess::enemies(hunter)[0].moveTo(7); GameTestAccess::reverse(hunter, 2); GameTestAccess::enemyPhase(hunter);
    require(hunter.getEnemies()[0].getSector() == 5, "Hound follows reversed roads");

    auto breachTuner = scenario(); GameTestAccess::enemies(breachTuner).clear(); GameTestAccess::spawn(breachTuner, 8, 3);
    GameTestAccess::enemyPhase(breachTuner); require(breachTuner.getEnemies()[0].getSector() == 8, "Breach tuner holds position without a local defender");
    GameTestAccess::players(breachTuner)[0].moveTo(8); GameTestAccess::enemyPhase(breachTuner);
    require(!breachTuner.getCombatEvents().empty() && breachTuner.getCombatEvents()[0].style == CombatStyle::psychic, "Breach tuner attacks with psychic damage");

    auto rustDrone = scenario(); GameTestAccess::enemies(rustDrone).clear(); GameTestAccess::players(rustDrone)[0].moveTo(6); GameTestAccess::spawn(rustDrone, 6, 6); GameTestAccess::seed(rustDrone, 0);
    GameTestAccess::enemyPhase(rustDrone);
    require(rustDrone.currentPlayer().getPoison() == 2, "Drone infects independently of boss selection");


}

void combatPresentation() {
    int hits = 0, misses = 0;
    for (unsigned seed = 0; seed < 30; ++seed) {
        auto g = scenario(2); GameTestAccess::enemies(g).clear(); GameTestAccess::spawn(g, 1, 1);
        g.equip(WeaponId::pistol); GameTestAccess::seed(g, seed);
        require(g.act({ActionType::attackEnemy, g.getEnemies()[0].getId()}, false).accepted, "animated attack accepted");
        const auto& beat = g.getCombatEvents().at(0);
        require(beat.attacker == "Defender 1" && beat.target == "Siege automaton" && beat.sector == 1 && beat.defenderAttack, "combat event identifies both actors and sector");
        require(beat.style == CombatStyle::ranged && beat.damage == 0 && !beat.defeated, "armor-blocked shot records zero damage");
        hits += beat.hit; misses += !beat.hit;
    }
    require(hits > 0 && misses > 0, "animation distinguishes misses from blocked hits");
    auto enemies = scenario(2, BossId::echoWeaver); GameTestAccess::enemies(enemies).clear(); GameTestAccess::spawn(enemies, 1, 0);
    GameTestAccess::enemyPhase(enemies);
    require(enemies.getCombatEvents().size() == 2, "double enemy strike produces separate animation beats");
    for (const auto& beat : enemies.getCombatEvents()) require(!beat.defenderAttack && beat.sector == 1 && beat.target.find("Defender") == 0, "enemy animation identifies actual selected defender");
}

void lastStand() {
    auto g = scenario(2); GameTestAccess::enemies(g).clear();
    GameTestAccess::players(g)[0].moveTo(2); GameTestAccess::portals(g) = {{2,6}};
    GameTestAccess::spawn(g, 2, 0);
    require(g.act({ActionType::grenadePortal}, false).accepted && g.isLastStand() && g.getOutcome() == Outcome::ongoing, "closing last portal enters enemy last stand");
    require(logCount(g, "ENEMY LAST STAND") == 1, "entry announces mechanics exactly once");
    require(restore(snapshot(g)).isLastStand(), "last stand survives save/load");
    GameTestAccess::card(g, 11);
    require(g.getEnemies()[0].getSector() == 2, "Long March cannot move ritual participants");
    GameTestAccess::players(g)[1].moveTo(2);
    require(g.act({ActionType::grenadeEnemy, g.getEnemies()[0].getId()}, false).accepted && g.getOutcome() == Outcome::banished, "last enemy defeated with no portals wins");

    int summons = 0;
    for (unsigned seed = 0; seed < 300; ++seed) {
        auto trial = scenario(); GameTestAccess::portals(trial).clear(); GameTestAccess::enemies(trial).clear();
        GameTestAccess::spawn(trial, 8, 3); // Breach tuner must not attack during a ritual.
        GameTestAccess::players(trial)[0].moveTo(8);
        GameTestAccess::seed(trial, seed);
        std::mt19937 expected(seed);
        const bool success = std::uniform_int_distribution<int>(1,10)(expected) == 1;
        GameTestAccess::enemyPhase(trial);
        require(trial.getPortals().empty() != success, "each enemy gets exactly one 1-in-10 ritual roll");
        require(trial.getEnemies()[0].getSector() == 8 && trial.currentPlayer().getHealth() == 16, "ritual replaces attacks, movement and raids");
        if (success) { ++summons; require(trial.getPortals()[0].sector == 8 && trial.getPortals()[0].health == 6 && !trial.isLastStand(), "successful ritual opens local portal and exits last stand"); }
    }
    require(summons > 15 && summons < 50, "seeded ritual frequency close to ten percent");

    auto resumed = scenario(); GameTestAccess::portals(resumed).clear(); GameTestAccess::enemies(resumed).clear();
    GameTestAccess::players(resumed)[0].moveTo(8); GameTestAccess::spawn(resumed, 2, 5); GameTestAccess::spawn(resumed, 8, 3); GameTestAccess::calm(resumed);
    resumed.act({ActionType::wait}, false); resumed.advancePhase(); resumed.advancePhase(); resumed.advancePhase(); // draw, resolve, corrosion
    unsigned successSeed = 0;
    for (;; ++successSeed) { std::mt19937 rng(successSeed); if (std::uniform_int_distribution<int>(1,10)(rng) == 1) break; }
    GameTestAccess::seed(resumed, successSeed);
    auto copy = restore(snapshot(resumed));
    resumed.advancePhase(); copy.advancePhase();
    require(!resumed.isLastStand() && snapshot(resumed) == snapshot(copy), "save before ritual preserves exact success and RNG");
    require(resumed.enemyIntent().description.find("Attacks") != std::string::npos, "later enemies immediately resume normal behavior after summon");
    resumed.advancePhase(); require(!resumed.getCombatEvents().empty(), "resumed Breach tuner attacks instead of rolling another summon");

    auto suppression = scenario(2); GameTestAccess::enemies(suppression).clear(); GameTestAccess::portals(suppression).clear();
    GameTestAccess::spawn(suppression, 1, 0); GameTestAccess::seed(suppression, 0);
    require(suppression.act({ActionType::suppress, suppression.getEnemies()[0].getId()}, false).accepted && suppression.getEnemies()[0].isSuppressed(), "ritual participant can be suppressed");
    GameTestAccess::seed(suppression, successSeed); GameTestAccess::enemyPhase(suppression);
    require(suppression.isLastStand() && !suppression.getEnemies()[0].isSuppressed(), "suppression skips even a would-be successful ritual");
    GameTestAccess::card(suppression, 3);
    require(!suppression.isLastStand() && suppression.getPortals().size() == 1, "world portal event also ends last stand");

    auto doubleStrike = scenario(1, BossId::echoWeaver); GameTestAccess::enemies(doubleStrike).clear(); GameTestAccess::portals(doubleStrike).clear();
    GameTestAccess::spawn(doubleStrike, 2, 0); GameTestAccess::seed(doubleStrike, 0);
    GameTestAccess::enemyPhase(doubleStrike);
    require(logCount(doubleStrike, "attempts a last-stand ritual") == 1, "Echo Weaver grants no extra ritual rolls");

    auto safe = scenario(); GameTestAccess::portals(safe).clear(); GameTestAccess::enemies(safe).clear(); GameTestAccess::spawn(safe, 1, 0);
    GameTestAccess::seed(safe, successSeed); GameTestAccess::enemyPhase(safe);
    require(!safe.getPortals().empty() && safe.getPortals()[0].sector != 1 && safe.getPortals()[0].sector != 6, "ritual in protected sector opens portal elsewhere");

    auto boss = scenario(); GameTestAccess::manifest(boss); GameTestAccess::portals(boss).clear(); GameTestAccess::enemies(boss).clear();
    require(!boss.isLastStand() && boss.getOutcome() == Outcome::ongoing, "manifested boss disables last stand even with no portals");
    const int step=boss.routeStep(9,boss.currentPlayer().getSector());
    GameTestAccess::seed(boss, successSeed); GameTestAccess::enemyPhase(boss);
    require(boss.getPortals().empty() && boss.getBoss()->getSector()==step,"boss pursues instead of summoning");
    GameTestAccess::players(boss)[0].moveTo(step);
    GameTestAccess::spawn(boss,step,0); GameTestAccess::spawn(boss,2,5);
    GameTestAccess::enemyPhase(boss);
    require(boss.getCombatEvents().size()==2,"regular enemy and boss both attack without portals");
    require(boss.getEnemies()[1].getSector()!=2,"hunters pursue after manifestation without portals");
    require(logCount(boss,"last-stand ritual")==0,"no ritual attempts after boss spawn");
    require(!restore(snapshot(boss)).isLastStand(),"loading a boss campaign cannot restore last stand");
    GameTestAccess::card(boss,3); GameTestAccess::card(boss,21);
    require(boss.getPortals().empty() && !boss.isLastStand(),"closing later portals never reactivates last stand");

    auto transition=scenario(); GameTestAccess::enemies(transition).clear(); GameTestAccess::portals(transition).clear();
    GameTestAccess::spawn(transition,8,0); GameTestAccess::doom(transition,transition.getBossDef().deadline-1);
    GameTestAccess::calm(transition); GameTestAccess::seed(transition,0);
    require(transition.isLastStand(),"ritual state before natural manifestation");
    accepted(transition,{ActionType::wait});
    require(transition.hasLivingBoss() && !transition.isLastStand(),"natural manifestation ends last stand");
    require(logCount(transition,"LAST STAND ENDS")==1,"boss arrival explains that rituals stop");

}

Action patrolOrder(Game& g) {
    const auto& p = g.currentPlayer();
    // Use the best available weapon for the actual target resistance.
    const bool bossHere = g.hasLivingBoss() && g.getBoss()->getSector() == p.getSector();
    int best = -100;
    WeaponId bestWeapon = p.getWeapon();
    for (auto id : p.getWeapons()) {
        const auto profile = weaponProfile(id, p);
        const int resist = bossHere ? (profile.type == damageType::psychic ? g.getBoss()->getWill() : g.getBoss()->getArmor()) : 0;
        const int score = std::max(0, profile.damage - resist) * g.playerHitChance(p, id);
        if (score > best) { best = score; bestWeapon = id; }
    }
    g.equip(bestWeapon);
    if (p.getHealth() <= p.getMaxHealth() / 2 && p.getSupply(Consumable::medkit)) return {ActionType::medkit};
    if (bossHere) return {p.getSupply(Consumable::grenade) ? ActionType::grenadeBoss : ActionType::attackBoss};
    const auto portal = std::find_if(g.getPortals().begin(), g.getPortals().end(), [&](const auto& r) { return r.sector == p.getSector(); });
    if (portal != g.getPortals().end() && p.getSupply(Consumable::grenade)) return {ActionType::grenadePortal};
    for (const auto& e : g.getEnemies()) if (e.getSector() == p.getSector())
        return {p.getSupply(Consumable::grenade) && e.getHealth() <= 5 ? ActionType::grenadeEnemy : ActionType::attackEnemy, e.getId()};
    if (portal != g.getPortals().end()) return {ActionType::attackPortal};
    int destination = p.getSector(), shortest = 100;
    auto consider = [&](int sector) {
        if (sector != p.getSector() && g.routeStep(p.getSector(), sector) == p.getSector()) return;
        int cursor = p.getSector(), distance = 0;
        while (cursor != sector && distance < 10) { cursor = g.routeStep(cursor, sector); ++distance; }
        if (distance < shortest) { shortest = distance; destination = sector; }
    };
    if (g.hasLivingBoss()) consider(g.getBoss()->getSector());
    for (const auto& r : g.getPortals()) consider(r.sector);
    for (const auto& e : g.getEnemies()) consider(e.getSector());
    if (destination != p.getSector()) return {ActionType::move, g.routeStep(p.getSector(), destination)};
    return {ActionType::wait};
}
void simulations() {
    int total = 0, wins = 0, bossVictories = 0;
    for (int boss = 0; boss < 4; ++boss) {
        int bossWins = 0, bossRuns = 0;
        for (int squad = 1; squad <= 4; ++squad) for (unsigned seed = 0; seed < 50; ++seed) {
            Game g(static_cast<BossId>(boss), seed);
            for (int p = 0; p < squad; ++p) require(g.addPreset(p, "Patrol " + std::to_string(p)), "sim recruit");
            g.start(); g.takeLog();
            int actions = 0;
            while (g.getOutcome() == Outcome::ongoing && actions < 400) {
                accepted(g, patrolOrder(g));
                for (const auto& p : g.getPlayers()) require(p.getAlive() && p.getHealth() <= p.getMaxHealth(), "sim player HP invariant");
                for (const auto& e : g.getEnemies()) require(e.getAlive(), "sim no dead enemies retained");
                require(g.getInstability() >= 0 && g.getInstability() <= 6, "sim instability invariant");
                if (actions % 11 == 0) require(snapshot(restore(snapshot(g))) == snapshot(g), "sim save roundtrip");
                g.takeLog(); ++actions;
            }
            require(g.getOutcome() != Outcome::ongoing, "simulation terminates");
            const bool won = g.getOutcome() == Outcome::banished || g.getOutcome() == Outcome::bossSlain;
            wins += won; bossWins += won; ++total; ++bossRuns;
            bossVictories += g.getOutcome() == Outcome::bossSlain;
        }
        require(bossWins > 0, "every boss campaign winnable through public actions");
        std::cout << "  Patrol simulations / " << bossDefinition(static_cast<BossId>(boss)).name << ": " << bossWins << '/' << bossRuns << " wins\n";
    }
    std::cout << "  " << total << " seeded patrols completed, " << wins << " wins (simple clear-the-board policy).\n";
    std::cout << "  " << bossVictories << " victories came from defeating a naturally manifested boss.\n";
    require(bossVictories > 0, "boss combat can be won without scenario injection");
}
}

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests {
        {"same-sector trading", trading}, {"sector combat bonuses", sectorBonuses}, {"market and free movement", marketAndFreeMoves}, {"permanent bridge sacrifice", bridgeSacrifice}, {"damage and healing", damage}, {"map routes", mapRoutes}, {"directed movement", directedMovement}, {"character creation", creation},
        {"boss setup", setup}, {"turn order and rejected actions", turnOrderAndInvalidActions},
        {"weapon accuracy", accuracy}, {"portals and banishment", portalsAndLoot},
        {"items and scavenging", itemsAndSearch}, {"recovery and retargeting", recoveryAndRetargeting},
        {"portal waves and doom", wavesAndDoom}, {"boss rules and endings", bossRules},
        {"world event deck", worldEvents}, {"save validation and deterministic continuation", persistence},
        {"legacy branding import", legacyBrandImport},
        {"phased resolution and migration", phasedResolution}, {"world changes without direct damage", worldWithoutDamage}, {"expanded event effects", expandedEvents}, {"recruitment and percentage accuracy", recruitmentAndHitChance},
        {"new weapons, transport and patron enemies", newRules}, {"tactical enemy roles and counters", tacticalRoles}, {"enemy last stand and complete victory", lastStand}, {"combat presentation events", combatPresentation}, {"seeded campaign simulations", simulations}
    };
    for (const auto& [name, test] : tests) {
        try { test(); std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& e) { std::cerr << "FAIL " << name << ": " << e.what() << '\n'; return 1; }
    }
    std::cout << "All " << tests.size() << " groups passed (" << assertions << " assertions).\n";
}
