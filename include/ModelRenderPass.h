#pragma once

#include <d3d12.h>
#include <wrl.h>

namespace AMD {
struct ModelRenderPass {
    ModelRenderPass(Microsoft::WRL::ComPtr<ID3D12Device> &device);
    ~ModelRenderPass();

    void Execute(ID3D12CommandQueue *queue);

private:
    void CreateRootSignature();
    void CreatePipelineStateObject();

    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> uploadCmdAlloc_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> uploadCmdList_;
};
}
