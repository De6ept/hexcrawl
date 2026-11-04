#pragma once
#include <string_view>
#include <vector>
#include "card.h"

namespace hex {

// The starter set. Returned references stay valid for the whole program.
const std::vector<Card>& allCards();

const Card* findCard(std::string_view id);
const Card* findCard(const std::vector<Card>& cards, std::string_view id);
} // namespace hex
