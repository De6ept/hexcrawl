#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace hex {

enum class Element { None, Fire, Ice, Storm };

// How well the player traced the rune. Phase 4 will compute accuracy (0..1)
// from mouse strokes; until then it is just a number you pass in.
enum class RuneTier { Base, Resonant, Perfect };

inline constexpr float kResonantThreshold = 0.85f;
inline constexpr float kPerfectThreshold  = 0.95f;

RuneTier tierFromAccuracy(float accuracy);
const char* tierName(RuneTier tier);
const char* elementName(Element element);

struct Card;

// One card that has already resolved in the current chain.
struct HistoryEntry {
    const Card* card;
    RuneTier tier;
    int slot;          // 0-based position it was played in
};

// Everything a card can read or change while a chain resolves.
struct ChainState {
    float damage = 0.f;               // total dealt so far
    float mult = 1.f;                 // strikes deal (power x mult) at the moment they resolve
    Element element = Element::None;  // element of the most recent spell
    int slot = 0;                     // 0-based position of the card being resolved
    bool igniteNext = false;          // set by a card: the NEXT card ends as Fire
    std::vector<HistoryEntry> history;
    std::vector<std::string> log;     // human-readable trace for the console

    void note(std::string line) { log.push_back(std::move(line)); }
};

struct Card {
    std::string id;     // short name used on the command line, e.g. "ember"
    std::string name;
    std::string rune;   // which rune to trace (used from phase 4)
    std::string text;
    std::function<void(ChainState&, RuneTier)> effect;
    bool echoable = true;   // false for cards that must not be re-triggered (Reverb)
};

struct PlayedCard {
    const Card* card;
    float accuracy;     // 0..1, from the rune recognizer
};

} // namespace hex
