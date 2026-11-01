VEYRA - THE LAST WATCH
Current playable game: plain-text guide

Veyra's fractured relay network admits hostile machines from the Hollow.
The Last Watch faces four sovereign intelligences using salvaged equipment
and resonance weapons. docs/SYSTEMS.md records the systems and setting.

A local C++17 strategy RPG for 1-4 defenders sharing one computer. Play
through the native SDL2 GUI or terminal; both use the same engine and saves.
There is no network multiplayer. This implementation includes AI-assisted
code. README.md contains the full weapon, enemy, event, and route tables,
screenshots, source layout, and test instructions.

START PLAYING

On a new machine, complete "BUILD AND CHECK" step 1 below first.
play.sh builds and launches the game; it does not install dependencies.

  ./play.sh                     Build and open the GUI title screen
  ./play.sh --quick --seed 42    Start a seeded solo game against Ember Regent
  ./play.sh --cli                Build and play the terminal version
  ./play.sh --load 'patrol.sav'  Load a named campaign in the GUI

The GUI needs a C++17 compiler, CMake 3.16+, pkg-config, SDL2, FreeType,
a local TrueType/OpenType font, and Bash for the launcher. The terminal
build needs no SDL2 or FreeType. No account or asset downloads are needed.
The ./ttrpg wrapper also launches the GUI. After building, launch
./build/ttrpg_gui directly to skip the build step.

TITLE SCREEN AND CHARACTERS

- New campaign: choose a Sovereign and create a squad of 1-4 defenders.
- Quick deployment: use the title-screen 1-4 player selector to deploy
  distinct presets with varied callsigns. The lead preset rotates between
  deployments. The command-line --quick option starts only one defender.
- Tutorial: 12 guided lessons on a separate practice board. Exit tutorial
  or Escape returns to the menu without changing your campaign or save.
- Field manual: ten topics, including a dedicated Bridge collapse page.

Presets are Bulwark, Trailguard, Resonant, and Duskblade.
Use the left/right arrows to cycle in either direction. Custom characters
have a 50-point budget; increases beyond the budget are disabled. Choose
one of the presets' starting weapons. Everyone also starts with a rivet
pistol, a medkit, and a breach grenade; Trailguards get an extra grenade.
Hold Backspace to erase text repeatedly in name fields.

ROUNDS AND CONTROLS

Each living defender takes one action, then the shared world card is drawn
and flipped. Read it and choose Resolve world event. Existing corrosion ticks,
enemies activate individually, and the sovereign acts last if present. Internal
upkeep advances doom, processes reinforcements, and begins the next round.
Upkeep has no player-facing phase. The terminal resolves phases automatically.

The active defender's map icon blinks green. Click sectors to inspect them
for free. Click Move to follow an allowed outgoing route; arrowheads show
direction. Some roads are one-way. Attacks require a target in your sector.
Defender orders act at your location even while you inspect another sector.

Movement, attacking, searching, resting, medkits, grenades, Guard, Suppress,
Market purchases, and waiting normally spend your action. Equipping,
trading, inspecting, and outgoing road moves from Streets are free.
Guard reduces incoming physical/psychic hits by 3 before resistance for
that round. Suppress uses a ranged hit roll to skip an enemy's next
activation without dealing damage.

Use Equipment to equip weapons or Trade with another living defender in
your sector. Give a weapon, medkit, or grenade for free. Keep at least one
weapon; duplicate weapons cannot transfer. Supplies transfer one at a time.

GUI shortcuts: 1-9 inspect sectors; I opens Equipment; H opens the manual;
J opens the journal; F5 saves; Escape closes dialogs or opens Menu.
Menu controls animation speed. Cards wait for confirmation; phase banners
close automatically. Actions have animations. Low health flashes a red
outline on the defender panel; the battlefield UI turns red for the sovereign.

COMBAT AND RECOVERY

Weapon hit chance starts at 80% when requirements are met exactly.
Subtract 20 percentage points per unmet requirement, regardless of how
many stat points are missing. Add 5 percentage points per surplus point
in the aiming stat: Strength for melee, Precision for ranged, Will for
psychic weapons. Heavy weapon Strength is a requirement, not an extra
accuracy bonus. Final chance is capped between 0% and 100%.
Example: a Strength 5 defender using a Strength 2 sword has 95% accuracy.

