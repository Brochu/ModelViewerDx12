#pragma once
#include "AiWrapper.h"

#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>

namespace AMD {
namespace Meshes {
void CreateMeshBuffers (
    const std::string &folder,
    const Microsoft::WRL::ComPtr<ID3D12Device> &device,
    Draws &draws,
    std::vector<Material> &mats,
    Microsoft::WRL::ComPtr<ID3D12Resource> &uploadBuffer,
    Microsoft::WRL::ComPtr<ID3D12Resource> &vertexBuffer,
    D3D12_VERTEX_BUFFER_VIEW &vertexBufferView,
    Microsoft::WRL::ComPtr<ID3D12Resource> &indexBuffer,
    D3D12_INDEX_BUFFER_VIEW &indexBufferView,
    ID3D12GraphicsCommandList* uploadCommandList);
}

namespace Textures {
void UploadTextures (
        const std::string &folder,
        const Microsoft::WRL::ComPtr<ID3D12Device> &device,
        const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &srvHeap,
        const std::vector<Material> &mats,
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> &imgs,
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> &uploadImgs,
        ID3D12GraphicsCommandList* uploadCommandList);
}
}
