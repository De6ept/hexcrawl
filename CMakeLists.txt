cmake_minimum_required(VERSION 3.20)
project(hexcrawl VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Turn OFF to build only the headless core (used by CI and unit tests).
option(HEXCRAWL_BUILD_GAME "Build the raylib game executable" ON)

function(hexcrawl_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

# ---------------------------------------------------------------------------
# hexcrawl_core: pure game logic (cards, chain resolver, relics, rune math).
# Must never include raylib. This is what unit tests and CI build.
# ---------------------------------------------------------------------------
add_library(hexcrawl_core STATIC
  src/core/version.cpp
  src/core/resolver.cpp
  src/core/cards.cpp
  src/core/card_loader.cpp
)
target_include_directories(hexcrawl_core PUBLIC src/core)
hexcrawl_warnings(hexcrawl_core)

# JSON library (downloaded automatically, like raylib).
include(FetchContent)
FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(json)
target_link_libraries(hexcrawl_core PRIVATE nlohmann_json::nlohmann_json)
# ---------------------------------------------------------------------------
# hexcrawl_console: play chains in the terminal. No window, no raylib.
# ---------------------------------------------------------------------------
add_executable(hexcrawl_console src/console/main.cpp)
target_link_libraries(hexcrawl_console PRIVATE hexcrawl_core)
hexcrawl_warnings(hexcrawl_console)

# ---------------------------------------------------------------------------
# hexcrawl: the game itself (window, input, drawing). Links core + raylib.
# ---------------------------------------------------------------------------
if(HEXCRAWL_BUILD_GAME)
  find_package(raylib 5.5 QUIET)
  if(NOT raylib_FOUND)
    FetchContent_Declare(raylib
      GIT_REPOSITORY https://github.com/raysan5/raylib.git
      GIT_TAG 5.5
      GIT_SHALLOW TRUE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
  endif()

  add_executable(hexcrawl src/game/main.cpp)
  target_link_libraries(hexcrawl PRIVATE hexcrawl_core raylib)
  hexcrawl_warnings(hexcrawl)

  # Let the game find data/ no matter where it is launched from.
  target_compile_definitions(hexcrawl PRIVATE
    HEXCRAWL_DATA_DIR="${CMAKE_SOURCE_DIR}/data")
endif()
