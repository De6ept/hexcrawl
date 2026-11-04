#include "resolver.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace hex {

RuneTier tierFromAccuracy(float accuracy) {
    if (accuracy >= kPerfectThreshold)  return RuneTier::Perfect;
    if (accuracy >= kResonantThreshold) return RuneTier::Resonant;
    return RuneTier::Base;
}

const char* tierName(RuneTier tier) {
    switch (tier) {
        case RuneTier::Base:     return "Base";
        case RuneTier::Resonant: return "Resonant";
        case RuneTier::Perfect:  return "Perfect";
    }
    return "?";
}

const char* elementName(Element element) {
    switch (element) {
        case Element::None:  return "None";
        case Element::Fire:  return "Fire";
        case Element::Ice:   return "Ice";
        case Element::Storm: return "Storm";
    }
    return "?";
}

void retrigger(ChainState& state, const HistoryEntry& entry, RuneTier atLeast) {
    const int savedSlot = state.slot;
    const RuneTier tier = std::max(entry.tier, atLeast);

    state.slot = entry.slot;   // position-dependent cards see where they were played
    state.note(std::format("  echo of {} ({})", entry.card->name, tierName(tier)));
    entry.card->effect(state, tier);
    state.slot = savedSlot;
}

ChainResult resolveChain(const std::vector<PlayedCard>& chain) {
    ChainState state;

    for (std::size_t i = 0; i < chain.size(); ++i) {
        const Card& card = *chain[i].card;
        const RuneTier tier = tierFromAccuracy(chain[i].accuracy);

        state.slot = static_cast<int>(i);
        // Read and clear the flag BEFORE the effect, so this card can set it for the next one.
        const bool ignited = std::exchange(state.igniteNext, false);

        state.note(std::format("[{}] {} ({})", i + 1, card.name, tierName(tier)));
        card.effect(state, tier);

        if (ignited && state.element != Element::Fire) {
            state.element = Element::Fire;
            state.note("  ignited: chain element is now Fire");
        }
        state.history.push_back({&card, tier, state.slot});
    }

    ChainResult result;
    result.damage  = static_cast<int>(std::lround(state.damage));
    result.mult    = state.mult;
    result.element = state.element;
    result.log     = std::move(state.log);
    return result;
}

} // namespace hex
