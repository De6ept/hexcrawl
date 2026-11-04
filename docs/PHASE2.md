# Phase 2: data-driven cards (step by step)

**What you're building.** Right now every card is hard-coded in `cards.cpp`. By the end
of this guide the cards live in `data/cards.json` and the game reads them at startup.
Changing a number becomes a text edit instead of a recompile.

## How this guide works

- **11 small steps.** Each has: why we're doing it, code to type, a check, a commit.
- **Type the code, don't paste it.** It sticks better, and typos teach you to read
  compiler errors, which is half of programming.
- **`/* ??? */` is a blank for you to fill in.** The comment next to it, or a line just
  above it, shows the pattern to copy. Nothing needs an idea you haven't seen.
- **Check after every step.** Build with `cmake --build build`, run the check, and only
  move on when your output matches. A bug found in step 5 is small. The same bug found
  in step 10 is a mystery.
- **Stuck for more than ~15 minutes?** Paste your code and the error to me. That's the
  intended way to use this guide.

Files you already have from the Phase 2 zip: `data/cards.json` (all six cards, converted),
`src/core/effects.h`, `src/core/card_loader.h`, `src/core/card_loader.cpp` (a stub),
`scripts/check_phase2.sh` and `docs/demo_totals.txt`. This file replaces the old `docs/PHASE2.md`.

---

## Three ideas first (a 5 minute read)

### 1. JSON

JSON is a text format for data:

```json
{ "name": "Ember Bolt", "power": { "base": 6, "resonant": 9 } }
```

Curly braces make an **object** (values with names). Square brackets make an **array**
(a list). Values are text in quotes, numbers, or `true` / `false`. In C++ we use a library
called nlohmann/json, and you ask for a value by name: `card.at("name")`.

### 2. Lambdas and "capturing"

You've already read lambdas in `cards.cpp`: `[](ChainState& s, RuneTier t) { ... }` is a
function without a name. The `[ ]` part says what it remembers from where it was created.
`[=]` means "make a copy of every outside variable I use":

```cpp
int gain = 5;
auto addGain = [=](int x) { return x + gain; };   // gain (5) is copied INTO addGain
gain = 100;
addGain(1);                                       // still 6: it kept its own copy
```

This matters here because we build each card's effect *while reading the JSON*, but the
effect runs later, during a fight, when the JSON is gone. The rule for this whole phase:

> **Copy the numbers out of the JSON into plain variables first. Then capture those.**
> Never capture the JSON object itself.

### 3. Kinds

A JSON file can hold numbers and words but not code. So each card says what *kind* of card
it is (`"kind": "striker"`), and C++ has one **builder function** per kind. A builder reads
the card's numbers and returns the effect. A table connects the word to the builder:

```
"striker"  ->  makeStriker(json)  ->  an effect (a lambda)
"opener"   ->  makeOpener(json)   ->  an effect (a different lambda)
```

A new striker card is then just a JSON edit. A new *kind* of card is one new builder function.

**Rune tiers, as a reminder:** under 85% is Base, 85 to 95% is Resonant, 95%+ is Perfect.
In JSON a number that depends on the tier looks like `{ "base": 6, "resonant": 9 }`.
Perfect uses the `resonant` number *plus* the card's hidden tech, exactly like `cards.cpp`.

---

## Step 1: Add the JSON library

**Why:** so C++ can read JSON. Like raylib, CMake downloads it automatically the first time.

Open `CMakeLists.txt` and make three small changes.

**1a.** In the list of `hexcrawl_core` source files, add the loader file. The end of the
list should look like this:

```cmake
  src/core/cards.cpp
  src/core/card_loader.cpp
)
```

**1b.** Find the line `hexcrawl_warnings(hexcrawl_core)` (the call, not the `function(...)`
definition above it) and replace it with this whole block. It starts with that same line:

```cmake
hexcrawl_warnings(hexcrawl_core)

# JSON library (downloaded automatically, like raylib).
include(FetchContent)
FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(json)
target_link_libraries(hexcrawl_core PRIVATE nlohmann_json::nlohmann_json)
```

