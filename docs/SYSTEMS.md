# Systems review — Veyra: The Last Watch

Reviewed against the finished engine and both frontends on 2026-10-31.
This is a map of the implemented systems, not the earlier design discussion.
The setting revision changes names, prose and procedural artwork; it preserves
rule values, definition ordering, random draws, map connections and save fields.

## Setting and vocabulary

Veyra is an isolated industrial city whose relay network has fractured into the
Hollow: a signal space where commands can assemble physical machines. Four
hostile intelligences, called sovereigns, try to cross into the district. The
Last Watch combines municipal defenders, scouts and resonance specialists to
close the breaches before the city loses coherence.

Psychic damage represents interference carried through the relay network;
Will measures the ability to withstand or focus it. Instability measures the
district's breakdown. Corrosion is the lingering damage caused by invasive
metal-eating material. These descriptions give the existing mechanics a new
context without adding new rules.

| Campaign (stable numeric ID) | Identity | Followers | Existing rule |
| --- | --- | --- | --- |
| Ember Regent (0) | A furnace intelligence rebuilding the district as a kiln | Kiln sentinel, Cinder tracker | Doom 24; boss gains 2 damage at or below half HP |
| Prism Oracle (1) | A fractured routing intelligence projecting false paths | Prism mote, Refraction crawler | Doom 32; four starting portals; psychic boss attacks |
| Echo Weaver (2) | A repeating command signal that drives synchronized machines | Echo runner, Harmonic prowler | Doom 28; enemies and boss strike twice |
| Rust Gardener (3) | An industrial reclamation intelligence cultivating living corrosion | Rust carrier, Orchard crawler | Doom 30; damaging hostile hits inflict corrosion |

Defender presets are Bulwark, Trailguard, Resonant and Duskblade, in that order.
Their starting equipment is respectively Rotary riveter, Beam carbine,
Resonance staff and Arc blade. All retain a Rivet pistol and standard supplies;
Trailguards receive the existing extra grenade.

Other enemies are Hollow scavenger, Siege automaton, Signal leech, Breach tuner,
Gate anchor, Wire stalker and Rust drone. A Gate anchor protects its local portal.
Weapon and consumable names, event cards, tutorials and contact labels use the
same terminology in both interfaces. Sector names remain ordinary city locations.

The visual vocabulary is industrial: visored work helmets, antennae, mechanical
walkers, furnace vents, faceted prisms, waveforms and branching reclamation pipes.
The title emblem is a relay beacon. All illustrations are drawn locally by the
GUI; the documentation screenshots are captured from that renderer.

## Ownership and interfaces

`Game` owns players and enemies by value in vectors, portals as small records,
and an optional boss. It also owns the campaign RNG, event deck, phase cursors,
temporary effects, counters, logs and combat presentation events.

`Entity` implements health, resistances and movement. `Player` adds inventory,
supplies, equipment, recovery, guard and infection state. Players own weapon IDs,
not individually allocated weapon objects. `weapons()`, `characters()`,
`enemyDefinitions()` and `bosses()` return shared definition tables.

`Game::act` validates and resolves player orders. Rejected orders do not consume
actions, items or randomness. `equip` and `trade` are separate free operations.
The terminal (`main.cpp`) and SDL2 GUI (`gui/`) call the same engine. GUI animation
uses `CombatEvent` records and a presentation state machine; it does not implement
a separate combat ruleset. `Game::tutorial` creates an isolated practice campaign.

## Campaign creation and randomness

- One to four local cooperative defenders; no network multiplayer.
- Four fixed presets, or a custom 50-point build. Stat cost is `n*(n+1)/2`;
  Vitality is 1–9, Armor 0–4, other purchased stats 0–9; HP is `1 + 3*Vitality`.
- Start with two portals (four for Prism Oracle), and two enemies per defender:
  one sovereign follower and one Gate anchor, placed at starting portals.
- `Game::rng` is seeded once and serialized. All campaign rolls use it except
  Market offers, which derive a separate seed from campaign seed, round and player
  index. Viewing or reloading Market does not reroll combat or its current offers.
- `random.cpp` is an older standalone helper, not part of the current CMake engine.

## Round and action flow

`players → worldDraw → worldResolve → enemies → roundEnd → players`

Every living defender spends one action before the shared world phase. Ordinary
movement, attacks, searching, waiting, healing, guarding, suppression, taking a
Market offer and collapsing Bridge spend an action. Outgoing road moves from
Streets, inspecting, equipping and trading are free.

`act(..., false)` lets the GUI advance phases individually; the terminal can use
automatic resolution. Drawing and resolving a world card are separate states.
The hostile phase processes existing corrosion, then one enemy at a time, then
the boss. Suppression skips one activation. Upkeep adds `1 + instability` doom,
handles portal waves and manifestation, expires effects, and resets recovery and
guard. Fallen defenders are skipped; ordinary recovered defenders act next round.

## Combat

- Weapon damage is fixed by its definition, with sector modifiers. Player stats
  affect accuracy rather than adding damage. The current staff is always psychic;
  the early low-Will physical fallback is no longer implemented.
