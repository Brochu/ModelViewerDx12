#pragma once
#include "AiWrapper.h"

#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>

namespace AMD {
void UploadTextures (
        const std::string &folder,
        const Microsoft::WRL::ComPtr<ID3D12Device> &device,
        const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &srvHeap,
        const std::vector<Material> &mats,
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> &imgs,
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> &uploadImgs,
        ID3D12GraphicsCommandList* uploadCommandList);
}
