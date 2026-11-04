// Console harness: play chains in the terminal and watch the numbers.
//
//   hexcrawl_console                      run the built-in demos
//   hexcrawl_console --cards              list all cards
//   hexcrawl_console ember spark:97 ...   resolve your own chain
//
// Each argument is  id  or  id:accuracy.  Accuracy is 0-1 or 0-100
// (so 0.9 and 90 mean the same thing). No accuracy = 0 = Base tier.

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "cards.h"
#include "resolver.h"

using namespace hex;

namespace {

bool parseChain(const std::vector<std::string>& specs, std::vector<PlayedCard>& out) {
    for (const std::string& spec : specs) {
        const std::size_t colon = spec.find(':');
        const std::string id = spec.substr(0, colon);
        float accuracy = 0.f;
        if (colon != std::string::npos) {
            accuracy = std::strtof(spec.c_str() + colon + 1, nullptr);
            if (accuracy > 1.f) accuracy /= 100.f;
        }
        const Card* card = findCard(id);
        if (!card) {
            std::fprintf(stderr, "unknown card '%s' (try --cards)\n", id.c_str());
            return false;
        }
        out.push_back({card, accuracy});
    }
    return true;
}

int run(const char* title, const std::vector<std::string>& specs, bool verbose = true) {
    std::vector<PlayedCard> chain;
    if (!parseChain(specs, chain)) return -1;

    const ChainResult r = resolveChain(chain);

    std::printf("== %s ==\n", title);
    if (verbose) {
        for (const std::string& line : r.log) std::printf("%s\n", line.c_str());
    }
    std::printf("=> %d damage  (final mult x%g, element %s)\n\n",
                r.damage, r.mult, elementName(r.element));
    return r.damage;
}

void listCards() {
    for (const Card& c : allCards()) {
        std::printf("%-11s %-12s rune: %-9s %s\n",
                    c.id.c_str(), c.name.c_str(), c.rune.c_str(), c.text.c_str());
    }
}

void demos() {
    std::printf("Hexcrawl resolver demo\n\n");

    run("1. Ember Bolt alone",                  {"ember"});
    run("2a. Overcharge BEFORE the strike",      {"overcharge", "ember"});
    run("2b. Overcharge AFTER the strike",       {"ember", "overcharge"});
    run("3a. Spark in slot 1 (opener bonus)",    {"spark", "ember"});
    run("3b. Spark in slot 2 (no bonus)",        {"ember", "spark"});
    run("4. Reverb re-triggers the opener",      {"spark", "reverb", "ember"});
    run("5a. Wildfire after Ice (no bonus)",     {"frost", "wildfire"});
    run("5b. Wildfire after Fire (x2)",          {"ember", "wildfire"});
    run("6a. Ember -> Frost -> Wildfire, all sloppy", {"ember", "frost", "wildfire"});
    run("6b. Same chain, Ember traced Perfect",  {"ember:0.97", "frost", "wildfire"});
    run("7. Everything traced well",
        {"spark:0.96", "overcharge:0.97", "reverb:0.96", "ember:0.90", "wildfire:0.99"});
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 1) { demos(); return 0; }

    const std::string first = argv[1];
    if (first == "--cards") { listCards(); return 0; }
    if (first == "--help" || first == "-h") {
        std::printf("usage: hexcrawl_console [--cards | id[:accuracy] ...]\n");
        return 0;
    }

    std::vector<std::string> specs(argv + 1, argv + argc);
    return run("your chain", specs) < 0 ? 1 : 0;
}
