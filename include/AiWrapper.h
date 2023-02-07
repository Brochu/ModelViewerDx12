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
};

void ExtractAiScene();
}
