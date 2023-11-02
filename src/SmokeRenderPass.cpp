#include "SmokeRenderPass.h"
#include "Utility.h"

#include <d3dcompiler.h>
#include <d3dx12.h>
#include <vector>
#include <DirectXMath.h>

using namespace Microsoft::WRL;
using namespace DirectX;

namespace AMD {
struct SmokeCBuffer {
    XMFLOAT4 bgColor;
    XMFLOAT4 values;
};

const char *smokeShader_ = "shaders/smoke.hlsl";

SmokeRenderPass::SmokeRenderPass() { }

SmokeRenderPass::~SmokeRenderPass() { }

void SmokeRenderPass::Prepare(ComPtr<ID3D12Device> &device) {
    device_ = device;

    CreateRootSignature();
    CreatePipelineStateObject();
    CreateConstantBuffer();
}
void SmokeRenderPass::Execute(ComPtr<ID3D12GraphicsCommandList> &renderCmdList, int backBufferIndex) {
    UpdateConstantBuffer(backBufferIndex);

    // Set our state (shaders, etc.)
    renderCmdList->SetPipelineState (pso_.Get ());

    // Set our root signature
    renderCmdList->SetGraphicsRootSignature (rootSignature_.Get ());

    // Set slot 0 of our root signature to the constant buffer view
    renderCmdList->SetGraphicsRootConstantBufferView (0, constBuffer_[backBufferIndex]->GetGPUVirtualAddress());

    // Draw full screen tri for smoke effect
    renderCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    renderCmdList->DrawInstanced(3, 1, 0, 0);
}

void SmokeRenderPass::CreateRootSignature() {
    if (rootSignature_.Get() != nullptr) return;

    // We have one parameter: a constant buffer to send debug params to smoke shader
    CD3DX12_ROOT_PARAMETER parameters[1];
    parameters[0].InitAsConstantBufferView (0);

    // Maybe we will need a static sample for later?
    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init (0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

    CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
    descRootSignature.Init (1, parameters,
                            1, samplers,
                            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    );

    ComPtr<ID3DBlob> rootBlob;
    ComPtr<ID3DBlob> errorBlob;
    D3D12SerializeRootSignature (&descRootSignature,
        D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

    device_->CreateRootSignature (0,
        rootBlob->GetBufferPointer (),
        rootBlob->GetBufferSize (), IID_PPV_ARGS (&rootSignature_));
}

void SmokeRenderPass::CreatePipelineStateObject() {
    if (pso_.Get() != nullptr) return;

    std::vector<unsigned char> code = ReadFile(smokeShader_);

    static const D3D_SHADER_MACRO macros[] = {
        { nullptr, nullptr }
    };

    ComPtr<ID3DBlob> error;
    ComPtr<ID3DBlob> vertexShader;
    D3DCompile (code.data(), code.size(),
        "", macros, nullptr,
        "VS_main", "vs_5_1", 0, 0, &vertexShader, &error);
    if (error != nullptr) {
        const char *vserr = reinterpret_cast<const char*>(error->GetBufferPointer());
    }

    ComPtr<ID3DBlob> pixelShader;
    D3DCompile (code.data(), code.size(),
        "", macros, nullptr,
        "PS_main", "ps_5_1", 0, 0, &pixelShader, &error);
    if (error != nullptr) {
        const char *pserr = reinterpret_cast<const char*>(error->GetBufferPointer());
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = {nullptr, 0};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.VS.BytecodeLength = vertexShader->GetBufferSize();
    psoDesc.VS.pShaderBytecode = vertexShader->GetBufferPointer();
    psoDesc.PS.BytecodeLength = pixelShader->GetBufferSize();
    psoDesc.PS.pShaderBytecode = pixelShader->GetBufferPointer();
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = false;
    psoDesc.DepthStencilState.StencilEnable = false;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc.Count = 1;

    device_->CreateGraphicsPipelineState (&psoDesc, IID_PPV_ARGS (&pso_));
}

void SmokeRenderPass::CreateConstantBuffer() {
    for (int i = 0; i < QUEUE_SLOT_COUNT; i++) {
        D3D12_HEAP_PROPERTIES uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(SmokeCBuffer));

        device_->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&constBuffer_[i])
        );

        UpdateConstantBuffer(i);
    }
}

void SmokeRenderPass::UpdateConstantBuffer(int backBufferIndex) {
    SmokeCBuffer cb {};
    //TODO: Dynamic values change here
    cb.bgColor = { 0.57f, 0.77f, 0.92f, 1.0f };
    cb.values = { 1.0, 1.0, 1.0, 0.0 };

    UINT8 *p;
    CD3DX12_RANGE readRange(0, 0);
    constBuffer_[backBufferIndex]->Map(0, &readRange, reinterpret_cast<void **>(&p));
    memcpy(p, &cb, sizeof(cb));
    constBuffer_[backBufferIndex]->Unmap(0, nullptr);
}

}
