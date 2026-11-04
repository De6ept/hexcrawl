#include <cstdio>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {

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
        
        const bool echoable = card.value("echoable",false);
        std::printf("echoable = %d\n", echoable);

        try{
            card.at("kind");
        } catch (const json::exception& e) {
            std::printf("error: %s\n",e.what());
        }

        return 0;

}
