#pragma once
#include "AiWrapper.h"

#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>

namespace AMD {
struct UploadPass {
    UploadPass(Microsoft::WRL::ComPtr<ID3D12Device> &device);
    ~UploadPass();

    void CreateMeshBuffers (
        std::vector<Vertex> &vertices,
        std::vector<unsigned int> &indices,
        Microsoft::WRL::ComPtr<ID3D12Resource> &vertexBuffer,
        D3D12_VERTEX_BUFFER_VIEW &vertexBufferView,
        Microsoft::WRL::ComPtr<ID3D12Resource> &indexBuffer,
        D3D12_INDEX_BUFFER_VIEW &indexBufferView);

    void UploadTextures (
        const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &srvHeap,
        const std::vector<Texture> &textures,
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> &imgs);

    void Execute(ID3D12CommandQueue *queue);
    void WaitForUpload();

private:
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12Fence> uploadFence_;

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> uploadCmdAlloc_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> uploadCmdList_;

    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer_;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadImages_;
};
}