**1c.** Further down, inside `if(HEXCRAWL_BUILD_GAME)`, delete the line `include(FetchContent)`.
We moved it up so the headless build (and GitHub Actions) can download the library too.

**Check:**

```bash
cmake -B build
cmake --build build
```

The first run downloads the library, which takes a few seconds. No errors means you're done.
(If your editor shows red squiggles on includes later, run `cmake -B build` again so it
refreshes its picture of the project.)

**Commit:** `Phase 2 step 1: add json library`, then **push**. This is the first time GitHub
Actions downloads the library, so a green check there confirms the CMake edit is right.

---

## Step 2: Playground, meet JSON

**Why:** you need four JSON calls. Learn them in a tiny throwaway program before touching
real code.

**2a.** In `CMakeLists.txt`, replace the line `hexcrawl_warnings(hexcrawl_console)` with:

```cmake
hexcrawl_warnings(hexcrawl_console)

# Scratch program for learning JSON. Not part of the game.
add_executable(playground src/console/playground.cpp)
target_link_libraries(playground PRIVATE nlohmann_json::nlohmann_json)
```

**2b.** Create the file `src/console/playground.cpp`. Fill in the three blanks (they are the
JSON keys named in the text above them):

```cpp
// Scratch file for learning nlohmann/json. Not part of the game.
#include <cstdio>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    // R"( ... )" is a raw string: everything between the brackets is plain text.
    const std::string text = R"({
        "name": "Ember Bolt",
        "power": { "base": 6, "resonant": 9 }
    })";

    json card = json::parse(text);      // text in, a json object you can ask questions out

    // 1. A string value.
    const std::string name = card.at("name").get<std::string>();
    std::printf("name = %s\n", name.c_str());

    // 2. A number inside an object inside the object.
    const float base     = card.at("power").at(/* ??? */).get<float>();
    const float resonant = card.at("power").at(/* ??? */).get<float>();
    std::printf("base = %g, resonant = %g\n", base, resonant);

    // 3. An optional value with a default. "echoable" is NOT in the text above,
    //    so we get the default (true) back instead of an error.
    const bool echoable = card.value(/* ??? */, true);
    std::printf("echoable = %d\n", echoable);

    // 4. A required key that is missing. .at() throws: catch it and read the message.
    try {
        card.at("kind");
    } catch (const json::exception& e) {
        std::printf("error: %s\n", e.what());
    }

    // ---- part 2 goes here (step 3) ----
    return 0;
}
```

**2c.** Build and run:

```bash
cmake -B build
cmake --build build
./build/playground
```

You should see:

```text
$ ./build/playground
name = Ember Bolt
base = 6, resonant = 9
echoable = 1
error: [json.exception.out_of_range.403] key 'kind' not found
```

**What you just learned:**
- `.at("name")` asks for a value that **must** exist. A missing key throws an error.
- `.get<std::string>()` / `.get<float>()` converts the JSON value to a C++ type.
- `.value("echoable", true)` asks for an **optional** value and gives a default if it's missing.
- JSON errors name the missing key. We'll reuse those messages later.

**Commit:** `Phase 2 step 2: json playground`

---

## Step 3: Playground, read a file and loop

**Why:** the real loader needs to open a file and visit every card in the list.

**3a.** At the top of `playground.cpp`, add `#include <fstream>` under `#include <cstdio>`.
Change `int main() {` to `int main(int argc, char** argv) {` (so it can receive a file name).

**3b.** Replace the comment `// ---- part 2 goes here (step 3) ----` with this. The three blanks
copy the pattern from part 1:

```cpp
    // ---- part 2: a whole file ----
    if (argc < 2) return 0;                 // no file given: stop here
    std::ifstream in(argv[1]);
    if (!in) { std::printf("cannot open %s\n", argv[1]); return 1; }
    const json root = json::parse(in);

    // root["cards"] is an ARRAY. A range-for walks it one element at a time.
    for (const json& c : root.at(/* ??? */)) {
        const std::string cardName = /* ??? */;
        const std::string kind     = /* ??? */;
        std::printf("%-12s kind=%s\n", cardName.c_str(), kind.c_str());
    }
```

**3c.** Rebuild (`cmake --build build`), then run it on the real data file:

