# Hexcrawl

A 2D roguelike deckbuilder: chain spell cards, trace runes, break the numbers.
Learning project. PC only.

## Build

Requirements: CMake 3.20+, a C++20 compiler.
On Linux you also need the raylib dependencies (X11/OpenGL dev packages).

```
cmake -B build
cmake --build build
./build/hexcrawl
```

Headless core only (no window, no raylib download):

```
cmake -B build -DHEXCRAWL_BUILD_GAME=OFF
cmake --build build
```

## Layout

```
src/core/   game logic, no rendering (cards, resolver, relics, rune math)
src/game/   raylib window, input, drawing
data/       cards.json, relics.json, runes.json
tests/      unit tests (added in phase 2)
docs/       roadmap and design notes
```

Rule: `src/core` never includes raylib.
