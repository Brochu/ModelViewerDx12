#include "Config.h"
#include <fstream>

namespace AMD {
const char *configFile = "config.ini";

Config ParseConfig() {
    Config c {};
    std::fstream fs(configFile);
    std::string line;

    while (std::getline(fs, line)) {
        if (line[0] == '[' && line.find("models") != std::string::npos) {
            // Parsing possible models to render
            std::string line;

            while (std::getline(fs, line)) {
                if (line.size() == 0) break;

                const auto idx = line.find('|');
                const std::string folder = line.substr(0, idx);
                const std::string file = line.substr(idx + 1);

                c.models.push_back({folder, {file} });
                //TODO: Handle sub models found in the selected model folder
                // Return an array of all scenes to combine
                // Find a way to find a list of offsets for each sub models
            }
        }
    }

    return c;
}
}
