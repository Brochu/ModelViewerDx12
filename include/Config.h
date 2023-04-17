#pragma once

#include <vector>
#include <string>

namespace AMD{

struct ModelEntry {
    std::string folder;
    std::vector<std::string> files;
};

struct Config {
    std::vector<ModelEntry> models;
};

Config ParseConfig();
}