Weapons have fixed base damage, modified by sector bonuses. Armor reduces
physical damage; Will reduces psychic damage; true damage ignores both.
Breach grenades always hit: they destroy a portal outright or deal 5 true
damage to an enemy or sovereign (10 in Armory). Gate anchors block weapon damage
to local portals but cannot stop grenades. Unguarded portals are automatic
weapon hits; guarded portals require the normal weapon hit roll.

Ordinary defeat sends a defender to Infirmary at full HP, clears corrosion, and
adds 1 instability. They cannot be attacked again during that hostile phase.
Bridge collapse is the permanent-death exception described below.
Rest heals 5 HP; Infirmary rest fully heals and cures corrosion. A medkit heals
8 HP and cures corrosion. Corrosion deals 1 true damage per round for its duration.
Only world events can lower instability; there are no prayer or seal actions.

SECTORS

  1 Barracks   +2 armor for defenders.
  2 Armory     +5 true damage on successful defender hits, including grenades.
  3 Alley      +20 percentage points to defender ranged accuracy.
  4 Streets    Outgoing road movement is free. Non-road flights cost an action.
  5 Cathedral  +4 Will for defenders, including resistance and weapon accuracy.
  6 Infirmary  Rest fully heals and cures corrosion.
  7 Slums      +2 physical damage to all melee attacks, including enemies/sovereigns.
  8 Market     Choose one of three items for an action. Offers refresh each
               defender turn; viewing or loading does not reroll them.
  9 Bridge     Can be destroyed permanently with a breach grenade.

BRIDGE COLLAPSE

Stand on Bridge with a breach grenade. Choose Collapse Bridge and confirm.
This costs one grenade and your action. Throwing a grenade at a target
does not destroy the Bridge.

- If the sovereign is ON BRIDGE when it collapses, it loses half its CURRENT HP,
  ignoring resistance, and moves to Slums. Damage rounds up: 56 HP becomes
  28; 51 HP becomes 25. A sovereign elsewhere takes no damage and does not move.
- Every defender on Bridge dies PERMANENTLY, including the acting defender.
  There is no Infirmary recovery or instability increase. Regular enemies there
  die, and local portals close.
- Bridge stays inaccessible for the rest of the campaign, even during
  Transport crew. With normal roads, Slums loses its only exit. Reversed
  roads, Emergency Teleport, or Transport crew can offer an escape.
- A sovereign that appears after an earlier collapse starts in Slums without
  taking collapse damage. Fallen defenders are skipped in turn order.
- Losing the whole squad ends the campaign unless the sovereign also dies;
  killing the sovereign takes victory priority. Collapse and deaths persist
  across saves.

PORTALS, EVENTS, AND ENDINGS

Portals have 6 HP. Campaigns start with two portals, or four against
Prism Oracle, and TWO enemies per starting defender. Every fifth completed
round, remaining portals spawn ONE enemy per LIVING defender globally,
regardless of portal count. Portals without defenders receive priority.
Reinforcements begin acting the following round.

Before the sovereign appears, destroying all portals while enemies remain starts
last stand. The UI explains the change. Each enemy holds position and has
one 10% summon chance per activation instead of attacking or moving.
Suppression skips that activation. A successful summon restores normal
behavior immediately. Once the sovereign appears, last stand is permanently off.

World draws have a 50% chance of A Moment of Silence. Otherwise a shuffled
deck supplies 22 different events before reshuffling. Events can destroy
portals or enemies, reverse roads, spawn threats, grant supplies, lower
instability, close sectors temporarily, transport defenders, or alter accuracy.
No world card directly damages defenders. Benefits apply to living
defenders and never revive permanent casualties. Transport crew permits
flights to any sector except a destroyed Bridge through the next defender
phase, ignoring roads and temporary closures.

