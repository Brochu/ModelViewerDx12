#pragma once

#include <vector>
#include <string>

namespace AMD{

struct ModelEntry {
    std::string folder;
    std::string file;
};

struct Config {
    std::vector<ModelEntry> models;
};

Config ParseConfig();
}
