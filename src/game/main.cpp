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

// The two fighters stand on the "floor": you on the left, the enemy on the right.
constexpr Rectangle kPlayerBox{160.f, 150.f, 140.f, 210.f};
constexpr Rectangle kEnemyBox {930.f, 130.f, 190.f, 230.f};

// float -> int, because DrawText wants whole pixels.
int px(float v) { return static_cast<int>(v); }




// ---- game state -----------------------------------------------------------
// A placeholder enemy: only numbers and text for now. Phase 5 makes it a real opponent.

struct Enemy {
    std::string name   = "Training Dummy";
    int         hp     = 80;
    int         maxHp  = 80;
    std::string intent = "Attack 8";     // what it plans to do (just text for now)
};

struct Game {
    std::vector<Card> library;    // every card, loaded from data/cards.json

    Enemy enemy;
};

// ---- helpers --------------------------------------------------------------

// ---- update: runs once per frame, before drawing --------------------------

void update(Game& /*g*/) {
    // input handling goes here (from step 4 on)
}

// ---- drawing helpers ------------------------------------------------------

void drawHpBar(Rectangle bar, float hp, int maxHp) {
    DrawRectangleRec(bar, Color{40, 20, 24, 255});

    Rectangle fill = bar;
    fill.width = bar.width * (hp/maxHp);
    DrawRectangleRec(fill, Color{190, 50, 50, 255});
    DrawRectangleLinesEx(bar, 2.f, BLACK);

    const char* label = TextFormat("%d/%d", hp , maxHp);
    DrawText(label ,px(bar.x + bar.width / 2.f) - MeasureText(label, 16) / 2, px(bar.y) + 3, 16, RAYWHITE);
}

void drawTopBar() {
    DrawRectangle(0, 0, kScreenW, 44, Color{10, 8, 18, 255});
    DrawText("Hexcaster", 16, 10, 24, Color{190, 140, 255, 255});
    DrawText("HP 70/70", 190, 12, 20, Color{230, 90, 90, 255});
    DrawText("Gold 99", 320, 12, 20, GOLD);
    DrawText("Floor 1", kScreenW - 110, 12, 20, GRAY);
}

void drawPlayer() {
    DrawRectangleRec(kPlayerBox, Color{50, 90, 150, 255});
    DrawRectangleLinesEx(kPlayerBox, 2.f, BLACK);
    DrawText("YOU", px(kPlayerBox.x) + 12, px(kPlayerBox.y) + 12, 22, RAYWHITE);
    drawHpBar({ kPlayerBox.x, kPlayerBox.y + kPlayerBox.height + 14.f, kPlayerBox.width, 22.f }, 70, 70);
}

void drawEnemy(const Enemy& e) {
    DrawRectangleRec(kEnemyBox, Color{150, 50, 60, 255});
    DrawRectangleLinesEx(kEnemyBox, 2.f, BLACK);
    DrawText("dummy", px(kEnemyBox.x) + 12, px(kEnemyBox.y) + 12, 22, RAYWHITE);

    // The "intent" floats above the enemy's head, like in Slay the Spire.
    DrawText(TextFormat("Intent: %s", e), px(kEnemyBox.x), px(kEnemyBox.y) - 34, 22, GOLD);

    drawHpBar({ kEnemyBox.x, kEnemyBox.y + kEnemyBox.height + 14.f, kEnemyBox.width, 22.f }, e.hp, e.maxHp);
}
// ---- draw: runs once per frame --------------------------------------------

void draw(const Game& g) {
    ClearBackground(Color{18, 14, 28, 255});
    DrawRectangle(0, 340, kScreenW, kScreenH - 340, Color{28, 22, 44, 255});   // the floor

    drawTopBar();
    drawPlayer();
    drawEnemy(g.enemy);
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