```text
$ ./build/playground data/cards.json
name = Ember Bolt
base = 6, resonant = 9
echoable = 1
error: [json.exception.out_of_range.403] key 'kind' not found
Ember Bolt   kind=striker
Frost Shard  kind=striker
Spark        kind=opener
Overcharge   kind=amplifier
Reverb       kind=echo
Wildfire     kind=conditional
```

(The first four lines are part 1, then one line per card.)

**Commit:** `Phase 2 step 3: playground reads a file`

---

## Step 4: A `--data` flag for the console

**Why:** the console should be able to play cards from a file. The loader is still a stub that
throws an error, so this step's check is that the error message appears properly. It proves the
plumbing works before the hard part exists.

**4a.** In `src/core/cards.h`, under the existing `findCard` line, add:

```cpp
// Same, but searches any list of cards (like ones loaded from JSON).
const Card* findCard(const std::vector<Card>& cards, std::string_view id);
```

**4b.** In `src/core/cards.cpp`, find the function `findCard(std::string_view id)` near the
bottom and replace the whole function with these two. (C++ allows two functions to share a
name when their parameters differ. That's called *overloading*. The old one now just calls
the new one with the built-in list.)

```cpp
const Card* findCard(const std::vector<Card>& cards, std::string_view id) {
    for (const Card& c : cards) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

const Card* findCard(std::string_view id) {
    return findCard(allCards(), /* ??? */);
}
```

The rest of this step is in `src/console/main.cpp`.

**4c. Include.** With the other `#include` lines at the top, add `#include "card_loader.h"`.

**4d. A pointer to the active card list.** Just above the line `bool parseChain(` (it's inside
`namespace {`), add this line. `g_library` is a *pointer* to whichever list of cards is in use:
the built-in ones, or the ones loaded from a file.

```cpp
const std::vector<Card>* g_library = nullptr;   // the list of cards we look things up in
```

**4e. Use it.** Two one-word edits:
- Inside `parseChain`, change `findCard(id)` to `findCard(*g_library, id)`.
  (`*` means "the thing the pointer points at".)
- Inside `listCards`, change `allCards()` to `*g_library`.

**4f. A new `main`.** At the bottom of the file, delete everything from the line
`int main(int argc, char** argv) {` to the end, and type this instead (two blanks):

```cpp
int main(int argc, char** argv) {
    // Turn the command line into a list of strings we can edit.
    std::vector<std::string> args(argv + 1, argv + argc);

    std::vector<Card> loaded;           // lives as long as main(): chains point into it
    g_library = &allCards();            // default: the built-in cards

    if (args.size() >= 2 && args[0] == "--data") {
        try {
            loaded = /* ??? */;
        } catch (const std::exception& e) {
            std::fprintf(stderr, "error: %s\n", e.what());
            return 1;
        }
        g_library = /* ??? */;
        args.erase(args.begin(), args.begin() + 2);     // forget "--data file"
    }

    if (args.empty()) { demos(); return 0; }
    if (args[0] == "--cards") { listCards(); return 0; }
    if (args[0] == "--help" || args[0] == "-h") {
        std::printf("usage: hexcrawl_console [--data cards.json] [--cards | id[:accuracy] ...]\n");
        return 0;
    }
    return run("your chain", args) < 0 ? 1 : 0;
}
```

**Check:** build, then run:

```text
$ ./build/hexcrawl_console --data data/cards.json
error: loadCards is not implemented yet (see docs/PHASE2.md)
$ echo $?
1
```

(`$ echo $?` prints the exit code of the previous command. 1 means "failed", which is right here.)
Also confirm that nothing else changed: `./build/hexcrawl_console` with no flags should still run the
same demos as before.

**Commit:** `Phase 2 step 4: --data flag`

---

## Step 5: `loadCards`, names only

**Why:** first get the file reading and the error handling right, using the easy fields.
Effects come next.

Replace the **entire** contents of `src/core/card_loader.cpp` with the code below. Four blanks,
all copy-the-pattern:

