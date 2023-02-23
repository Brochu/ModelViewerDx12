#pragma once

#include <d3d12.h>

namespace AMD {
namespace Mesh {
void CreateMeshBuffers (ID3D12GraphicsCommandList* uploadCommandList);
}

namespace Texture {
void CreateTexture (ID3D12GraphicsCommandList* uploadCommandList);
}
}
