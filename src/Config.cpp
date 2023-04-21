#include "Config.h"
#include <fstream>

void SplitOnce(const std::string& in, char separator, std::string& first, std::string& second) {
    first = { in };
    second = {};

    const size_t idx = in.find(separator);
    if (idx != std::string::npos) {
        first = { in.substr(0, idx) };
        second = { in.substr(idx + 1) };
    }
}

void SplitFolderFile(const std::string& in, char separator, std::string& first, std::string& second) {
    first = {};
    second = { in };

    const size_t idx = in.find_last_of(separator);
    if (idx != std::string::npos) {
        first = { "/" + in.substr(0, idx) };
        second = { in.substr(idx + 1) };
    }
}

AMD::ModelEntry ParseEntry(const std::string& line) {
    std::vector<AMD::ModelFile> files;

    std::string base, file;
    SplitOnce(line, '|', base, file);

    while (!file.empty()) {
        std::string current, rest;
        SplitOnce(file, ';', current, rest);
        std::string subfolder, f;
        SplitFolderFile(current, '/', subfolder, f);

        //TODO: If we find a * in the name, find all files with given extension
        files.push_back({ base + subfolder, f });
        file = rest;
    }

    //TODO: Find a way to find a list of offsets for each sub models

    return { files };
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
