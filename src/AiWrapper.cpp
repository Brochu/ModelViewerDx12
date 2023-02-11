#include "AiWrapper.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/material.h"
#include "assimp/mesh.h"
#include "assimp/postprocess.h"

#include "Tracy.hpp"

namespace AMD {

Draws ExtractAiScene(
    const char *path,
    std::vector<Vertex> &vertices,
    std::vector<unsigned int> &indices,
    std::vector<Material> &materials)
{
    Draws draws { };

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path,
                                             aiProcess_ConvertToLeftHanded |
                                             aiProcessPreset_TargetRealtime_MaxQuality | 
                                             aiProcess_PreTransformVertices);

    std::vector<aiNode*> stack;
    stack.push_back(scene->mRootNode);

    while (stack.size() > 0) {
        aiNode *current = stack.back();
        stack.pop_back();

        for (unsigned int i = 0; i < current->mNumMeshes; i++) {
            // Track vert/ind offsets for next draw call
            draws.vertexOffsets.push_back(vertices.size());
            draws.indexOffsets.push_back(indices.size());

            aiMesh *m = scene->mMeshes[current->mMeshes[i]];
            aiMaterial *mat = scene->mMaterials[m->mMaterialIndex];

            //TODO: Need to handle extracting texture information
            std::string out("MatIdx = ");
            out.append(std::to_string(m->mMaterialIndex));
            out.append("; textures name = ");
            aiString texName {};
            auto tex = mat->GetTexture(aiTextureType_DIFFUSE, 0, &texName);
            out.append(texName.C_Str());
            TracyMessage(out.c_str(), out.size());

            for (unsigned int j = 0; j < m->mNumVertices; j++) {
                const auto pos = m->mVertices[j];
                const auto normal = m->mNormals[j];
                const auto uv = m->mTextureCoords[0][j];

                vertices.push_back(
                    {{ pos.x, pos.y, pos.z },
                    { normal.x, normal.y, normal.z },
                    { uv.x, uv.y }}
                );
            }

            for (unsigned int j = 0; j < m->mNumFaces; j++) {
                aiFace f = m->mFaces[j];

                for (unsigned int k = 0; k < f.mNumIndices; k++) {
                    indices.push_back(f.mIndices[k]);
                }
            }

            // Indices that were added need to be drawn
            draws.indexCounts.push_back(indices.size() - draws.indexOffsets[draws.indexOffsets.size() - 1]);
            draws.numDraws++;
        }

        for (unsigned int i = 0; i < current->mNumChildren; i++) {
            // Breath first traversal of assimp tree
            stack.push_back(current->mChildren[i]);
        }
    }

    return draws;
}
}
