#pragma once
#include <string_view>
#include <vector>
#include "card.h"

namespace hex {

// The starter set. Returned references stay valid for the whole program.
const std::vector<Card>& allCards();

// Look a card up by its short id ("ember", "reverb", ...). nullptr if unknown.
const Card* findCard(std::string_view id);

} // namespace hex
