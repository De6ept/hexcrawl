#include "card_loader.h"

#include <stdexcept>

// You write this file. See docs/PHASE2.md, exercises 1-5.
//
// After exercise 1 (nlohmann/json is wired into CMake) add:
//     #include <nlohmann/json.hpp>
//     #include "effects.h"
//
// Suggested layout, top to bottom:
//   - helpers:   parseElement, readTiered, readTech       (exercise 2 / 4)
//   - factories: makeStriker, makeOpener, makeAmplifier,
//                makeEcho, makeConditional                (exercise 4 / 5)
//   - a table that maps the "kind" string to its factory  (exercise 4)
//   - loadCards() itself                                  (exercise 2)

namespace hex {

std::vector<Card> loadCards(const std::filesystem::path& /*file*/) {
    throw std::runtime_error("loadCards is not implemented yet (see docs/PHASE2.md)");
}

} // namespace hex
