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
                c.models.push_back(line);
            }
        }
    }

    return c;
}
}
