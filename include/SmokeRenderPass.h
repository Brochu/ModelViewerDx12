#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

#include <wrl.h>

namespace AMD {
struct SmokeRenderPass {
    SmokeRenderPass();
    ~SmokeRenderPass();

    void Prepare(Microsoft::WRL::ComPtr<ID3D12Device> &device);
    void Update(int backBufferIndex,
                float *smokePos,
                DirectX::XMMATRIX mvp,
                float sigmaa,
                float distmult);
    void Execute(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &renderCmdList, int backBufferIndex);

private:
    void CreateRootSignature();
    void CreatePipelineStateObject();
    void CreateConstantBuffer();
    void UpdateConstantBuffer(int backBufferIndex,
                              float *smokePos,
                              DirectX::XMMATRIX mvp,
                              float sigmaa,
                              float distmult);

    static const int QUEUE_SLOT_COUNT = 3;

    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;

    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_[QUEUE_SLOT_COUNT];
    DirectX::XMMATRIX mvp_[QUEUE_SLOT_COUNT];
};
}