```cpp
#include "card_loader.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "effects.h"
#include "resolver.h"

namespace hex {
namespace {

using json = nlohmann::json;

// ---------- helpers (step 6) ----------

// ---------- kind builders (steps 6-10) ----------

// ---------- kind table (step 6) ----------

// Build ONE Card from ONE { ... } block of the JSON file.
Card loadCard(const json& j) {
    Card card;
    card.id       = j.at("id").get<std::string>();
    card.name     = /* ??? */;
    card.rune     = /* ??? */;
    card.text     = /* ??? */;
    card.echoable = j.value("echoable", true);      // optional field, default true

    // STEP 6: the "kind" lookup goes here. For now the effect stays empty.
    return card;
}

} // namespace

std::vector<Card> loadCards(const std::filesystem::path& file) {
    // 1. Open the file.
    std::ifstream in(file);
    if (!in) {
        throw std::runtime_error(std::format("cannot open '{}'", file.string()));
    }

    // 2. Parse it. Broken JSON throws; we turn that into a readable message.
    json root;
    try {
        root = json::parse(in);
    } catch (const json::parse_error& e) {
        throw std::runtime_error(std::format("{}: invalid JSON: {}", file.string(), e.what()));
    }

    // 3. Build one Card per entry in "cards". `index` tells us which card
    //    went wrong if something throws.
    std::vector<Card> cards;
    std::size_t index = 0;
    try {
        for (const json& j : /* ??? */) {
            cards.push_back(loadCard(j));
            ++index;
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::format("{}: card #{}: {}", file.string(), index, e.what()));
    }
    return cards;
}

} // namespace hex
```

How it's organised:
- `loadCard` builds **one** `Card` from **one** `{ ... }` block.
- `loadCards` opens the file, parses it, and calls `loadCard` for every entry in `"cards"`.
- Anything that goes wrong becomes a one-line error saying **which file and which card number**.
  `index` counts cards from 0.
- The three comment lines under `namespace {` are empty on purpose. You'll fill them in steps 6 to 10.

**Check:** list the cards that came from the file:

```text
$ ./build/hexcrawl_console --data data/cards.json --cards
ember       Ember Bolt   rune: triangle  Deal 6. Sets Fire.
frost       Frost Shard  rune: spiral    Deal 5. Sets Ice.
spark       Spark        rune: zigzag    Deal 3. First slot only: +10 more. Sets Storm.
overcharge  Overcharge   rune: cross     +1 mult for later strikes.
reverb      Reverb       rune: circle    Re-trigger the previous card. Resonant+: echoes play at Resonant at least.
wildfire    Wildfire     rune: wave      Deal 4. x2 if the chain element is Fire. Sets Fire.
```

Now break things on purpose and make sure each message makes sense. Work on a copy, never on the real file:

```bash
cp data/cards.json data/broken.json
```

Edit `data/broken.json` and run `./build/hexcrawl_console --data data/broken.json --cards` for each
of these. You should get messages like the ones below:

1. **Delete the `"name"` line of the second card:**

```text
$ ./build/hexcrawl_console --data data/broken.json --cards
error: data/broken.json: card #1: [json.exception.out_of_range.403] key 'name' not found
```

2. **Cut the file off halfway** (delete everything after a comma):

```text
$ ./build/hexcrawl_console --data data/broken.json --cards
error: data/broken.json: invalid JSON: [json.exception.parse_error.101] parse error at line 1, column 31: syntax error while parsing object key - unexpected end of input; expected string literal
```

3. **Point at a file that doesn't exist:**

```text
$ ./build/hexcrawl_console --data data/nope.json --cards
error: cannot open 'data/nope.json'
```

Delete `data/broken.json` when you're done.

> **Heads-up:** cards loaded in this step have no effect yet. If you try to play a chain with
> `--data` now, the program crashes with `bad_function_call`. That's expected until step 6.

**Commit:** `Phase 2 step 5: load card names from JSON`, then **push**.

---

## Step 6: Your first kind, `striker` (a worked example)

This one is a worked example, so read it first, then type it. Steps 7 to 10 reuse the same shape.

**6a. Helpers.** Under the comment `// ---------- helpers (step 6) ----------`, type this. It has
three small tools: turn `"fire"` into `Element::Fire`, read a `{ "base": .., "resonant": .. }`
number, and read the optional `"tech"` object. Two blanks:

