#pragma once
#include <filesystem>
#include <vector>

#include "card.h"

namespace hex {

// Load cards from a JSON file (format: docs/PHASE2.md).
// Throws std::runtime_error with a readable message if the file or its data is bad.
std::vector<Card> loadCards(const std::filesystem::path& file);

} // namespace hex
