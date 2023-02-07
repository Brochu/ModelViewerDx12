#include "AiWrapper.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/mesh.h"
#include "assimp/postprocess.h"

namespace AMD {
Draws ExtractAiScene(const char *path, std::vector<Vertex> &vertices, std::vector<unsigned int> &indices) {
    Draws draws;

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path,
                                             aiProcess_ConvertToLeftHanded |
                                             aiProcessPreset_TargetRealtime_MaxQuality);

    std::vector<aiNode*> stack;
    stack.push_back(scene->mRootNode);

    while (stack.size() > 0) {
        aiNode *current = stack.back();
        stack.pop_back();

        for (unsigned int i = 0; i < current->mNumMeshes; i++) {
            draws.vertexOffsets.push_back(vertices.size());
            draws.indexOffsets.push_back(indices.size());

            aiMesh *m = scene->mMeshes[current->mMeshes[i]];

            for (unsigned int j = 0; j < m->mNumVertices; j++) {
                auto pos = m->mVertices[j];
                auto uv = m->mTextureCoords[0][j];

                vertices.push_back({{ pos.x, pos.y, pos.z },{ uv.x, uv.y }} );
            }

            for (unsigned int j = 0; j < m->mNumFaces; j++) {
                aiFace f = m->mFaces[j];

                for (unsigned int k = 0; k < f.mNumIndices; k++) {
                    indices.push_back(f.mIndices[k]);
                }
            }

            draws.indexCounts.push_back(indices.size() - draws.indexOffsets[draws.indexOffsets.size() - 1]);
            draws.numDraws++;
        }

        for (unsigned int i = 0; i < current->mNumChildren; i++) {
            stack.push_back(current->mChildren[i]);
        }
    }

    //TODO: Fill draw calls in return struct
    return draws;
}
}
