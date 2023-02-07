#include <vector>

namespace AMD {
struct Vertex {
    float position[3];
    float uv[2];
};

struct Draws {
    std::vector<unsigned int> indexCounts;

    std::vector<unsigned int> vertexOffsets;
    std::vector<unsigned int> indexOffsets;

    unsigned int numDraws;
};

void ExtractAiScene(const char *path, std::vector<Vertex> &vertices, std::vector<unsigned int> &indices);
}
