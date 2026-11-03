#include <raylib.h>
#include "version.h"

int main() {
    InitWindow(1280, 720, "Hexcrawl");
    SetTargetFPS(60);

    const std::string version = hex::versionString();

    while (!WindowShouldClose()) {   // ESC or window close
        BeginDrawing();
        ClearBackground(Color{18, 14, 28, 255});
        DrawText("HEXCRAWL", 480, 280, 80, Color{190, 140, 255, 255});
        DrawText(version.c_str(), 480, 380, 24, RAYWHITE);
        DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
