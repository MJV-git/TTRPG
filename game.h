#pragma once
#include "boss.h"
#include "enemy.h"
#include "map.h"
#include "player.h"
#include <array>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <random>
#include <string>
#include <vector>

enum class Outcome { ongoing, banished, bossSlain, defeat };
enum class TurnPhase { players, worldDraw, worldResolve, enemies, roundEnd };
enum class ActionType {
    move, attackEnemy, attackPortal, attackBoss, search, rest, medkit,
    grenadeEnemy, grenadePortal, grenadeBoss, wait, guard, suppress, market, collapseBridge
};
struct Action { ActionType type; int target = 0; };
struct ActionResult { bool accepted; std::string error; };
struct Item { bool isWeapon; int id; };
std::string itemName(Item item);
struct Portal { int sector; int health = 6; };
struct CharacterDef {
    std::string name;
    int health, armor, strength, precision, will;
    WeaponId weapon;
    std::string description;
};
const std::vector<CharacterDef>& characters();
struct QuickDefender { int preset; std::string name; };
QuickDefender quickDefender(std::uint32_t seed, int previous = -1);
const std::vector<std::string>& eventNames();
struct EventCard { std::string name, category, flavor, effect; int art; };
const std::vector<EventCard>& eventCards();
struct EnemyIntent {
    std::string name, description;
    int sector = 0, destination = 0;
    bool active = false;
};
enum class CombatStyle { ranged, melee, psychic, grenade, suppression };
struct CombatEvent {
    std::string attacker, target;
    int sector, damage;
    CombatStyle style;
    bool hit, defenderAttack, defeated;
};

class Game {
    friend struct GameTestAccess;
    BossId patron;
    std::uint32_t seed;
    std::mt19937 rng;
    std::vector<Player> players;
    std::vector<Enemy> enemies;
    std::vector<Portal> portals;
    std::optional<Boss> boss;
    std::array<int, 9> sectorClosures {};
    int playerAimRounds = 0, enemyAimRounds = 0;
    int transportRounds = 0;
    bool bridgeCollapsed = false;
    std::array<Item,3> marketOffers {{{true,0},{false,0},{false,1}}};
    std::vector<int> deck;
    TurnPhase phase = TurnPhase::players;
    int activeCard = -1, enemyCursor = 0, reversedRounds = 0;
    bool rotResolved = false, bossResolved = false;
    std::vector<std::string> log;
    std::vector<CombatEvent> combatEvents;
    std::size_t playerIdx = 0;
    int round = 1, doom = 0, chaos = 0, finalRounds = 0, nextEnemyId = 1;
    int kills = 0, closed = 0;
    bool started = false;
    Outcome outcome = Outcome::ongoing;
    std::string ending;

    int roll(int low, int high);
    void record(const std::string& message);
    void spawnEnemy(int sector, int kind);
    int patronEnemy(bool elite = false) const { return 7 + 2 * static_cast<int>(patron) + (elite ? 1 : 0); }
    void spawnPortal(int preferredSector = 0);
    void summonPortal(int sector, const std::string& source);
    void announceLastStand(bool previous);
    void grantSupply(Player& player, Consumable item);
    void grantLoot(Player& player, bool weaponPreferred = false);
    bool attackHits(int bonus, int difficulty);
    bool playerHits(const Player& player);
    void closeSector();
    void refreshMarket();
    void finishAction();
    void resolveWorld();
    void drawWorldCard();
    void applyWorldCard();
    void resolveEnemies();
    void resolveEnemyStep();
    void attackAt(int sector, int damage, damageType type, int accuracy, const std::string& source, bool infect);
    int nearestDestination(int from) const;
    void finishRound();
    void recover(Player& player);
    int harm(Player& player, damageType type, int damage, const std::string& source, bool infect);
    void checkOutcome();
    void cleanBattlefield(Player& player);
    std::vector<std::size_t> targetsAt(int sector) const;

public:
    explicit Game(BossId patron = BossId::khorne, std::uint32_t seed = std::random_device{}());
    static Game tutorial(int sector = 1);
    bool addPreset(int preset, const std::string& name);
    bool addCustom(int vitality, int armor, int strength, int precision, int will, const std::string& name, WeaponId startingWeapon = WeaponId::lasgun);
    static int statCost(int value);
    void start();
    ActionResult act(Action action, bool automatic = true);
    bool advancePhase();
    void resolvePending();
    TurnPhase getPhase() const { return phase; }
    int getActiveCard() const { return activeCard; }
    int getDeckSize() const { return static_cast<int>(deck.size()); }
    int getReversedRounds() const { return reversedRounds; }
    bool portalShielded(int sector) const;
    bool hasLivingBoss() const { return boss && boss->getAlive(); }
    bool isLastStand() const { return started && outcome == Outcome::ongoing && !boss && portals.empty() && !enemies.empty(); }
    std::vector<int> getRoutes(int sector) const;
    bool canMove(int from, int to) const;
    int routeStep(int from, int to) const;
    int getClosure(int sector) const { return sectorClosures.at(sector - 1); }
    int getPlayerAimRounds() const { return playerAimRounds; }
    int getEnemyAimRounds() const { return enemyAimRounds; }
    int getTransportRounds() const { return transportRounds; }
    bool isBridgeCollapsed() const { return bridgeCollapsed; }
    const std::array<Item,3>& getMarketOffers() const { return marketOffers; }
    bool freeMove(int from, int to) const;
    int playerDamage(const Player& player, WeaponId id) const;
    int playerTrueBonus(const Player& player) const { return player.getSector() == 2 ? 5 : 0; }
    int playerHitChance(const Player& player, WeaponId id) const;
    EnemyIntent enemyIntent() const;
    const std::vector<CombatEvent>& getCombatEvents() const { return combatEvents; }
    bool equip(WeaponId id);
    ActionResult trade(std::size_t recipient, Item item);
    const Player& currentPlayer() const { return players.at(playerIdx); }
    const std::vector<Player>& getPlayers() const { return players; }
    const std::vector<Enemy>& getEnemies() const { return enemies; }
    const std::vector<Portal>& getPortals() const { return portals; }
    const std::optional<Boss>& getBoss() const { return boss; }
    const BossDef& getBossDef() const { return bossDefinition(patron); }
    std::size_t getPlayerIndex() const { return playerIdx; }
    int getRound() const { return round; }
    int getDoom() const { return doom; }
    int getChaos() const { return chaos; }
    int livingDefenders() const;
    int getFinalRounds() const { return finalRounds; }
    int getKills() const { return kills; }
    int getClosed() const { return closed; }
    std::uint32_t getSeed() const { return seed; }
    Outcome getOutcome() const { return outcome; }
    const std::string& getEnding() const { return ending; }
    std::vector<std::string> takeLog();
    void save(std::ostream& out) const;
    static Game load(std::istream& in);
};
