#pragma once

#include <vector>
#include <string>

namespace AMD{

struct GroupEntry {
    std::string name;
    std::vector<size_t> modelrefs;
};

struct ModelEntry {
    std::string folder;
    std::vector<std::string> files;
};

struct Config {
    std::vector<GroupEntry> groups;
    std::vector<ModelEntry> models;
};

Config ParseConfig();
}
