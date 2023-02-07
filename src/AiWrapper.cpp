#include "AiWrapper.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/mesh.h"
#include "assimp/postprocess.h"

#include "Tracy.hpp"

namespace AMD {
void ExtractAiScene(const char *path, std::vector<Vertex> &vertices, std::vector<unsigned int> &indices) {
    const aiScene *scene = Assimp::Importer().ReadFile(path,
                                             aiProcess_ConvertToLeftHanded |
                                             aiProcessPreset_TargetRealtime_MaxQuality);

    std::vector<aiNode*> stack;
    stack.push_back(scene->mRootNode);

    while (stack.size() > 0) {
        aiNode *current = stack.back();
        stack.pop_back();

        std::string out("Found ");
        out.append(std::to_string(current->mNumChildren));
        out.append(" Children currents from ");
        out.append(current->mName.C_Str());
        TracyMessage(out.c_str(), out.size());

        out = "Found ";
        out.append(std::to_string(current->mNumMeshes));
        out.append(" Meshes from ");
        out.append(current->mName.C_Str());
        TracyMessage(out.c_str(), out.size());

        for (unsigned int i = 0; i < current->mNumMeshes; i++) {
            // Extract all the indices
            aiMesh *m = scene->mMeshes[current->mMeshes[i]];

            for (unsigned int j = 0; j < m->mNumVertices; j++) {
                auto pos = m->mVertices[j];
                auto uv = m->mTextureCoords[0][j];

                vertices.insert(vertices.begin(), {{ pos.x, pos.y, pos.z },{ uv.x, uv.y }} );
            }

            for (unsigned int j = 0; j < m->mNumFaces; j++) {
                aiFace f = m->mFaces[j];

                for (unsigned int k = 0; k < f.mNumIndices; k++) {
                    std::string out (std::to_string(f.mIndices[k]));
                    TracyMessage(out.c_str(), out.size());

                    indices.insert(indices.begin(), f.mIndices[k]);
                }
            }
        }

        for (unsigned int i = 0; i < current->mNumChildren; i++) {
            stack.push_back(current->mChildren[i]);
        }
    }
}
}
