#include "Config.h"
#include <fstream>

AMD::ModelEntry ParseEntry(const std::string& line) {
    const auto idx = line.find('|');
    const std::string folder = line.substr(0, idx);
    const std::string file = line.substr(idx + 1);

    //TODO: Handle sub models found in the selected model folder
    // Return an array of all scenes to combine
    // Find a way to find a list of offsets for each sub models

    return { folder, {file} };
};

AMD::GroupEntry ParseGroup(const std::string& line, const std::vector<std::string>& models, AMD::Config& c) {
    std::vector<size_t> refs;
    for (const auto& m : models) {
        refs.emplace_back(c.models.size());
        c.models.push_back(ParseEntry(m));
    }

    return { line, refs };
}

void ParseModelSection(std::fstream& fs, AMD::Config& c) {
    // Parsing possible models to render
    std::string groupLine;

    while (std::getline(fs, groupLine)) {
        if (groupLine.size() == 0) break;

        std::vector<std::string> modelLines{};
        std::string line;

        while (std::getline(fs, line)) {
            if (line.size() == 0 || line[0] != '-') break;
            modelLines.emplace_back(line.substr(1));
        }

        c.groups.push_back(ParseGroup(groupLine, modelLines, c));
    }
}

namespace AMD {
const char *configFile = "config.ini";

Config ParseConfig() {
    Config c {};
    std::fstream fs(configFile);
    std::string line;

    while (std::getline(fs, line)) {
        if (strcmp(line.c_str(), "[models]") == 0) {
            ParseModelSection(fs, c);
        }
    }

    return c;
}
}
