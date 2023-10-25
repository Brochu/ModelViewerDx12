#include "SmokeRenderPass.h"
#include "Utility.h"

#include <d3dcompiler.h>
#include <d3dx12.h>
#include <vector>

using namespace Microsoft::WRL;

namespace AMD {
const char *smokeShader_ = "shaders/smoke.hlsl";

SmokeRenderPass::SmokeRenderPass() { }

SmokeRenderPass::~SmokeRenderPass() { }

void SmokeRenderPass::Prepare(ComPtr<ID3D12Device> &device) {
    device_ = device;

    CreateRootSignature();
    CreatePipelineStateObject();
}
void SmokeRenderPass::Execute(ComPtr<ID3D12GraphicsCommandList> &renderCmdList) {
    // --------------------------------------
    // Set our state (shaders, etc.)
    //renderCmdList->SetPipelineState (pso_.Get ());

    // Set our root signature
    //renderCmdList->SetGraphicsRootSignature (rootSignature_.Get ());

    // Set the descriptor heap containing the texture srv
    //ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap.Get () };
    //renderCmdList->SetDescriptorHeaps (1, heaps);

    // Set slot 0 of our root signature to point to our descriptor heap with
    // the texture SRV
    //renderCmdList->SetGraphicsRootDescriptorTable (0, srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    // Set slot 1 of our root signature to the constant buffer view
    //renderCmdList->SetGraphicsRootConstantBufferView (1, constantBuffer->GetGPUVirtualAddress());

    //renderCmdList->IASetPrimitiveTopology (D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //renderCmdList->IASetVertexBuffers (0, 1, &vertBufferView);
    //renderCmdList->IASetIndexBuffer (&idxBufferView);

    //for (size_t i = 0; i < draws.numDraws; i++) {
    //    renderCmdList->SetGraphicsRoot32BitConstant(2, draws.materialIndices[i], 0);
    //    renderCmdList->DrawIndexedInstanced (
    //        (INT)draws.indexCounts[i],
    //        1,
    //        (INT)draws.indexOffsets[i],
    //        (INT)draws.vertexOffsets[i],
    //        0
    //    );
    //}
}

void SmokeRenderPass::CreateRootSignature() {
    //if (rootSignature_.Get() != nullptr) return;

    // We have two root parameters, one is a pointer to a descriptor heap
    // with a SRV, the second is a constant buffer view
    //CD3DX12_ROOT_PARAMETER parameters[3];

    // Create a descriptor table with one entry in our descriptor heap
    //CD3DX12_DESCRIPTOR_RANGE range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 256, 0 };
    //parameters[0].InitAsDescriptorTable (1, &range);

    // Our constant buffer view
    //parameters[1].InitAsConstantBufferView (0);

    // Per draw texture index
    //parameters[2].InitAsConstants(4, 1);

    // We don't use another descriptor heap for the sampler, instead we use a
    // static sampler
    //CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    //samplers[0].Init (0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

    //CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;

    // Create the root signature
    //descRootSignature.Init (3, parameters,
    //    1, samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    //ComPtr<ID3DBlob> rootBlob;
    //ComPtr<ID3DBlob> errorBlob;
    //D3D12SerializeRootSignature (&descRootSignature,
    //    D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

    //device_->CreateRootSignature (0,
    //    rootBlob->GetBufferPointer (),
    //    rootBlob->GetBufferSize (), IID_PPV_ARGS (&rootSignature_));

}

void SmokeRenderPass::CreatePipelineStateObject() {
    //if (pso_.Get() != nullptr) return;

    //std::vector<unsigned char> code = ReadFile(shaderFile_);

    //static const D3D12_INPUT_ELEMENT_DESC layout[] =
    //{
    //    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
    //    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
    //    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
    //    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    //};

    //static const D3D_SHADER_MACRO macros[] = {
    //    { nullptr, nullptr }
    //};

    //ComPtr<ID3DBlob> error;
    //ComPtr<ID3DBlob> vertexShader;
    //D3DCompile (code.data(), code.size(),
    //    "", macros, nullptr,
    //    "VS_main", "vs_5_1", 0, 0, &vertexShader, &error);
    //unsigned char *vserr = reinterpret_cast<unsigned char*>(error->GetBufferPointer());

    //ComPtr<ID3DBlob> pixelShader;
    //D3DCompile (code.data(), code.size(),
    //    "", macros, nullptr,
    //    "PS_main", "ps_5_1", 0, 0, &pixelShader, &error);
    //unsigned char *pserr = reinterpret_cast<unsigned char*>(error->GetBufferPointer());

    //D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    //psoDesc.VS.BytecodeLength = vertexShader->GetBufferSize ();
    //psoDesc.VS.pShaderBytecode = vertexShader->GetBufferPointer ();
    //psoDesc.PS.BytecodeLength = pixelShader->GetBufferSize ();
    //psoDesc.PS.pShaderBytecode = pixelShader->GetBufferPointer ();
    //psoDesc.pRootSignature = rootSignature_.Get ();
    //psoDesc.NumRenderTargets = 1;
    //psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    //psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    //psoDesc.InputLayout.NumElements = std::extent<decltype(layout)>::value;
    //psoDesc.InputLayout.pInputElementDescs = layout;
    //psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC (D3D12_DEFAULT);
    //psoDesc.BlendState = CD3DX12_BLEND_DESC (D3D12_DEFAULT);
    //// Simple alpha blending
    ////TODO: I think blending works okay, but the draws are not sorted so looks strange
    //psoDesc.BlendState.RenderTarget[0].BlendEnable = true;
    //psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    //psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    //psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    //psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    //psoDesc.SampleDesc.Count = 1;
    //psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    //psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
    //psoDesc.SampleMask = 0xFFFFFFFF;
    //psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    //device_->CreateGraphicsPipelineState (&psoDesc, IID_PPV_ARGS (&pso_));
}

}
