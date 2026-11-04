# Phase 2: data-driven cards (exercises)

**Goal:** cards live in `data/cards.json` instead of C++. Change a number, restart,
done. No recompiling.

**Rule of the phase:** the demo must give *exactly the same totals* as before.
You prove it with `bash scripts/check_phase2.sh` (exercise 6).

Unit tests are skipped for now. The check script is your safety net until then.

You write `src/core/card_loader.cpp` yourself. The files I gave you:

| File | What it is |
|---|---|
| `src/core/effects.h` | `EffectFn`, `byTier`, `isPerfect`, `strike`. The same helpers `cards.cpp` uses, now shareable. |
| `src/core/card_loader.h` | The one function you implement: `loadCards()`. |
| `src/core/card_loader.cpp` | A stub that throws. Replace it. |
| `data/cards.json` | All six starter cards, already converted. |
| `docs/demo_totals.txt` | The totals the demo must still produce. |
| `scripts/check_phase2.sh` | Compares your output to those totals. |

---

## The idea: kinds and techs

You can't put a C++ lambda in a JSON file. So the split is:

- **Numbers and names are data:** JSON.
- **Behaviour is code:** a small fixed set of **kinds**, each a C++ function that
  *builds* a card effect from JSON fields. A card says `"kind": "striker"` and the
  loader looks the kind up in a table.
- **Hidden tech is data too:** an optional `"tech"` object. It only fires on a
  Perfect (>= 95%) trace.

Adding a new striker later is then a JSON edit. Adding a new *kind* is one new C++ function.

## The JSON format

Numbers that depend on the rune tier look like `{ "base": 6, "resonant": 9 }`.
Perfect uses the `resonant` number plus the tech, exactly as `cards.cpp` does.

Fields every card has: `id`, `name`, `rune`, `text`, `kind`.
Optional: `echoable` (default `true`), `tech`.

| kind | extra fields | behaviour (port it from the matching card in `cards.cpp`) |
|---|---|---|
| `striker` | `element` (optional), `power` | set element, then strike |
| `opener` | `element`, `power`, `opener_bonus` | slot 0 adds the bonus to the strike |
| `amplifier` | `mult_gain` | adds to the multiplier |
| `echo` | `count`, `resonant_floor` | re-triggers earlier echoable cards |
| `conditional` | `element`, `power`, `requires_element`, `multiplier` | multiplies power if the chain element matches |

| tech `type` | extra | used by | effect |
|---|---|---|---|
| `ignite_next` | | striker | next card ends as Fire |
| `add_mult` | `amount` | striker, opener | mult += amount (opener: only if its bonus fired) |
| `retroactive` | | amplifier | damage so far is scaled before the mult rises |
| `echo_count` | `count` | echo | echo this many cards instead |
| `ignore_condition` | | conditional | the element condition counts as met |

Elements are the strings `none`, `fire`, `ice`, `storm`.

---

## Exercises

Commit after each one (`Ctrl+Shift+G` in your editor). Check `git rev-parse --show-toplevel`
first if you ever open a terminal somewhere unfamiliar.

### 1. Add the JSON library

Wire `nlohmann/json` into `CMakeLists.txt` and add `src/core/card_loader.cpp` to the
`hexcrawl_core` sources.

**Done when:** `#include <nlohmann/json.hpp>` compiles inside `card_loader.cpp`.

<details><summary>Hints</summary>

- Copy the pattern you already have for raylib: `FetchContent_Declare` then `FetchContent_MakeAvailable`.
- Repo `https://github.com/nlohmann/json.git`, tag `v3.11.3`, `GIT_SHALLOW TRUE`.
- `include(FetchContent)` is currently *inside* the `if(HEXCRAWL_BUILD_GAME)` block. The
  headless build (and CI) needs json too, so it has to live outside it.
- Link target: `nlohmann_json::nlohmann_json`. Which keyword fits, `PRIVATE` or `PUBLIC`?
  Think about whether `card_loader.h` exposes any json type.
</details>

### 2. A `--data` flag on the console

`hexcrawl_console --data data/cards.json [rest as before]` should load cards from a file
and use them instead of the built-in ones.

**Done when:** running it prints `error: loadCards is not implemented yet ...` and exits
with code 1. That proves the plumbing works before the loader exists. Without `--data`
everything behaves as before.

<details><summary>Hints</summary>

- `findCard` only searches the built-in list. Add an overload that takes the list to search.
- The demo functions need to know which list is active. A file-level pointer
  (`const std::vector<Card>*`) that `main` sets is the simple way.
