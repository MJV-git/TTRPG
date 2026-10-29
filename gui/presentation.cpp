#include "app.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
using namespace ui;
const std::array<SDL_Point, 9> points {{{164, 252}, {466, 252}, {800, 252}, {800, 422}, {466, 422}, {164, 422}, {164, 594}, {800, 594}, {466, 594}}};
float smooth(float t) { t = std::clamp(t, 0.0F, 1.0F); return t*t*(3 - 2*t); }
Color cardInk {41, 39, 35}, paper {221, 213, 192};
}

bool Gui::acceptsOrders() const {
    return game && game->getOutcome() == Outcome::ongoing && game->getPhase() == TurnPhase::players && presentation == Presentation::idle;
}
void Gui::beginStage(Presentation value) { presentation = value; stageStarted = animationClock; }
void Gui::advanceGamePhase() {
    beforeAnimation = *game;
    game->advancePhase(); gatherLog();
    if (autoSave) save();
}
void Gui::resumePresentation() {
    beforeAnimation.reset();
    if (!game || game->getOutcome() != Outcome::ongoing) { beginStage(Presentation::idle); return; }
    switch (game->getPhase()) {
        case TurnPhase::players: beginStage(Presentation::handoff); break;
        case TurnPhase::worldDraw: beginStage(Presentation::drawing); break;
        case TurnPhase::worldResolve: beginStage(Presentation::card); break;
        case TurnPhase::enemies: beginStage(Presentation::enemyIntro); break;
        case TurnPhase::roundEnd: finishInternalRound(); break;
    }
}
void Gui::prepareEnemy() {
    // Skip empty bookkeeping steps; every enemy activation gets a visible beat.
    for (int i = 0; i < 68 && game->getPhase() == TurnPhase::enemies && game->getOutcome() == Outcome::ongoing; ++i) {
        intent = game->enemyIntent();
        if (intent.active) { beginStage(Presentation::enemyPreview); return; }
        advanceGamePhase();
    }
    if (game->getOutcome() != Outcome::ongoing) beginStage(Presentation::idle);
    else finishInternalRound();
}
void Gui::finishInternalRound() {
    if (game->getPhase() == TurnPhase::roundEnd && game->getOutcome() == Outcome::ongoing) advanceGamePhase();
    syncSelection(); beforeAnimation.reset();
    beginStage(game->getOutcome() == Outcome::ongoing ? Presentation::handoff : Presentation::idle);
}
void Gui::updatePresentation(Uint32 now) {
    const Uint32 delta = now - lastTick; lastTick = now; animationClock = now;
    if (modal != Modal::none || screen != Screen::battle) { stageStarted += delta; return; }
    if (!game || presentation == Presentation::idle || presentation == Presentation::card) return;
    const Uint32 elapsed = now - stageStarted;
    const float speed = animationScale();
    auto passed = [&](int milliseconds) { return elapsed >= static_cast<Uint32>(milliseconds * speed); };
    switch (presentation) {
        case Presentation::handoff:
            if (passed(800)) { beforeAnimation.reset(); beginStage(Presentation::idle); }
            break;
        case Presentation::action:
            if (!passed(std::max(1100, static_cast<int>(game->getCombatEvents().size()) * 1300))) break;
            syncSelection();
            if (tutorialStep >= 0) { beginStage(Presentation::idle); break; }
            if (game->getOutcome() != Outcome::ongoing) beginStage(Presentation::idle);
            else beginStage(game->getPhase() == TurnPhase::players ? (game->getPlayerIndex() == static_cast<std::size_t>(actingPlayer) ? Presentation::idle : Presentation::handoff) : Presentation::drawing);
            break;
        case Presentation::drawing:
            if (passed(850)) { advanceGamePhase(); beginStage(Presentation::flipping); }
            break;
        case Presentation::flipping:
            if (passed(1000)) beginStage(Presentation::card);
            break;
        case Presentation::worldEffect:
            if (passed(1500)) {
                if (game->getOutcome() != Outcome::ongoing) beginStage(Presentation::idle);
                else beginStage(Presentation::enemyIntro);
            }
            break;
        case Presentation::enemyIntro:
            if (passed(950)) prepareEnemy();
            break;
        case Presentation::enemyPreview:
            if (passed(950)) { advanceGamePhase(); beginStage(Presentation::enemyResult); }
            break;
        case Presentation::enemyResult:
            if (passed(std::max(1250, static_cast<int>(game->getCombatEvents().size()) * 1300))) prepareEnemy();
            break;
        default: break;
    }
}
void Gui::phaseRail() {
    const std::array<const char*, 3> names {{"DEFENDERS", "WORLD CARD", "HOSTILES"}};
    int active = 0;
    if (presentation >= Presentation::drawing && presentation <= Presentation::worldEffect) active = 1;
    if (presentation >= Presentation::enemyIntro && presentation <= Presentation::enemyResult) active = 2;
    for (int i = 0; i < 3; ++i) {
        const SDL_Rect cell {405 + i * 130, 24, 122, 42};
        draw.fill(cell, i == active ? Color{44, 41, 34} : panel);
        draw.outline(cell, i == active ? gold : border);
        draw.label(cell.x + 10, cell.y + 14, names[i], 11, i == active ? gold : muted);
        if (i < 2) draw.label(cell.x + 123, cell.y + 12, ">", 10, border);
    }
}
void Gui::eventArtwork(int art, int x, int y) {
    const Color accent = art == 3 ? Color{108, 71, 123} : art == 2 ? Color{142, 68, 62} : Color{91, 95, 82};
    draw.fill({x, y, 376, 174}, {202, 194, 173});
    for (int i = 0; i < 16; ++i) {
        int bx = x + 8 + i*23, by = y + 125 - (i*37 % 70);
        draw.fill({bx, by, 17, y + 164 - by}, {179, 174, 157});
        draw.outline({bx, by, 17, y + 164 - by}, {157, 154, 141});
    }
    const int cx = x + 188, cy = y + 80;
    if (art == 3) {
        for (int r : {29, 43, 58, 63}) draw.circle(cx, cy, r, accent);
        for (int i = 0; i < 12; ++i) {
            const float a = static_cast<float>(i) * 6.2831853F / 12;
            draw.line(cx + static_cast<int>(48*std::cos(a)), cy + static_cast<int>(48*std::sin(a)), cx + static_cast<int>(74*std::cos(a + 0.13F)), cy + static_cast<int>(74*std::sin(a + 0.13F)), accent);
        }
        draw.icon(cx, cy, Icon::portal, accent, 2);
    } else if (art == 4) {
        draw.arrow(cx - 142, cy - 24, cx + 140, cy - 24, accent);
        draw.arrow(cx + 142, cy + 25, cx - 140, cy + 25, accent);
        for (int i : {-1, 0, 1}) draw.outline({cx + i*100 - 19, cy - 43, 38, 86}, accent);
    } else if (art == 5) {
        draw.circle(cx, cy, 62, accent); draw.circle(cx, cy, 58, accent);
        for (int i = 0; i < 12; ++i) {
            float a = static_cast<float>(i) * 6.2831853F / 12;
            draw.line(cx + static_cast<int>(49*std::cos(a)), cy + static_cast<int>(49*std::sin(a)), cx + static_cast<int>(56*std::cos(a)), cy + static_cast<int>(56*std::sin(a)), accent);
        }
        draw.line(cx, cy, cx + 30, cy - 30, accent); draw.line(cx, cy, cx, cy - 43, accent);
    } else if (art == 1) {
        for (int i : {-1, 0, 1}) {
            const int bx = cx + i*90;
            draw.outline({bx - 28, cy + 10, 56, 50}, accent); draw.icon(bx, cy + 34, Icon::supplies, accent);
            draw.line(bx - 28, cy + 10, bx - 42, cy - 35, accent); draw.line(bx + 28, cy + 10, bx + 42, cy - 35, accent);
            draw.line(bx - 42, cy - 35, bx, cy - 58, accent); draw.line(bx, cy - 58, bx + 42, cy - 35, accent);
        }
    } else if (art == 2) {
        for (int i = -1; i <= 1; ++i) {
            const int bx = cx + i*76;
            draw.line(bx - 23, cy - 53, bx + 21, cy + 48, accent);
            draw.line(bx + 21, cy + 48, bx + 9, cy - 20, accent);
        }
        draw.circle(cx - 110, cy + 8, 8, accent, true); draw.circle(cx + 110, cy + 8, 8, accent, true);
    } else {
        for (int i = 0; i < 16; ++i) {
            const float a = static_cast<float>(i) * 6.2831853F / 16;
            draw.line(cx + static_cast<int>(43*std::cos(a)), cy + static_cast<int>(43*std::sin(a)), cx + static_cast<int>(75*std::cos(a)), cy + static_cast<int>(75*std::sin(a)), accent);
        }
        draw.icon(cx, cy, Icon::insignia, accent, 3);
    }
}
SDL_Texture* Gui::eventTexture(bool front) {
    const int id = game ? game->getActiveCard() : -1;
    if (front && cardFace && cachedCard == id) return cardFace.get();
    if (!front && cardBack) return cardBack.get();
    auto& storage = front ? cardFace : cardBack;
    storage.reset(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 440, 580));
    if (!storage) throw std::runtime_error(SDL_GetError());
    SDL_SetTextureBlendMode(storage.get(), SDL_BLENDMODE_BLEND);
    SDL_Texture* original = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, storage.get()); SDL_RenderSetLogicalSize(renderer, 440, 580);
    draw.fill({0, 0, 440, 580}, front ? paper : Color{37, 43, 48});
    draw.outline({5, 5, 430, 570}, front ? Color{127, 117, 94} : gold);
    draw.outline({12, 12, 416, 556}, front ? Color{165, 151, 120} : Color{100, 86, 62});
    for (int y = 18; y < 565; y += 7) draw.line(18, y, 422, y, front ? Color{95, 83, 61, 7} : Color{150, 135, 105, 12});
    for (const auto& corner : std::array<SDL_Point, 4>{{{22, 22}, {418, 22}, {22, 558}, {418, 558}}}) draw.circle(corner.x, corner.y, 4, gold);
    if (front && id >= 0) {
        const auto& card = eventCards().at(id);
        draw.label(32, 29, "WORLD EVENT / " + card.category, 12, {113, 82, 59});
        draw.paragraph(30, 58, card.name, 376, 30, cardInk, 2);
        eventArtwork(card.art, 32, 140);
        draw.label(32, 332, "ON RESOLUTION", 12, {113, 82, 59});
        draw.paragraph(32, 357, card.effect, 376, 18, cardInk, 6);
        draw.line(32, 511, 408, 511, {158, 143, 111});
        cachedCard = id;
    } else {
        draw.label(220 - draw.measure("THE LAST WATCH", 23)/2, 86, "THE LAST WATCH", 23, gold);
        draw.circle(220, 287, 102, gold); draw.circle(220, 287, 94, {102, 88, 64});
        draw.icon(220, 287, Icon::insignia, gold, 5);
        draw.label(220 - draw.measure("FATE OF THE DISTRICT", 14)/2, 454, "FATE OF THE DISTRICT", 14, gold);
        draw.label(220 - draw.measure("WORLD EVENT", 12)/2, 491, "WORLD EVENT", 12, muted);
    }
    SDL_SetRenderTarget(renderer, original); SDL_RenderSetLogicalSize(renderer, Width, Height);
    return storage.get();
}
void Gui::boardEffects() {
    if (presentation == Presentation::idle || presentation == Presentation::handoff || !game) return;
    combatAnimation();
    actionAnimation();
    float progress = std::clamp((animationClock - stageStarted) / (900.0F * animationScale()), 0.0F, 1.0F);
    const float pulse = 0.5F + 0.5F * std::sin(progress * 6.2831853F);
    if ((presentation == Presentation::enemyPreview || presentation == Presentation::enemyResult) && intent.sector > 0) {
        const auto at = points[intent.sector - 1];
        draw.outline({at.x - 109, at.y - 59, 218, 118}, {222, 112, 107, static_cast<unsigned char>(130 + pulse*125)});
    }
    if (!beforeAnimation || (presentation != Presentation::action && presentation != Presentation::worldEffect &&
        presentation != Presentation::enemyResult)) return;
    for (std::size_t i = 0; i < game->getPlayers().size(); ++i) {
        const auto& p = game->getPlayers()[i]; const auto& old = beforeAnimation->getPlayers()[i];
        const auto a = points[old.getSector() - 1], b = points[p.getSector() - 1];
        if (old.getSector() != p.getSector()) {
            const float t = smooth(progress);
            const int x = static_cast<int>(a.x + (b.x - a.x)*t), y = static_cast<int>(a.y + (b.y - a.y)*t);
            draw.circle(x, y, 18, {9, 25, 26}, true); draw.circle(x, y, 18, teal);
            draw.icon(x, y, defenderIcon(p), teal, 0.85F);
            draw.label(x + 16, y + 8, std::to_string(i + 1), 11, teal);
        }
        const int delta = p.getHealth() - old.getHealth();
        if (delta != 0 && game->getCombatEvents().empty()) {
            const int y = b.y - 65 - static_cast<int>(18*progress);
            draw.fill({b.x - 45, y - 3, 90, 29}, background);
            draw.label(b.x - 37, y, (delta > 0 ? "+" : "") + std::to_string(delta) + " HP", 18, delta < 0 ? red : teal);
        }
    }
    if (presentation == Presentation::enemyResult && intent.sector > 0 && intent.destination != intent.sector) {
        const auto a = points[intent.sector - 1], b = points[intent.destination - 1];
        const float t = smooth(progress);
        const int x = static_cast<int>(a.x + (b.x - a.x)*t), y = static_cast<int>(a.y + (b.y - a.y)*t);
        draw.circle(x, y, 19, {43, 22, 23}, true); draw.circle(x, y, 19, red);
        draw.icon(x, y, actorIcon(intent.name, false), red);
    }
    if (presentation == Presentation::worldEffect) {
        for (int sector = 1; sector <= 9; ++sector) {
            const auto at = points[sector - 1];
            const auto count = [sector](const Game& state) {
                return std::count_if(state.getEnemies().begin(), state.getEnemies().end(), [sector](const Enemy& e) { return e.getSector() == sector; }) +
                       std::count_if(state.getPortals().begin(), state.getPortals().end(), [sector](const Portal& p) { return p.sector == sector; });
            };
            if (count(*game) != count(*beforeAnimation) || game->getReversedRounds() != beforeAnimation->getReversedRounds() || game->getClosure(sector) != beforeAnimation->getClosure(sector))
                draw.outline({at.x - 109, at.y - 59, 218, 118}, {169, 128, 203, static_cast<unsigned char>(125 + 130*pulse)});
        }
    }
}
void Gui::actionAnimation() {
    if (!beforeAnimation || (!game->getCombatEvents().empty() && animatedAction != ActionType::collapseBridge) ||
        (presentation != Presentation::action && presentation != Presentation::enemyResult)) return;
    const bool player = presentation == Presentation::action;
    const float t = std::clamp((animationClock - stageStarted) / (1100.0F * animationScale()), 0.0F, 1.0F);
    if (player && animatedAction == ActionType::collapseBridge) {
        const auto at=points[8];
        draw.circle(at.x,at.y,static_cast<int>(18+90*t),{239,126,70,static_cast<Uint8>(210*(1-t))});
        for (int i=0;i<12;++i) {
            const float angle=i*6.2831853F/12;
            const int x=at.x+static_cast<int>((14+65*t)*std::cos(angle));
            const int y=at.y+static_cast<int>((8+35*t)*std::sin(angle)+35*t*t);
            draw.fill({x,y,6,5},i%2 ? red : gold);
        }
        draw.icon(at.x,at.y,Icon::skull,red,1.5F);
        return;
    }
    const bool ritual = !player && beforeAnimation->isLastStand();
    const bool suppressed = !player && intent.description.find("Suppressed") == 0;
    auto effect = [&](int sector, const std::string& label, int kind) {
        const auto at = points[sector - 1];
        const Color ink = ritual && !suppressed ? purple : player ? teal : red;
        const int x = at.x, y = at.y;
        draw.circle(x, y, 20 + static_cast<int>(32*t), ink);
        if (kind == 0) { // Scanning searchlight.
            const float angle = t * 6.2831853F;
            draw.line(x, y, x + static_cast<int>(48*std::cos(angle)), y + static_cast<int>(48*std::sin(angle)), gold);
            draw.icon(x, y, Icon::search, gold);
        } else if (kind == 1) { // Medical cross rises from the token.
            const int cy = y - static_cast<int>(22*t);
            draw.icon(x, cy, Icon::infirmary, teal, 1.3F);
        } else if (kind == 2) { // Shield closes around the defender.
            draw.icon(x, y, Icon::shield, ink, 1.0F + 0.5F*t);
        } else if (kind == 3) { // Portal ritual, followed by its actual result.
            for (int i = 0; i < 6; ++i) {
                const float a = t*6.2831853F + i*1.04719755F;
                draw.circle(x + static_cast<int>(30*std::cos(a)), y + static_cast<int>(30*std::sin(a)), 4, purple, true);
            }
            draw.icon(x, y, Icon::portal, purple);
        } else if (kind == 4) {
            draw.line(x-20, y-20, x+20, y+20, ink); draw.line(x+20, y-20, x-20, y+20, ink);
        } else { // Holding position: sweep of a watch dial.
            draw.icon(x, y, Icon::clock, ink, 1.4F);
            const float a = t*6.2831853F;
            draw.line(x, y, x + static_cast<int>(19*std::sin(a)), y - static_cast<int>(19*std::cos(a)), ink);
        }
        draw.fill({x - 104, y + 54, 208, 26}, background);
        draw.label(x - draw.measure(label, 13)/2, y + 59, label, 13, ink);
    };
    if (player) {
        if (animatedAction == ActionType::move) return; // Token interpolation is drawn by boardEffects.
        const int sector = beforeAnimation->getPlayers().at(actingPlayer).getSector();
        const int kind = (animatedAction == ActionType::search || animatedAction == ActionType::market) ? 0 :
            (animatedAction == ActionType::rest || animatedAction == ActionType::medkit) ? 1 : animatedAction == ActionType::guard ? 2 : animatedAction == ActionType::collapseBridge ? 4 : 5;
        effect(sector, actionLabel, kind);
    } else if (intent.sector > 0 && intent.destination == intent.sector) {
        const std::string label = suppressed ? "SUPPRESSED / SKIPPED" : ritual ?
            (t < 0.65F ? "SUMMONING..." : game->getPortals().empty() ? "RITUAL FAILED" : "PORTAL SUMMONED") : "HOLDING POSITION";
        effect(intent.sector, label, suppressed ? 4 : ritual ? 3 : 5);
    } else if (intent.sector == 0) {
        for (const auto& p : beforeAnimation->getPlayers()) if (p.getPoison() > 0) effect(p.getSector(), "LINGERING ROT", 4);
    }
}
ui::Icon Gui::actorIcon(const std::string& name, bool defender) const {
    const auto& state = beforeAnimation ? *beforeAnimation : *game;
    if (defender) {
        for (const auto& p : state.getPlayers()) if (p.getName() == name) return ui::defenderIcon(p);
        return ui::Icon::defender;
    }
    if (state.getBoss() && state.getBoss()->getName() == name)
        return ui::patronIcon(static_cast<int>(state.getBoss()->getKind()));
    for (const auto& e : state.getEnemies())
        if (name == e.getName() || name == e.getName() + " #" + std::to_string(e.getId())) return ui::enemyIcon(e.getKind());
    return ui::Icon::cultist;
}
void Gui::combatAnimation() {
    if (presentation != Presentation::action && presentation != Presentation::enemyResult) return;
    const auto& beats = game->getCombatEvents();
    if (beats.empty()) return;
    const float elapsed = (animationClock - stageStarted) / animationScale();
    const std::size_t index = std::min(beats.size() - 1, static_cast<std::size_t>(elapsed / 1300));
    const auto& beat = beats[index];
    const float t = std::clamp((elapsed - index * 1300) / 1300, 0.0F, 1.0F);
    const auto at = points[beat.sector - 1];
    const Color color = beat.style == CombatStyle::psychic ? purple : beat.defenderAttack ? teal : red;
    const int ax = at.x - 58, tx = at.x + 58, y = at.y + 10;
    draw.fill({at.x - 94, at.y - 48, 188, 96}, {17, 25, 29, 240});
    draw.outline({at.x - 94, at.y - 48, 188, 96}, color);
    draw.label(at.x - 80, at.y - 39, "COMBAT / " + std::to_string(beat.sector), 12, color);
    draw.circle(ax, y, 13, color); draw.circle(tx, y, 13, beat.defenderAttack ? red : teal);
    draw.icon(ax, y, actorIcon(beat.attacker, beat.defenderAttack), color, 0.85F);
    draw.icon(tx, y, beat.target == "Warp portal" ? Icon::portal : actorIcon(beat.target, !beat.defenderAttack),
        beat.defenderAttack ? red : teal, 0.85F);
    const float flight = smooth(t / 0.48F);
    const int x = static_cast<int>(ax + (tx - ax) * flight);
    if (t < 0.48F) {
        if (beat.style == CombatStyle::grenade) {
            draw.icon(x, y - static_cast<int>(35 * std::sin(flight * 3.14159265F)), Icon::grenade, gold, 0.55F);
        } else if (beat.style == CombatStyle::melee) {
            draw.line(x - 15, y + 18, x + 15, y - 18, color);
            draw.line(x - 11, y + 18, x + 19, y - 18, color);
        } else if (beat.style == CombatStyle::psychic) {
            draw.circle(x, y, 8, purple); draw.circle(x, y, 4, purple, true);
            draw.line(ax, y, x, y - 4, purple);
        } else {
            for (int offset : {-5, 0, 5}) draw.line(x - 18, y + offset, x, y + offset, color);
            draw.circle(ax + 13, y, 6, gold);
        }
    } else {
        const int radius = 12 + static_cast<int>((t - 0.48F) * 65);
        if (beat.hit) {
            draw.circle(tx, y, radius, beat.style == CombatStyle::grenade ? gold : color);
            for (int i = 0; i < 8; ++i) {
                const float a = i * 6.2831853F / 8;
                draw.line(tx + static_cast<int>(13*std::cos(a)), y + static_cast<int>(13*std::sin(a)),
                    tx + static_cast<int>(radius*std::cos(a)), y + static_cast<int>(radius*std::sin(a)), color);
            }
        } else draw.line(tx - 15, y - 25, tx + 26, y - 37, muted);
        const std::string result = !beat.hit ? "MISS" : beat.style == CombatStyle::suppression ? "SUPPRESSED" :
            beat.damage == 0 ? "BLOCKED" : "-" + std::to_string(beat.damage) + " HP";
        draw.fill({at.x - 90, at.y + 53, 180, 26}, background);
        draw.label(at.x - draw.measure(result, 16)/2, at.y + 56, result, 16, beat.hit ? color : muted);
    }
    // Identify both participants above the board, without a blocking dialog.
    draw.fill({40, 174, 920, 28}, panel);
    draw.label(52, 179, draw.elide(beat.attacker + "  >  " + beat.target +
        (t >= 0.48F && beat.defeated ? (beat.defenderAttack ? " / DESTROYED" : " / EVACUATED") : ""), 890, 15), 15, color);
}
void Gui::presentationOverlay() {
    if (!game || presentation == Presentation::idle) return;
    // Remove board orders, but keep pause/save available during all animations.
    buttons.erase(std::remove_if(buttons.begin(), buttons.end(), [](const Button& b) { return b.id != "menu" && b.id != "save" && b.id != "archive" && b.id != "codex"; }), buttons.end());
    const float elapsed = static_cast<float>(animationClock - stageStarted) / animationScale();
    if (presentation == Presentation::drawing || presentation == Presentation::flipping || presentation == Presentation::card) {
        draw.fill({0, 80, Width, 844}, {6, 10, 12, 238});
        draw.label(56, 202, "02 / WORLD PHASE", 15, gold);
        // Physical-looking deck on the command table.
        for (int i = 3; i >= 0; --i) {
            SDL_Rect stack {97 + i*4, 565 + i*4, 168, 222};
            SDL_RenderCopy(renderer, eventTexture(false), nullptr, &stack);
        }
        draw.label(99, 812, std::to_string(game->getDeckSize()) + " CARDS IN DECK", 12, muted);
        SDL_Rect destination {495, 178, 440, 580};
        SDL_Texture* texture = eventTexture(false);
        double angle = 0;
        if (presentation == Presentation::drawing) {
            float t = smooth(elapsed / 850);
            destination = {static_cast<int>(97 + 398*t), static_cast<int>(565 - 387*t), static_cast<int>(168 + 272*t), static_cast<int>(222 + 358*t)};
            angle = -12*(1 - t);
        } else if (presentation == Presentation::flipping) {
            float t = std::clamp(elapsed / 1000, 0.0F, 1.0F);
            destination.w = std::max(2, static_cast<int>(440 * std::abs(std::cos(t * 3.14159265F))));
            destination.x = 715 - destination.w/2;
            if (t >= 0.5F) texture = eventTexture(true);
        } else texture = eventTexture(true);
        draw.fill({destination.x + 9, destination.y + 12, destination.w, destination.h}, {0, 0, 0, 135});
        SDL_RenderCopyEx(renderer, texture, nullptr, &destination, angle, nullptr, SDL_FLIP_NONE);
        if (presentation == Presentation::card) {
            button("card-resolve", {495, 797, 440, 54}, "RESOLVE WORLD EVENT", [this] {
                advanceGamePhase(); beginStage(Presentation::worldEffect);
            }, true, true);
        } else {
            draw.label(567, 814, presentation == Presentation::drawing ? "DRAWING FROM THE DECK..." : "REVEALING WORLD EVENT...", 16, gold);
        }
        return;
    }
    if (presentation == Presentation::handoff) {
        draw.fill({320, 174, 800, 56}, panel); draw.outline({320, 174, 800, 56}, gold);
        draw.label(342, 191, draw.elide("DEFENDER PHASE / ROUND " + std::to_string(game->getRound()) +
            " / P" + std::to_string(game->getPlayerIndex() + 1) + " " + game->currentPlayer().getName(), 754, 19), 19, gold);
        return;
    }
    if (presentation == Presentation::enemyIntro) {
        draw.fill({440, 174, 560, 56}, panel); draw.outline({440, 174, 560, 56}, red);
        draw.label(464, 191, "HOSTILE PHASE / ENEMIES ACT", 21, red);
        return;
    }
    {
        std::string title = actionLabel;
        if (presentation == Presentation::worldEffect) title = "WORLD EFFECT / " + eventCards().at(game->getActiveCard()).name;
        if (presentation == Presentation::enemyPreview || presentation == Presentation::enemyResult) title = "HOSTILE / " + intent.name;
        draw.fill({380, 698, 604, 214}, {27, 32, 35}); draw.outline({380, 698, 604, 214}, presentation == Presentation::enemyPreview || presentation == Presentation::enemyResult ? red : gold);
        draw.label(398, 715, draw.elide(title, 562, 18), 18, gold);
        if (presentation == Presentation::enemyPreview) draw.paragraph(398, 753, intent.description, 560, 18, text, 5);
        else {
            int y = 752;
            // Keep the roll and result readable rather than flashing an entire transcript.
            const int start = std::max(0, static_cast<int>(recentLog.size()) - 4);
            for (int i = start; i < static_cast<int>(recentLog.size()) && y < 879; ++i)
                y = draw.paragraph(398, y, recentLog[i], 562, 15, (recentLog[i].find("d20") != std::string::npos || recentLog[i].find("d100") != std::string::npos) ? gold : text, 2) + 3;
        }
    }
    button("skip-animation", {751, 656, 216, 30}, "Finish this animation", [this] {
        stageStarted = animationClock - 10000; updatePresentation(animationClock);
    }, true, false, "Only finishes the current animation. Event cards still require your confirmation.");
}
void Gui::settleTest(bool acknowledge, const std::string& enemyScreenshot) {
    for (int i = 0; i < 500; ++i) {
        if (presentation == Presentation::idle) return;
        if (presentation == Presentation::card) {
            if (!acknowledge) return;
            clickTest("card-resolve");
        } else {
            if (presentation == Presentation::enemyResult && !enemyScreenshot.empty() && !game->getCombatEvents().empty()) {
                updatePresentation(animationClock + static_cast<Uint32>(780 * animationScale()));
                screenshot(enemyScreenshot);
            }
            updatePresentation(animationClock + 10000);
        }
    }
    throw std::runtime_error("Presentation did not settle.");
}
