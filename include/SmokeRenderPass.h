#pragma once
#include <d3d12.h>
#include <wrl.h>

namespace AMD {
struct SmokeRenderPass {
    SmokeRenderPass();
    ~SmokeRenderPass();

    void Prepare(Microsoft::WRL::ComPtr<ID3D12Device> &device);
    void Execute(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &renderCmdList);

private:
    void CreateRootSignature();
    void CreatePipelineStateObject();
    void CreateConstantBuffer();

    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;

    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
};
}