Doom rises by 1 + instability each round; Lost Hours adds 2 more. The sovereign manifests
at its doom limit: Ember Regent 24, Prism Oracle 32, Echo Weaver 28, Rust Gardener 30. Boss base
HP is respectively 50, 52, 50, and 60, plus 6 per STARTING defender. Each
patron has two follower types and distinct rules; generic enemies include
Anchors, hunters, and psychic attackers. Echo Weaver grants double strikes;
Rust Gardener's damaging hits inflict corrosion.

Win by closing every portal and defeating every enemy, or by killing the
sovereign. Killing the sovereign wins immediately regardless of other enemies/portals.
Lose at instability 6, when every defender is permanently dead, or after eight
further rounds finish with the manifested sovereign still alive.

SAVING AND EXITING

New saves use the VEYRA_SAVE header. Earlier campaigns still load with
their original numeric IDs, callsigns and random state. Saving them writes
the new header; keep a separate copy for use with older executables.

GUI saving is manual by default: Save, F5, or Archive writes a campaign.
Menu offers separate save-and-exit and WITHOUT-saving choices for both
title and desktop. Closing the window opens Menu. Optional Autosave writes
accepted orders, trades, equipment changes, and phase advances. Quitting
without saving cannot undo autosaves that have already been written.

Default file: campaign.sav in the working directory (the project directory
when using play.sh). Archive selects a named slot. Terminal commands include
save, save my patrol.sav, load my patrol.sav, and quit. Paths can contain
spaces; enter them without quotes in the prompt. Terminal quit or end of
input saves; terminal play does not autosave every turn.

Current save format: 6. Versions 1-5 can also be loaded. Saves preserve the
turn phase, unresolved card, random state, Market offers, destroyed Bridge,
and permanent deaths. Invalid loads leave the active campaign intact.

BUILD AND CHECK

These commands use Linux and Bash. Open a terminal in the project folder
containing CMakeLists.txt and play.sh.

1. Install dependencies (Debian/Ubuntu):

  sudo apt update
  sudo apt install build-essential cmake pkg-config libsdl2-dev libfreetype6-dev fonts-dejavu-core python3

Other distributions need equivalent C++17 build tools, CMake 3.16+,
pkg-config, SDL2 and FreeType development files, and a local font.
Python 3 is optional; it enables frontend integration tests. Dependency
installation requires network access; subsequent builds and play do not.

Check that the tools and GUI development libraries are available:

  cmake --version
  g++ --version
  pkg-config --modversion sdl2 freetype2

If play.sh fails at line 15 with "cmake: command not found", CMake is
missing from your terminal's PATH. Install the dependencies above, confirm
cmake --version works, then retry bash ./play.sh from the project folder.
This failure occurs before game code compiles. The CLI also needs CMake.

If CMake cannot find SDL2 or FreeType, install pkg-config, libsdl2-dev and
libfreetype6-dev on Debian/Ubuntu. Runtime libraries alone are not enough.
A build using temporary tools elsewhere does not install dependencies on
your machine. No script edit is needed for the missing CMake command.

2. Build and launch the GUI in one step:

  bash ./play.sh

Or configure, build, and launch separately:

  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=ON -DBUILD_TESTING=ON
  cmake --build build --parallel
  ./build/ttrpg_gui

Rerun the build command after editing source files. To play an existing
build, run ./build/ttrpg_gui directly. If no font is found, use:

  ./build/ttrpg_gui --font /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf

3. For a terminal-only build:

  cmake -S . -B build-cli -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=OFF -DBUILD_TESTING=ON
  cmake --build build-cli --parallel
  ./build-cli/ttrpg

The shortcut is bash ./play.sh --cli. This version needs only the compiler,
build tools, and CMake (Debian/Ubuntu: build-essential cmake). Python 3 is
optional for tests; SDL2, FreeType, pkg-config, and fonts are unnecessary.

4. Run tests after building:

  ctest --test-dir build --output-on-failure

For terminal-only tests, replace build with build-cli. Configure with
-DBUILD_TESTING=OFF to omit tests. The game itself does not need Python.
See README.md for test coverage, sanitizer builds, screenshots, and full
gameplay tables.
