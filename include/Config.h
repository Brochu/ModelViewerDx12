#pragma once

#include <vector>
#include <string>

namespace AMD{

struct GroupEntry {
    std::string name;
    std::vector<size_t> modelrefs;
};

struct ModelFile {
    std::string folder;
    std::string file;
};

struct ModelEntry {
    std::vector<ModelFile> files;
};

struct Config {
    std::vector<GroupEntry> groups;
    std::vector<ModelEntry> models;
};

Config ParseConfig();
}
