#include <cstdio>
#include <string>
#include <fstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char** argv) {

    const std::string text = R"({
        "name": "Ember Bolt",
        "power": {"base": 6, "resonant" : 9 }
        })";
    
        json card = json::parse(text);


        const std::string name = card.at("name").get<std::string>();
        std::printf("name = %s\n", name.c_str());

        const float base     = card.at("power").at("base").get<float>();
        const float resonant = card.at("power").at("resonant").get<float>();
        std::printf("base = %g, resonant = %g\n", base, resonant);
        
        const bool echoable = card.value("echoable",true);
        std::printf("echoable = %d\n", echoable);

        try{
            card.at("kind");
        } catch (const json::exception& e) {
            std::printf("error: %s\n",e.what());
        }

        if (argc < 2) return 0;                 // no file given: stop here
        std::ifstream in(argv[1]);

        if (!in) { std::printf("cannot open %s\n", argv[1]); return 1; }
        const json root = json::parse(in);

        // root["cards"] is an ARRAY. A range-for walks it one element at a time.
        for (const json& c : root.at("cards")) {
            const std::string cardName = c.at("name").get<std::string>();
            const std::string kind     = c.at("kind").get<std::string>();
            std::printf("%-12s kind=%s\n", cardName.c_str(), kind.c_str());
        }

}
