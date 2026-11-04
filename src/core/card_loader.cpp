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

// ---------- helpers (step 6) ----------

// ---------- kind builders (steps 6-10) ----------

// ---------- kind table (step 6) ----------

// Build ONE Card from ONE { ... } block of the JSON file.
Card loadCard(const json& j) {
    Card card;
    card.id       = j.at("id").get<std::string>();
    card.name     = j.at("name").get<std::string>();
    card.rune     = j.at("rune").get<std::string>();
    card.text     = j.at("text").get<std::string>();
    card.echoable = j.value("echoable", true);      // optional field, default true

    // STEP 6: the "kind" lookup goes here. For now the effect stays empty.
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