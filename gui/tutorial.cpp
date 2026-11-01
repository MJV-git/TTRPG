#include "app.h"
#include <algorithm>

using namespace ui;

namespace {
struct Lesson { const char* title; const char* text; const char* practice; };
const std::array<Lesson, 12> lessons {{
    {"Your watch begins", "Save the city by closing every portal and defeating every enemy, or by defeating the manifested Sovereign.\n\nThis is an isolated practice board. Each lesson resets it so you can experiment without risking a campaign.\n\nUse Next to learn one mechanic at a time. Exit tutorial or Escape returns to the menu at any point.", ""},
    {"Follow the arrows", "Numbered teal tokens are defenders. Violet rifts are portals; red labels mark enemies.\n\nRoad arrows show permitted travel. Some routes work in only one direction. Inspecting a sector is free; moving spends your action except for road exits from Streets.\n\nTry moving from Barracks (1) to Armory (2) and watch your token travel.", "Move to Armory"},
    {"Spend one action", "Each defender gets one order per round. Moving, attacking, searching, healing, guarding and waiting all spend that order.\n\nSearch finds weapons or supplies. Armory is a good place to find weapons. Sectors never run out of search opportunities.\n\nTry Search and read the result in the log.", "Search Armory"},
    {"Choose your weapon", "Equipment changes are free during your turn. Compare damage, requirements and your actual hit chance.\n\nBase accuracy is 80%. Every point above the aiming stat requirement adds 5 points, up to 100%. Every unmet requirement costs 20 points.\n\nMelee uses Strength; ranged uses Precision; psychic uses Will. Heavy weapons also require Strength. Open Equipment and try switching weapons.", "Open Equipment"},
    {"Attack a local enemy", "You must share a sector with your target. Select a contact, then attack.\n\nArmor reduces physical damage; Will reduces psychic damage. A higher damage weapon can punch through resistance.\n\nTry shooting the Scavenger. Watch the projectile and the hit or miss result. The displayed chance is the actual roll used.", "Attack the Scavenger"},
    {"Close a portal", "Portals have 6 HP. Unguarded portals are automatic weapon hits. Enemies guarding them require a hit roll; a Gate anchor blocks weapon damage entirely.\n\nBreach grenades always destroy a portal, even through an Anchor shield. Against enemies or bosses they deal 5 true damage, plus any Armory bonus.\n\nTry collapsing the Armory portal.", "Throw breach at portal"},
    {"Defensive orders", "Guard reduces each incoming enemy hit by 3 for the round, before Armor or Will applies. It does not prevent existing corrosion damage.\n\nSuppress uses a ranged hit roll to make one local enemy skip its next activation, including a ritual. It deals no damage.\n\nTry Guard and watch the shield form.", "Take cover"},
    {"Stay alive", "Rest restores 5 HP. At the Infirmary it fully heals and cures corrosion. A medkit restores 8 HP and cures corrosion anywhere.\n\nOrdinary defeat sends you to Infirmary and adds 1 instability. Bridge collapse kills defenders permanently. Only a world event can reduce instability. Instability 6 loses the game.\n\nTry healing the wounded training defender.", "Use a medkit"},
    {"Read the world card", "After all defenders act, one shared world card is drawn and flipped. It waits for you to resolve it.\n\nHalf of draws are a quiet round. Other cards alter roads, portals, enemies or supplies. Transport crew lets each defender travel to any sector during the next defender phase.\n\nTry drawing a card, then click Resolve world event when ready.", "Draw a practice card"},
    {"Watch the hostiles", "Enemies activate individually. Watch the preview, action animation and result. Hunters pursue along roads; other enemies hold positions. Different sovereigns send different followers.\n\nRot ticks before attacks. Every fifth round, portals spawn one enemy per defender globally, preferring sectors without defenders. Doom advances automatically by 1 + instability.\n\nTry watching a hostile phase on this practice board.", "Watch the Scavenger act"},
    {"The last stand", "Before the sovereign arrives, closing the final portal makes surviving enemies stop fighting and moving. Each has a 10% chance per activation to summon a new portal.\n\nThe UI turns crimson and explains the change. A successful summon immediately restores normal enemy behavior. Suppression can stop an attempt.\n\nThe sovereign's arrival ends last stand permanently. Enemies then fight normally even without portals.", ""},
    {"Face the sovereign", "When doom reaches the deadline, the Sovereign manifests and the UI turns red. Sovereigns have at least 50 HP, plus squad scaling, and powerful attacks.\n\nYou have eight more rounds to defeat it. Killing the sovereign immediately wins, even with other enemies or portals remaining.\n\nSave manually using Save or F5. Menu includes exits without saving to title or desktop. You are ready to deploy.", ""}
}};
}

