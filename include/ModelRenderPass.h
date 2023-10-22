#pragma once
#include "AiWrapper.h"

#include <d3d12.h>
#include <wrl.h>

namespace AMD {
struct ModelRenderPass {
    ModelRenderPass();
    ~ModelRenderPass();

    void Prepare(Microsoft::WRL::ComPtr<ID3D12Device> &device);
    void Execute(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &renderCmdList,
                 Draws &draws,
                 D3D12_VERTEX_BUFFER_VIEW &vertBufferView,
                 D3D12_INDEX_BUFFER_VIEW &idxBufferView,
                 Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer,
                 Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap);

private:
    void CreateRootSignature();
    void CreatePipelineStateObject();

    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;
};
}
