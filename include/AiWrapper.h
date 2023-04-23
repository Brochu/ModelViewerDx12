#pragma once

#include <vector>
#include <string>

namespace AMD {
struct Vertex {
    float position[3];
    float normal[3];
    float uv[2];
};

struct Material {
    std::string textureName;
};

struct Texture {
    int width;
    int height;
    std::vector<std::uint8_t> data;
};

struct Draws {
    std::vector<size_t> indexCounts;

    std::vector<size_t> vertexOffsets;
    std::vector<size_t> indexOffsets;

    std::vector<unsigned int> materialIndices;

    unsigned int numDraws;
};

void ExtractAiScene(
    const char *path,
    Draws &draws,
    std::vector<Vertex> &vertices,
    std::vector<unsigned int> &indices,
    std::vector<Material> &materials,
    size_t matOffset);
}