void Gui::startTutorial() {
    tutorialReturnGame = std::move(game); tutorialReturnHistory = std::move(history);
    screen = Screen::battle; setTutorialStep(0);
}
void Gui::setTutorialStep(int step) {
    tutorialStep = step; tutorialPracticed = false;
    game = Game::tutorial(step < 2 ? 1 : 2);
    modal = Modal::none; clearFocus(); history.clear(); notice.clear();
    beforeAnimation.reset(); gatherLog(); syncSelection(); beginStage(Presentation::idle);
}
void Gui::exitTutorial() {
    game = std::move(tutorialReturnGame); tutorialReturnGame.reset();
    history = std::move(tutorialReturnHistory); tutorialStep = -1;
    screen = Screen::title; modal = Modal::none; beforeAnimation.reset();
    clearFocus(); notice.clear(); beginStage(Presentation::idle);
}
void Gui::tutorialOverlay() {
    // Preserve only lesson interactions; campaign/save controls cannot affect training.
    buttons.erase(std::remove_if(buttons.begin(), buttons.end(), [](const Button& b) {
        return b.id != "card-resolve" && b.id != "skip-animation" && b.id != "inventory-close" &&
            b.id.rfind("equip-", 0) != 0 && b.id.rfind("inventory-", 0) != 0;
    }), buttons.end());
    button("tutorial-exit", {1190, 24, 226, 42}, "Exit tutorial", [this] { exitTutorial(); }, true, true);
    if (modal == Modal::inventory) return;
    const auto& lesson = lessons.at(tutorialStep);
    draw.fill({1004, 170, 412, 742}, panel); draw.outline({1004, 170, 412, 742}, gold);
    draw.label(1026, 193, "TUTORIAL / " + std::to_string(tutorialStep + 1) + " OF 12", 13, gold);
    draw.paragraph(1026, 231, lesson.title, 366, 28, text, 2);
    draw.paragraph(1026, 309, lesson.text, 366, 18, text, 18);
    if (*lesson.practice) button("tutorial-practice", {1026, 754, 366, 44}, tutorialPracticed ? "Practice complete" : lesson.practice, [this] {
        tutorialPracticed = true;
        switch (tutorialStep) {
            case 1: perform({ActionType::move, 2}); break;
            case 2: perform({ActionType::search}); break;
            case 3: openModal(Modal::inventory); tutorialPracticed = false; break;
            case 4: perform({ActionType::attackEnemy, game->getEnemies().front().getId()}); break;
            case 5: perform({ActionType::grenadePortal}); break;
            case 6: perform({ActionType::guard}); break;
            case 7: perform({ActionType::medkit}); break;
            case 8:
                game->act({ActionType::wait}, false); gatherLog(); beginStage(Presentation::drawing); break;
            case 9:
                game->act({ActionType::wait}, false); advanceGamePhase(); advanceGamePhase();
                beginStage(Presentation::enemyIntro); break;
        }
    }, !tutorialPracticed && acceptsOrders(), true);
    button("tutorial-prev", {1026, 826, 118, 44}, "Previous", [this] { setTutorialStep(tutorialStep - 1); }, tutorialStep > 0);
    button("tutorial-next", {1156, 826, 236, 44}, tutorialStep == 11 ? "Finish tutorial" : "Next lesson", [this] {
        if (tutorialStep == 11) exitTutorial(); else setTutorialStep(tutorialStep + 1);
    }, true, true);
    draw.label(1026, 886, "Practice is optional. Esc exits anytime.", 13, muted);
}
