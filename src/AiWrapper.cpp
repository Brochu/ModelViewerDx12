#include "AiWrapper.h"

#include "assimp/Importer.hpp"
#include "assimp/material.h"
#include "assimp/mesh.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

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

            draws.materialIndices.push_back(m->mMaterialIndex);

            // Indices that were added need to be drawn
            draws.indexCounts.push_back(indices.size() - draws.indexOffsets[draws.indexOffsets.size() - 1]);
            draws.numDraws++;
        }

        for (unsigned int i = 0; i < current->mNumChildren; i++) {
            // Breath first traversal of assimp tree
            stack.push_back(current->mChildren[i]);
        }
    }

    // To load the textures needed later, we need to extract the materials' data
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        //TODO: Maybe handle different texture types ?
        // All the models I test now use the DIFFUSE textures only
        aiString texPath;
        scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);

        materials.push_back({ texPath.C_Str() });
    }

    return draws;
}
}
