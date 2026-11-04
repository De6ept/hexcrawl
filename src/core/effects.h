#pragma once
// Small building blocks shared by every card effect.
#include <format>
#include <functional>

#include "card.h"

namespace hex {

// What a card does when it resolves.
using EffectFn = std::function<void(ChainState&, RuneTier)>;

inline bool isPerfect(RuneTier tier) { return tier == RuneTier::Perfect; }

// Pick the number for a tier. Perfect uses the Resonant number (plus its hidden tech).
inline float byTier(RuneTier tier, float base, float resonant) {
    return tier == RuneTier::Base ? base : resonant;
}

// Deal `power` damage at the current multiplier and write it to the log.
inline void strike(ChainState& s, float power) {
    const float dealt = power * s.mult;
    s.damage += dealt;
    s.note(std::format("  {:g} power x{:g} mult = {:g} damage", power, s.mult, dealt));
}

} // namespace hex
