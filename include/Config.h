#pragma once

#include <vector>
#include <string>

namespace AMD{

struct Config {
    std::vector<std::string> models;
};

Config ParseConfig();
}