```cpp
// "fire" -> Element::Fire, and so on. A typo in the JSON becomes a clear error.
Element parseElement(const std::string& name) {
    if (name == "none")  return Element::None;
    if (name == "fire")  return Element::Fire;
    // ice and storm follow the same pattern:
    /* ??? */
    /* ??? */
    throw std::runtime_error(std::format("unknown element '{}'", name));
}

// A number that depends on the rune tier, like { "base": 6, "resonant": 9 }.
struct Tiered {
    float base = 0.f;
    float resonant = 0.f;
    float at(RuneTier t) const { return byTier(t, base, resonant); }
};

Tiered readTiered(const json& card, const char* key) {
    const json& o = card.at(key);
    return { o.at("base").get<float>(), o.at("resonant").get<float>() };
}

// The optional hidden tech, like { "type": "add_mult", "amount": 0.5 }.
struct Tech {
    std::string type;       // empty = this card has no tech
    float amount = 0.f;
    int count = 0;
};

Tech readTech(const json& card) {
    Tech tech;
    if (!card.contains("tech")) return tech;
    const json& o = card.at("tech");
    tech.type   = o.at("type").get<std::string>();
    tech.amount = o.value("amount", 0.f);
    tech.count  = o.value("count", 0);
    return tech;
}
```

**6b. The builder.** Just **above** the comment `// ---------- kind table (step 6) ----------`, type:

```cpp
EffectFn makeStriker(const json& card) {
    // 1. Read everything we need from the JSON NOW, into plain C++ values.
    const Element element = parseElement(card.value("element", "none"));
    const Tiered  power   = readTiered(card, "power");
    const Tech    tech    = readTech(card);

    // 2. Build the effect. [=] COPIES element, power and tech into the lambda,
    //    so it keeps working after this function (and the json) is long gone.
    return [=](ChainState& s, RuneTier t) {
        if (element != Element::None) s.element = element;
        strike(s, power.at(t));             // power.at(t) = base or resonant number

        if (!isPerfect(t)) return;          // hidden tech only fires on Perfect
        if (tech.type == "ignite_next") {
            s.igniteNext = true;
            s.note("  hidden tech: the next card ends as Fire");
        } else if (tech.type == "add_mult") {
            s.mult += tech.amount;
            s.note(std::format("  hidden tech: +{:g} mult", tech.amount));
        }
    };
}
```

Read it slowly:
- **Part 1** reads everything from the JSON into plain variables (`element`, `power`, `tech`).
- **Part 2** returns a lambda. `[=]` copies those variables inside it, so it still works after
  `makeStriker` returns and the JSON is gone. This is exactly the rule from the start of the guide.
- Inside the lambda it's the same logic as the hard-coded Ember Bolt in `cards.cpp`:
  set the element, strike, and on Perfect apply the tech.

**6c. The table.** Under the comment `// ---------- kind table (step 6) ----------`, type:

```cpp
// A table from the "kind" text in the JSON to the function that builds that kind.
using Factory = EffectFn (*)(const json&);

const std::unordered_map<std::string, Factory>& kinds() {
    static const std::unordered_map<std::string, Factory> table = {
        {"striker", makeStriker},
    };
    return table;
}
```

It maps the text `"striker"` to the function `makeStriker`. The `static` means the table is
built once and reused.

**6d. Use the table.** In `loadCard`, replace the comment line
`// STEP 6: the "kind" lookup goes here. ...` with:

```cpp
    const std::string kind = j.at("kind").get<std::string>();
    const auto it = kinds().find(kind);
    if (it == kinds().end()) {
        throw std::runtime_error(std::format("unknown kind '{}'", kind));
    }
    card.effect = it->second(j);        // call the builder, keep the effect it returns
```

**Check, part 1.** The full file has kinds you haven't written yet, so loading it should now
**fail on purpose**:

```text
$ ./build/hexcrawl_console --data data/cards.json --cards
error: data/cards.json: card #2: unknown kind 'opener'
```