- Base accuracy is 80%, minus 20 percentage points per unmet requirement, plus
  5 points per surplus aiming-stat point, capped at 100%. Aiming uses Strength,
  Precision or Will according to the weapon; heavy Strength requirements do not
  add an aiming bonus. `Game` applies temporary and sector accuracy modifiers and
  clamps to 0–100%. A `d100 <= chance` hits.
- Armor resists physical damage; Will resists psychic damage. Damage floors at
  zero and cannot exceed current HP. True damage ignores both resistances.
- Hostiles use a d20 against 12 with their attack stat (bosses use +4); natural 1
  misses and natural 20 hits. Instability adds damage before resistance.
- Each strike independently chooses an eligible local defender. Hunters and the
  boss follow directed routes toward the nearest eligible defender when none is
  local. Moving does not also attack. Other enemies hold their sector.
- Guard subtracts 3 before resistance for the round. Ranged suppression rolls the
  weapon hit chance, does no damage, and skips the enemy's next activation.
- Breach grenades always hit: destroy a portal, or deal 5 true damage to an enemy
  or boss. Armory adds 5 true damage to successful defender attacks, including
  grenades. Unguarded portals take automatic weapon hits; guarded ones require
  accuracy rolls. Gate anchors block weapon damage but cannot block grenades.

## Board, equipment and recovery

The nine-sector directed graph is defined in `map.cpp`. Road reversals, temporary
entry closures and destroyed Bridge modify access centrally in `Game`; hunters,
the boss and frontends use those routes. Closures preserve existing connectivity
among remaining open sectors. Transport flights bypass roads and temporary
closures, but never a destroyed Bridge.

Sector bonuses: Barracks +2 Armor; Armory +5 true damage; Alley +20 ranged accuracy;
Streets free road exits; Cathedral +4 Will; Infirmary full healing and corrosion
cure on rest; Slums +2 melee damage for both sides; Market three changing offers;
Bridge permanent demolition. The README contains the full outgoing-route table.

Search never depletes a sector. Loot favors weapons in Barracks, Armory and
Cathedral, and medical supplies in Infirmary. Duplicates convert to supplies.
Kills have a 50% loot chance; non-quiet events have a 25% extra supply chance.
Supplies cap at 99. Trading requires living co-located defenders and preserves
at least one weapon for the giver; transferring the equipped weapon equips a
remaining one. Duplicate weapon transfers are rejected.

Ordinary defeat immediately restores a defender to full HP in Infirmary, clears
corrosion/guard, adds 1 instability and excludes them from further targeting that
hostile phase. The old overkill/skip-turn mechanic is absent. Corrosion deals 1
true damage per round for two rounds, refreshes rather than stacks, and begins
ticking next round. Medkits heal 8 and cure it; ordinary rest heals 5.

Bridge collapse consumes a grenade and action. Defenders on Bridge die
permanently; local enemies die and portals close. A local boss loses half its
current HP rounded up and moves to Slums. The crossing remains inaccessible;
a later boss manifests in Slums. A boss elsewhere is unaffected.

## Events, escalation and endings

Half of world draws are quiet without consuming the deck. Otherwise, 22 distinct
events are shuffled and exhausted before reshuffling. Cards change supplies,
portals, enemies, routes, closures, transport, accuracy, instability or doom;
none directly damage defenders. Only world events lower instability.

Every fifth completed round, portals spawn one enemy per living defender across
the entire board, distributed across unoccupied portals first. Waves are based
on round count, not doom. At 64 enemies, extra spawns increase instability instead.

Before manifestation, closing the last portal while enemies remain triggers last
stand: each unsuppressed enemy attempts one 10% portal summon instead of moving
or attacking. A new portal immediately restores normal behavior. Manifestation
permanently disables last stand. Echo Weaver does not double summon attempts.

Boss HP is base 50/52/50/60 plus 6 per starting defender; damage is 10 physical,
10 psychic, 7 physical twice, or 9 physical, in campaign order. Manifestation
starts an eight-round countdown. Win by clearing all portals and enemies before
manifestation, or killing the boss at any time. Lose at instability 6, all
defenders permanently fallen, or the countdown expiring. Boss death takes
priority over simultaneous squad death.

## Persistence and verification

New saves begin with `VEYRA_SAVE 6`. The loader still accepts the legacy marker
for versions 1–6. Numeric boss, weapon, enemy and event IDs retain their original
ordering, so names and artwork can change without changing saved mechanics.
The old marker occurs only in import code and migration tests, not new saves or UI.
Player-entered callsigns are preserved as user data, including in old campaigns.
Completed endings are derived from the current rules/text on load.

Saves preserve RNG, deck, active card, phases and cursors, entities, inventories,
temporary effects, offers and permanent board changes. Candidate loading validates
before replacing a running game; file writes use a temporary file and rename.
GUI saves are manual unless autosave is enabled; terminal quit/EOF saves.

Tests cover engine rules and 800 seeded campaigns, terminal input, actual SDL
mouse/text handlers, animation phases, archive behavior and screenshot capture.
Legacy-marker import tests additionally verify that phase and random continuation
remain identical under the new branding. This review is descriptive; it is not
a balance audit or legal clearance of a release.
