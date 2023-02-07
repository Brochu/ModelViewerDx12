#include <vector>

namespace AMD {
struct Vertex {
    float position[3];
    float uv[2];
};

struct Draws {
    std::vector<size_t> indexCounts;

    std::vector<size_t> vertexOffsets;
    std::vector<size_t> indexOffsets;

    unsigned int numDraws;
};

Draws ExtractAiScene(const char *path, std::vector<Vertex> &vertices, std::vector<unsigned int> &indices);
}
