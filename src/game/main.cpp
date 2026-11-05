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

constexpr float kCardW = 140.f;
constexpr float kCardH = 196.f;
constexpr float kGap   = 12.f;
constexpr float kHandY = 510.f;    // top edge of a card resting in the hand

// Where hand position i is, when the hand holds `count` cards. The row is centred.
Rectangle handSlot(int i, int count) {
    const float rowWidth = static_cast<float>(count) * kCardW + static_cast<float>(count - 1) * kGap;
    const float left     = (static_cast<float>(kScreenW) - rowWidth) / 2.f;
    return { left + static_cast<float>(i) * (kCardW + kGap), kHandY, kCardW, kCardH };
}

// ---- game state -----------------------------------------------------------
// A placeholder enemy: only numbers and text for now. Phase 5 makes it a real opponent.

struct Enemy {
    std::string name   = "Training Dummy";
    int         hp     = 74;
    int         maxHp  = 80;
    std::string intent = "Attack 8";     // what it plans to do (just text for now)
};


struct CardView {
    const Card* card;               // points INTO game.library
    bool        selected = false;   // raised = part of the chain
};

struct Game {
    std::vector<Card> library;    // every card, loaded from data/cards.json

    Enemy enemy;
    std::vector<CardView> hand; 
};

// ---- helpers --------------------------------------------------------------

// ---- update: runs once per frame, before drawing --------------------------

void update(Game& /*g*/) {
    // input handling goes here (from step 4 on)
}

// ---- drawing helpers ------------------------------------------------------

void drawHpBar(Rectangle bar, int hp, int maxHp) {
    DrawRectangleRec(bar, Color{40, 20, 24, 255});

    Rectangle fill = bar;

    fill.width = bar.width * ((float)hp/maxHp);
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
    DrawText(e.name.c_str(), px(kEnemyBox.x) + 12, px(kEnemyBox.y) + 12, 22, RAYWHITE);

    // The "intent" floats above the enemy's head, like in Slay the Spire.
    DrawText(TextFormat("Intent: %s", e.intent.c_str()), px(kEnemyBox.x), px(kEnemyBox.y) - 34, 22, GOLD);

    drawHpBar({ kEnemyBox.x, kEnemyBox.y + kEnemyBox.height + 14.f, kEnemyBox.width, 22.f }, e.hp, e.maxHp);
}

// Draws text inside `box`, starting a new line whenever the next word would not fit.
void drawWrapped(const std::string& text, Rectangle box, int fontSize, Color color) {
    std::istringstream words(text);
    std::string word, line;
    int y = px(box.y);

    while (words >> word) {
        const std::string longer = line.empty() ? word : line + " " + word;
        if (!line.empty() && static_cast<float>(MeasureText(longer.c_str(), fontSize)) > box.width) {
            DrawText(line.c_str(), px(box.x), y, fontSize, color);
            y += fontSize + 2;
            line = word;
        } else {
            line = longer;
        }
    }
    if (!line.empty()) DrawText(line.c_str(), px(box.x), y, fontSize, color);
}

// One card: a rectangle, its name, its rune and its text.
void drawCard(const Card& card, Rectangle r, bool selected, bool hovered) {
    Color fill   = Color{48, 38, 78, 255};
    Color border = GRAY;
    if (hovered)  { fill = Color{70, 56, 110, 255};  border = RAYWHITE; }
    if (selected) { fill = Color{86, 66, 130, 255};  border = GOLD; }
    DrawRectangleRec(r, fill);
    DrawRectangleLinesEx(r, selected ? 3.f : 2.f, border);

    DrawText(card.name.c_str(), px(r.x) + 10, px(r.y) + 10, 18, RAYWHITE);
    DrawText(TextFormat("rune: %s", card.rune.c_str()), px(r.x) + 10, px(r.y) + 36, 14, SKYBLUE);
    drawWrapped(card.text, { r.x + 10.f, r.y + 70.f, r.width - 20.f, r.height - 80.f }, 14, LIGHTGRAY);
}

void drawHand(const Game& g) {
    const Vector2 mouse = GetMousePosition();
    const int     count = static_cast<int>(g.hand.size());
    for (int i = 0; i < count; ++i) {
        const Rectangle r = handSlot(i, count);
        drawCard(*g.hand[i].card, r, g.hand[i].selected, CheckCollisionPointRec(mouse, r));
    }
}

// ---- draw: runs once per frame --------------------------------------------

void draw(const Game& g) {
    ClearBackground(Color{18, 14, 28, 255});
    DrawRectangle(0, 340, kScreenW, kScreenH - 340, Color{28, 22, 44, 255});   // the floor

    drawTopBar();
    drawPlayer();
    drawEnemy(g.enemy);
    drawHand(g);
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

    // One CardView per card, in the same order as the library. They point INTO
    // game.library, so never add or remove library cards after this point.
    for (const Card& c : game.library) {
        game.hand.push_back({ &c, false });
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