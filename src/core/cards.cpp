#include "cards.h"
#include "resolver.h"

#include <algorithm>
#include <format>

namespace hex {
namespace {

// Numbers live here so tuning is one place. Perfect uses the Resonant numbers
// plus its hidden tech.
float byTier(RuneTier tier, float base, float resonant) {
    return tier == RuneTier::Base ? base : resonant;
}

bool isPerfect(RuneTier tier) { return tier == RuneTier::Perfect; }

// Deal `power` damage at the current multiplier.
void strike(ChainState& s, float power) {
    const float dealt = power * s.mult;
    s.damage += dealt;
    s.note(std::format("  {:g} power x{:g} mult = {:g} damage", power, s.mult, dealt));
}

std::vector<Card> makeCards() {
    std::vector<Card> cards;

    // ---- Striker: reliable damage, sets the element -------------------------
    cards.push_back({
        .id = "ember", .name = "Ember Bolt", .rune = "triangle",
        .text = "Deal 6. Sets Fire.",
        .effect = [](ChainState& s, RuneTier t) {
            s.element = Element::Fire;
            strike(s, byTier(t, 6, 9));
            if (isPerfect(t)) {
                s.igniteNext = true;
                s.note("  hidden tech: the next card ends as Fire");
            }
        }});

    cards.push_back({
        .id = "frost", .name = "Frost Shard", .rune = "spiral",
        .text = "Deal 5. Sets Ice.",
        .effect = [](ChainState& s, RuneTier t) {
            s.element = Element::Ice;
            strike(s, byTier(t, 5, 8));
            if (isPerfect(t)) {
                s.mult += 0.5f;
                s.note("  hidden tech: +0.5 mult");
            }
        }});

    // ---- Opener: huge in the first slot, weak anywhere else -----------------
    cards.push_back({
        .id = "spark", .name = "Spark", .rune = "zigzag",
        .text = "Deal 3. First slot only: +10 more. Sets Storm.",
        .effect = [](ChainState& s, RuneTier t) {
            s.element = Element::Storm;
            float power = 3;
            if (s.slot == 0) {
                power += byTier(t, 10, 15);
                s.note("  opener bonus");
            }
            strike(s, power);
            if (s.slot == 0 && isPerfect(t)) {
                s.mult += 1.f;
                s.note("  hidden tech: +1 mult");
            }
        }});

    // ---- Amplifier: boosts every strike that comes AFTER it ------------------
    cards.push_back({
        .id = "overcharge", .name = "Overcharge", .rune = "cross",
        .text = "+1 mult for later strikes.",
        .effect = [](ChainState& s, RuneTier t) {
            const float gain = byTier(t, 1.0f, 1.5f);
            if (isPerfect(t)) {
                const float ratio = (s.mult + gain) / s.mult;
                s.damage *= ratio;
                s.note(std::format("  hidden tech: damage so far x{:.2f}", ratio));
            }
            s.mult += gain;
            s.note(std::format("  mult is now {:g}", s.mult));
        }});

    // ---- Echo: re-triggers earlier cards ------------------------------------
    cards.push_back({
        .id = "reverb", .name = "Reverb", .rune = "circle",
        .text = "Re-trigger the previous card. Resonant+: echoes play at Resonant at least.",
        .effect = [](ChainState& s, RuneTier t) {
            const std::size_t count = isPerfect(t) ? 2 : 1;   // Perfect: hidden tech
            const RuneTier floor = t == RuneTier::Base ? RuneTier::Base : RuneTier::Resonant;

            std::vector<HistoryEntry> picks;
            for (auto it = s.history.rbegin();
                 it != s.history.rend() && picks.size() < count; ++it) {
                if (it->card->echoable) picks.push_back(*it);
            }
            std::reverse(picks.begin(), picks.end());   // replay in original order

            if (picks.empty()) { s.note("  nothing to echo"); return; }
            if (isPerfect(t)) s.note("  hidden tech: echoes the previous TWO cards");
            for (const HistoryEntry& e : picks) retrigger(s, e, floor);
        },
        .echoable = false});   // Echo must not echo an Echo

    // ---- Conditional: rewards a specific previous element --------------------
    cards.push_back({
        .id = "wildfire", .name = "Wildfire", .rune = "wave",
        .text = "Deal 4. x2 if the chain element is Fire. Sets Fire.",
        .effect = [](ChainState& s, RuneTier t) {
            float power = byTier(t, 4, 6);
            if (s.element == Element::Fire || isPerfect(t)) {
                power *= 2;
                s.note(isPerfect(t) && s.element != Element::Fire
                           ? "  hidden tech: Fire bonus without Fire"
                           : "  fire bonus x2");
            }
            s.element = Element::Fire;
            strike(s, power);
        }});

    return cards;
}

} // namespace

const std::vector<Card>& allCards() {
    static const std::vector<Card> cards = makeCards();
    return cards;
}

const Card* findCard(std::string_view id) {
    for (const Card& c : allCards()) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

} // namespace hex
