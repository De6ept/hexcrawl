#include <raylib.h>

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

#include "card_loader.h"
#include "resolver.h"

using namespace hex;

namespace {

// ---- layout: sizes and positions ------------------------------------------

constexpr int kScreenW = 1280;
constexpr int kScreenH = 720;

// ---- game state -----------------------------------------------------------

struct Game {
    std::vector<Card> library;    // every card, loaded from data/cards.json
};

// ---- helpers --------------------------------------------------------------

// ---- update: runs once per frame, before drawing --------------------------

void update(Game& /*g*/) {
    // input handling goes here (from step 4 on)
}

// ---- drawing helpers ------------------------------------------------------

// ---- draw: runs once per frame --------------------------------------------

void draw(const Game& g) {
    ClearBackground(Color{18, 14, 28, 255});
    DrawText("HEXCRAWL", 40, 16, 36, Color{190, 140, 255, 255});

    int y = 80;
    for (const Card& c : g.library) {
        DrawText(c.name.c_str() , 40, y, 24, RAYWHITE);
        y += 32;
    }
}

} // namespace

int main() {
    Game game;
    try {
        game.library = loadCards(std::string(HEXCRAWL_DATA_DIR) + "/cards.json");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }

    InitWindow(kScreenW, kScreenH, "Hexcrawl");
    
    std::printf("monitor: %d Hz\n", GetMonitorRefreshRate(GetCurrentMonitor()));
    SetTargetFPS(120);

    while (!WindowShouldClose()) {      // ESC or closing the window
        update(game);
        BeginDrawing();
        draw(game);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}