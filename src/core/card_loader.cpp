#include "card_loader.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "effects.h"
#include "resolver.h"

namespace hex {
namespace {

using json = nlohmann::json;

// "fire" -> Element::Fire, and so on. A typo in the JSON becomes a clear error.
Element parseElement(const std::string& name) {
    if (name == "none")   return Element::None;
    if (name == "fire")   return Element::Fire;
    if (name == "ice")    return Element::Ice;
    if (name == "storm")  return Element::Storm;
    throw std::runtime_error(std::format("unknown element '{}'", name));
}

// A number that depends on the rune tier, like { "base": 6, "resonant": 9 }.
struct Tiered {
    float base = 0.f;
    float resonant = 0.f;
    float at(RuneTier t) const { return byTier(t, base, resonant); }
};

Tiered readTiered(const json& card, const char* key) {
    const json& o = card.at(key);
    return { o.at("base").get<float>(), o.at("resonant").get<float>() };
}

// The optional hidden tech, like { "type": "add_mult", "amount": 0.5 }.
struct Tech {
    std::string type;       // empty = this card has no tech
    float amount = 0.f;
    int count = 0;
};

Tech readTech(const json& card) {
    Tech tech;
    if (!card.contains("tech")) return tech;
    const json& o = card.at("tech");
    tech.type   = o.at("type").get<std::string>();
    tech.amount = o.value("amount", 0.f);
    tech.count  = o.value("count", 0);
    return tech;
}

// ---------- kind striker

EffectFn makeStriker(const json& card) {
    //  Read everything we need from the JSON NOW, into plain C++ values.
    const Element element = parseElement(card.value("element", "none"));
    const Tiered  power   = readTiered(card, "power");
    const Tech    tech    = readTech(card);

    //  Build the effect. [=] COPIES element, power and tech into the lambda,
    //  so it keeps working after this function (and the json) is long gone.
    return [=](ChainState& s, RuneTier t) {
        if (element != Element::None) s.element = element;
        strike(s, power.at(t));             // power.at(t) = base or resonant number

        if (!isPerfect(t)) return;          // hidden tech only fires on Perfect
        if (tech.type == "ignite_next") {
            s.igniteNext = true;
            s.note("  hidden tech: the next card ends as Fire");
        } else if (tech.type == "add_mult") {
            s.mult += tech.amount;
            s.note(std::format("  hidden tech: +{:g} mult", tech.amount));
        }
    };
}

// ---------- kind opener

EffectFn makeOpener(const json& card) {
    const Element element = parseElement(card.value("element", "none"));
    const Tiered  power   = readTiered(card, "power");
    const Tiered  bonus   = readTiered(card, "opener_bonus");
    const Tech    tech    = readTech(card);

    return [=](ChainState& s, RuneTier t) {
        if (element != Element::None) s.element = element;

        float p = power.at(t);
        const bool opening = (s.slot == 0);       // slots count from 0: which one is first?
        if (opening) {
            p += bonus.at(t);                      // the bonus number for this tier
            s.note("  opener bonus");
        }
        strike(s, p);

        if (opening && isPerfect(t) && tech.type == "add_mult") {
            s.mult += tech.amount;                // one whole statement, ending in ;  (same as the striker's add_mult)
            s.note(std::format("  hidden tech: +{:g} mult", tech.amount));
        }
    };
}

// ---------- kind amplifire

EffectFn makeAmplifier(const json& card) {
    const Tiered gain = readTiered(card, "mult_gain");
    const Tech   tech = readTech(card);

    return [=](ChainState& s, RuneTier t) {
        const float g = gain.at(t);

        if (isPerfect(t) && tech.type == "retroactive") {
            const float ratio = (s.mult + g) / s.mult;
            s.damage *= ratio;
            s.note(std::format("  hidden tech: damage so far x{:.2f}", ratio));
        }

        s.mult += g;
        s.note(std::format("  mult is now {:g}", s.mult));
    };
}

// ---------- kind echo

EffectFn makeEcho(const json& card) {
    const std::size_t baseCount = card.value("count", 1);
    const bool floorOnResonant  = card.value("resonant_floor", false);
    const Tech tech             = readTech(card);

    return [=](ChainState& s, RuneTier t) {
        // How many cards to echo: normally baseCount, but a Perfect trace + tech changes it.
        std::size_t count = baseCount;
        if (isPerfect(t) && tech.type == "echo_count") {
            count = baseCount;   // the tech's own "count" number
        }

        // The tier echoed cards play at: Resonant at least, if this card asks for it.
        const RuneTier floor =
            (floorOnResonant && t != RuneTier::Base) ? RuneTier::Resonant : RuneTier::Base;

        // Walk the history BACKWARDS and collect up to `count` echoable cards.
        std::vector<HistoryEntry> picks;
        for (auto it = s.history.rbegin(); it != s.history.rend() && picks.size() < count; ++it) {
            if (it->card->echoable) picks.push_back(*it);   // skip cards that must not be echoed
        }
        std::reverse(picks.begin(), picks.end());      // replay in the order they were played

        if (picks.empty()) { s.note("  nothing to echo"); return; }
        for (const HistoryEntry& e : picks) {
            retrigger(s, e, floor);                // one whole statement, ending in ;  (see retrigger() in resolver.h)
        }
    };
}

// ---------- kind conditional

EffectFn makeConditional(const json& card) {
    // (Don't name a variable `requires`: it is a keyword in C++20.)
    const Element element  = parseElement(card.value("element", "none"));
    const Element required = parseElement(card.at("requires_element").get<std::string>());
    const Tiered  power    = readTiered(card, "power");
    const float multiplier = card.value("multiplier", 2.f);
    const Tech    tech     = readTech(card);

    return [=](ChainState& s, RuneTier t) {
        const bool techSkipsCheck = isPerfect(t) && tech.type == "ignore_condition";

        float p = power.at(t);
        if (s.element == required || techSkipsCheck) {   // matches, OR the tech says "pretend it does"
            p *= multiplier;
            s.note("  element bonus");
        }
        if (element != Element::None) s.element = element;   // set the new element AFTER the check
        strike(s, p);
    };
}

// ---------- kind table 
// A table from the "kind" text in the JSON to the function that builds that kind.
using Factory = EffectFn (*)(const json&);

const std::unordered_map<std::string, Factory>& kinds() {
    static const std::unordered_map<std::string, Factory> table = {
        {"striker", makeStriker},
        {"opener", makeOpener},
        {"amplifier", makeAmplifier},
        {"echo",makeEcho},
        {"conditional",makeConditional},
    };
    return table;
}

// Build ONE Card from ONE { ... } block of the JSON file.
Card loadCard(const json& j) {
    Card card;
    card.id       = j.at("id").get<std::string>();
    card.name     = j.at("name").get<std::string>();
    card.rune     = j.at("rune").get<std::string>();
    card.text     = j.at("text").get<std::string>();
    card.echoable = j.value("echoable", true);      // optional field, default true

    const std::string kind = j.at("kind").get<std::string>();
    const auto it = kinds().find(kind);
    if (it == kinds().end()) {
        throw std::runtime_error(std::format("unknown kind '{}'", kind));
    }
    card.effect = it->second(j);        // call the builder, keep the effect it returns
    return card;
}

} // namespace

std::vector<Card> loadCards(const std::filesystem::path& file) {
    // 1. Open the file.
    std::ifstream in(file);
    if (!in) {
        throw std::runtime_error(std::format("cannot open '{}'", file.string()));
    }

    // 2. Parse it. Broken JSON throws; we turn that into a readable message.
    json root;
    try {
        root = json::parse(in);
    } catch (const json::parse_error& e) {
        throw std::runtime_error(std::format("{}: invalid JSON: {}", file.string(), e.what()));
    }

    // 3. Build one Card per entry in "cards". `index` tells us which card
    //    went wrong if something throws.
    std::vector<Card> cards;
    std::size_t index = 0;
    try {
        for (const json& j : root.at("cards")) {
            cards.push_back(loadCard(j));
            ++index;
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::format("{}: card #{}: {}", file.string(), index, e.what()));
    }
    return cards;
}

} // namespace hex