(Card #2 is `spark`, an `opener`. The error is correct!)

**Check, part 2.** Make a small test file `data/mini.json` with only the two strikers you've built.
(Easiest way: copy `cards.json` to `mini.json` and delete the four other cards. Or type it:)

```json
{
  "cards": [
    {
      "id": "ember",
      "name": "Ember Bolt",
      "rune": "triangle",
      "text": "Deal 6. Sets Fire.",
      "kind": "striker",
      "element": "fire",
      "power": { "base": 6, "resonant": 9 },
      "tech": { "type": "ignite_next" }
    },
    {
      "id": "frost",
      "name": "Frost Shard",
      "rune": "spiral",
      "text": "Deal 5. Sets Ice.",
      "kind": "striker",
      "element": "ice",
      "power": { "base": 5, "resonant": 8 },
      "tech": { "type": "add_mult", "amount": 0.5 }
    }
  ]
}
```

Then play some chains. Without a rune accuracy a card plays at Base. `:0.97` means "traced at 97%":

```text
$ ./build/hexcrawl_console --data data/mini.json ember:0.97 frost
== your chain ==
[1] Ember Bolt (Perfect)
  9 power x1 mult = 9 damage
  hidden tech: the next card ends as Fire
[2] Frost Shard (Base)
  5 power x1 mult = 5 damage
  ignited: chain element is now Fire
=> 14 damage  (final mult x1, element Fire)
```

```text
$ ./build/hexcrawl_console --data data/mini.json frost:0.97 ember
== your chain ==
[1] Frost Shard (Perfect)
  8 power x1 mult = 8 damage
  hidden tech: +0.5 mult
[2] Ember Bolt (Base)
  6 power x1.5 mult = 9 damage
=> 17 damage  (final mult x1.5, element Fire)
```

In the first, Ember is Perfect, so it deals 9 *and* ignites the next card. In the second, Frost
Perfect adds 0.5 to the multiplier, so Ember deals 6 x 1.5 = 9 (total 8 + 9 = 17).

**Commit:** `Phase 2 step 6: striker kind`, then **push**.

---

## Steps 7 to 10: the other four kinds

Every kind is the same four moves:

1. Type the builder **above** the `kind table` comment (fill the blanks).
2. Add one line to the table, under the previous kind's line.
3. Copy that card's whole `{ ... }` block from `cards.json` into `data/mini.json`.
   Put a **comma** between blocks, but not after the last one. A stray comma is the most common
   JSON mistake.
4. Build and run the check.

For each kind, look at the matching card in `cards.cpp`. It's your answer key, and the logic is the same.

### Step 7: `opener` (Spark)

```cpp
EffectFn makeOpener(const json& card) {
    const Element element = parseElement(card.value("element", "none"));
    const Tiered  power   = readTiered(card, "power");
    const Tiered  bonus   = readTiered(card, "opener_bonus");
    const Tech    tech    = readTech(card);

    return [=](ChainState& s, RuneTier t) {
        if (element != Element::None) s.element = element;

        float p = power.at(t);
        const bool opening = (s.slot == /* ??? */);       // slots count from 0: which one is first?
        if (opening) {
            p += /* ??? */;                      // the bonus number for this tier
            s.note("  opener bonus");
        }
        strike(s, p);

        if (opening && isPerfect(t) && tech.type == "add_mult") {
            /* ??? */                 // one whole statement, ending in ;  (same as the striker's add_mult)
            s.note(std::format("  hidden tech: +{:g} mult", tech.amount));
        }
    };
}
```

Table line: `{"opener", makeOpener},`  Card to copy into `mini.json`: `spark`.

**Check:** the opener bonus only fires in the first slot, so the order of cards matters:

```text
$ ./build/hexcrawl_console --data data/mini.json spark ember | grep '^=>'
=> 19 damage  (final mult x1, element Fire)
```

```text
$ ./build/hexcrawl_console --data data/mini.json ember spark | grep '^=>'
=> 9 damage  (final mult x1, element Storm)
```

### Step 8: `amplifier` (Overcharge)

```cpp
EffectFn makeAmplifier(const json& card) {
    const Tiered gain = readTiered(card, "mult_gain");
    const Tech   tech = readTech(card);

    return [=](ChainState& s, RuneTier t) {
        const float g = gain.at(t);

        if (isPerfect(t) && tech.type == "retroactive") {
            // Damage dealt so far is scaled as if the NEW mult had been there all along:
            // ratio = (mult after) / (mult before).
            const float ratio = /* ??? */;
            s.damage *= ratio;
            s.note(std::format("  hidden tech: damage so far x{:.2f}", ratio));
        }

        s.mult += /* ??? */;
        s.note(std::format("  mult is now {:g}", s.mult));
    };
}
```

Table line: `{"amplifier", makeAmplifier},`  Card to copy: `overcharge`.

**Check:** the multiplier only helps cards *after* it (12 versus 6). The last chain is Overcharge
traced Perfect: the retroactive tech scales the 6 damage already dealt by 2.5, giving 15:

```text
$ ./build/hexcrawl_console --data data/mini.json overcharge ember | grep '^=>'
=> 12 damage  (final mult x2, element Fire)
```

```text
$ ./build/hexcrawl_console --data data/mini.json ember overcharge | grep '^=>'
=> 6 damage  (final mult x2, element Fire)
```

```text
$ ./build/hexcrawl_console --data data/mini.json ember overcharge:0.97 | grep '^=>'
=> 15 damage  (final mult x2.5, element Fire)
```

### Step 9: `echo` (Reverb)

This is the longest one. It replays earlier cards, using `retrigger()` from `resolver.h`.

```cpp
EffectFn makeEcho(const json& card) {
    const std::size_t baseCount = card.value("count", 1);
    const bool floorOnResonant  = card.value("resonant_floor", false);
    const Tech tech             = readTech(card);

    return [=](ChainState& s, RuneTier t) {
        // How many cards to echo: normally baseCount, but a Perfect trace + tech changes it.
        std::size_t count = baseCount;
        if (isPerfect(t) && tech.type == "echo_count") {
            count = /* ??? */;   // the tech's own "count" number
        }

        // The tier echoed cards play at: Resonant at least, if this card asks for it.
        const RuneTier floor =
            (floorOnResonant && t != RuneTier::Base) ? RuneTier::Resonant : RuneTier::Base;

        // Walk the history BACKWARDS and collect up to `count` echoable cards.
        std::vector<HistoryEntry> picks;
        for (auto it = s.history.rbegin(); it != s.history.rend() && picks.size() < count; ++it) {
            if (/* ??? */) picks.push_back(*it);   // skip cards that must not be echoed
        }
        std::reverse(picks.begin(), picks.end());      // replay in the order they were played

        if (picks.empty()) { s.note("  nothing to echo"); return; }
        for (const HistoryEntry& e : picks) {
            /* ??? */                  // one whole statement, ending in ;  (see retrigger() in resolver.h)
        }
    };
}
```

Table line: `{"echo", makeEcho},`  Card to copy: `reverb`.

**Check:** Reverb replays Spark's opener bonus, so Spark's 13 happens twice, plus Ember's 6 = 32.
Then try two Reverbs in a row. It must not crash, because Reverb is marked `"echoable": false` in
the JSON, so the second Reverb skips the first and echoes Spark again (13 + 13 + 13 = 39):

```text
$ ./build/hexcrawl_console --data data/mini.json spark reverb ember | grep '^=>'
=> 32 damage  (final mult x1, element Fire)
```

```text
$ ./build/hexcrawl_console --data data/mini.json spark reverb reverb | grep '^=>'
=> 39 damage  (final mult x1, element Storm)
```

### Step 10: `conditional` (Wildfire)

```cpp
EffectFn makeConditional(const json& card) {
    // (Don't name a variable `requires`: it is a keyword in C++20.)
    const Element element  = parseElement(card.value("element", "none"));
    const Element required = parseElement(card.at("requires_element").get<std::string>());
    const Tiered  power    = readTiered(card, "power");
    const float multiplier = card.value("multiplier", 2.f);
    const Tech    tech     = readTech(card);

    return [=](ChainState& s, RuneTier t) {
        const bool techSkipsCheck = isPerfect(t) && tech.type == "ignore_condition";

        float p = power.at(t);
        if (/* ??? */) {   // matches, OR the tech says "pretend it does"
            p *= multiplier;
            s.note("  element bonus");
        }
        if (element != Element::None) s.element = element;   // set the new element AFTER the check
        strike(s, p);
    };
}
```

Table line: `{"conditional", makeConditional},`  Card to copy: `wildfire`.

**Check:** Wildfire doubles only if the chain element is already Fire:

```text
$ ./build/hexcrawl_console --data data/mini.json ember wildfire | grep '^=>'
=> 14 damage  (final mult x1, element Fire)
```

```text
$ ./build/hexcrawl_console --data data/mini.json frost wildfire | grep '^=>'
=> 9 damage  (final mult x1, element Fire)
```

**Commit** after each kind: `Phase 2 step 7: opener kind`, and so on.

---

## Step 11: The final check

Now `mini.json` isn't needed. Run the real check against the full `cards.json`:

```bash
cmake --build build
bash scripts/check_phase2.sh
```

```text
$ bash scripts/check_phase2.sh
PASS: all demo totals match
```

`PASS` means the data-driven cards behave **exactly** like the hard-coded ones, including the
big Demo 7 combo. If it says `FAIL`, the output shows which totals differ (`<` is yours,
`>` is what's expected). See the next section.

**Commit:** `Phase 2 step 11: cards load from JSON`, then **push**. Tick "Phase 2, part 1" in
`docs/ROADMAP.md`.

`playground.cpp` was only for learning. Keep it for reference or delete it, your choice.

---

## When something goes wrong

| What you see | Usually means |
|---|---|
| `undefined reference to hex::loadCards` | `card_loader.cpp` isn't in the CMake source list (step 1a). |
| `fatal error: nlohmann/json.hpp: No such file` | That target doesn't link the library. `hexcrawl_core` needs the step 1b link line. `playground` needs its own `target_link_libraries` line. |
| Red squiggles on `#include <nlohmann/json.hpp>` but it builds | The editor is out of date. Run `cmake -B build`, then restart clangd (`Ctrl+Shift+P`, "clangd: Restart language server"). |
| The compiler complains right where you still have `/* ??? */` | That blank isn't filled in yet. |
| `unknown kind 'xyz'` | You haven't written that builder yet, or forgot its line in the table. |
| `unknown card 'xyz'` | That card isn't in the JSON file you passed (`mini.json` only has what you copied into it), or the id is mistyped. |
| `parse error ... line N` | A JSON typo near line N: a missing or extra comma, a missing quote. |
| `bad_function_call` | You played a card with no effect: you're before step 6, or its kind didn't register. |
| A strange crash after loading, only sometimes | The classic: a lambda captured the `json` (or a reference to it) instead of copies. Capture plain values with `[=]`. |
| `check_phase2.sh` says FAIL | Run the failing chain by hand **both ways** and compare the log lines: `./build/hexcrawl_console spark:0.97 reverb:0.97 ember` (built-in cards) and the same with `--data data/cards.json`. The first line that differs is your bug. |

Variable named `requires`? It's a keyword in C++20. Pick another name (the guide uses `required`).

If you're still stuck, paste your code and the full error message to me.

---

## Stretch ideas (optional, in any order)

- **Prove it's data-driven:** add a new striker, `gale` (Storm, power 4/7), to `cards.json` only.
  No C++ changes. Play it.
- **Duplicate ids:** two cards with the same `id` should be a load error.
- **A kinder unknown-kind error:** list the valid kinds in the message.
- **Catch tech mistakes at load time:** `retroactive` on a striker does nothing. Make that an error.
- **A `finisher` kind** that ends the chain, if you wrote Detonate earlier.
- **Clean up:** `cards.cpp` has its own copies of `byTier`, `isPerfect` and `strike`.
  Make it use `effects.h` instead. If the compiler says a call is *ambiguous*, work out why.
- **CI:** add `bash scripts/check_phase2.sh build/hexcrawl_console` to the `core` job in
  `.github/workflows/ci.yml` so GitHub checks your data on every push.

## Next: Phase 2, part 2: relics

Relic hooks inside the resolver loop (Twin Wick: the first card triggers twice), also
data-driven. Start when step 11 passes.