- Catch `std::exception` around `loadCards`, print to `stderr`, return 1.
- Keep the loaded vector alive in `main`. History stores `const Card*` into it.
</details>

### 3. The loader skeleton: names only

Implement `loadCards()` so it returns cards with `id`, `name`, `rune`, `text` and
`echoable` filled in. Leave `effect` empty for now.

**Done when:** `hexcrawl_console --data data/cards.json --cards` lists all six cards.
Then break `cards.json` on purpose (delete a field, delete a brace, misspell the file name).
Each failure should give a readable one-line error that names the file and, for bad
card data, **which card number**.

<details><summary>Hints</summary>

- `std::ifstream`, then `nlohmann::json::parse(in)`. A malformed file throws `json::parse_error`.
- `j.at("id").get<std::string>()` throws a helpful `json::out_of_range` if the key is missing.
  `j.value("echoable", true)` reads an optional field with a default.
- Wrap the per-card work in a `try`, and rethrow as `std::runtime_error` with the
  file name and index added. `std::format` is available.
- Do not use `operator[]` on a const json to read required fields. A missing key is
  undefined behaviour there. Use `.at()`.
</details>

### 4. Your first kind: `striker`

Now give cards a real `effect`. Build, in this order:

1. `parseElement(string) -> Element` (throw on unknown names).
2. A tier-number reader for `{ "base": .., "resonant": .. }`.
3. A tech reader for the optional `"tech"` object.
4. `makeStriker(json) -> EffectFn`.
5. A table from kind name to factory, and use it in your per-card code (unknown kind = error).

**Done when:** a `cards.json` containing only `ember` and `frost` gives
`ember` = 6 damage, `ember:0.97` = 9 damage and ignites the next card, and
`--data data/cards.json frost:0.97` raises the multiplier.

<details><summary>Hints</summary>

- **The big gotcha:** the lambda outlives `loadCards()`. Parse everything into plain C++
  values *first* (a small struct is ideal), then capture those **by value** (`[=]`).
  Never capture the `json` object or a reference into it. It is destroyed when the loader returns.
- A function pointer type works for the table: `EffectFn (*)(const nlohmann::json&)`.
  A function-local `static const std::unordered_map` keeps it tidy.
- Unsupported tech on a kind (say `retroactive` on a striker) is a data mistake. Catching it
  at load time, not mid-fight, is worth a few lines.
</details>

### 5. The other four kinds

Port `opener`, `amplifier`, `echo` and `conditional`. The behaviour is in `cards.cpp`.
You are turning each hardcoded lambda into a function that reads its numbers from JSON.

Things that are easy to get wrong:
- **opener:** the `add_mult` tech only applies when the opener bonus actually fired (slot 0).
- **amplifier:** `retroactive` scales damage *before* the mult goes up.
- **echo:** skips cards with `echoable == false`, and replays in original order. Look at
  `retrigger()` in `resolver.h`.
- **conditional:** set the element *after* checking the condition.

**Done when:** `bash scripts/check_phase2.sh` prints `PASS` (next exercise).

### 6. The acceptance check

```bash
cmake --build build
bash scripts/check_phase2.sh
```

`PASS` means data-driven cards behave exactly like the hardcoded ones, including
Demo 7's 252. On `FAIL` it shows which totals differ (`<` is yours, `>` is expected).
Debug by running the failing chain by hand:

```bash
./build/hexcrawl_console --data data/cards.json spark:0.97 reverb:0.97 ember
```

Tick Phase 2 part 1 in `docs/ROADMAP.md` and push.

### 7. Stretch (pick any, in any order)

- **Prove it's data-driven:** add a new striker (`gale`, Storm, 4/7) to `cards.json` only.
  No C++ changes. Play it.
- **Duplicate ids:** two cards with the same `id` should be a load error.
- **Better unknown-kind error:** list the valid kinds in the message.
- **A `finisher` kind:** if you wrote Detonate earlier, make it loadable from JSON.
- **Remove the duplication:** `cards.cpp` has its own copies of `byTier`, `isPerfect` and
  `strike`. Make it use `effects.h` instead. If the compiler says a call is *ambiguous*,
  work out why.
- **CI:** add `bash scripts/check_phase2.sh build/hexcrawl_console` to the `core` job in
  `.github/workflows/ci.yml` so GitHub checks your data on every push.

---

## Stuck?

Paste your code and the error and I'll help you find it. I'd rather point at the
line than hand you a finished file. If you want a full solution to compare against once
you're done, ask for it then.

## Next: Phase 2 part 2, relics

Relic hooks inside `resolveChain()` (Twin Wick: first card triggers twice), also data-driven.
Start it when exercise 6 passes.
