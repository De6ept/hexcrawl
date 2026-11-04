#pragma once
#include "card.h"

namespace hex {

struct ChainResult {
    int damage = 0;
    float mult = 1.f;
    Element element = Element::None;
    std::vector<std::string> log;
};

// Resolve cards left to right and return the total. No rendering, no input.
// Phase 2 adds relic hooks inside the loop.
ChainResult resolveChain(const std::vector<PlayedCard>& chain);

// Run a card that already resolved a second time (used by Echo).
// Position checks use the card's ORIGINAL slot, so echoing an Opener
// re-triggers its bonus. `atLeast` lets Echo upgrade the rune tier.
void retrigger(ChainState& state, const HistoryEntry& entry,
               RuneTier atLeast = RuneTier::Base);

} // namespace hex
