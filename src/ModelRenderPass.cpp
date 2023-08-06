#include "ModelRenderPass.h"
#include <d3dx12.h>

using namespace Microsoft::WRL;

namespace AMD {
ModelRenderPass::ModelRenderPass(ComPtr<ID3D12Device> &device) {
    device_ = device;

    device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&uploadCmdAlloc_));
    device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        uploadCmdAlloc_.Get(), nullptr, IID_PPV_ARGS(&uploadCmdList_));
}

ModelRenderPass::~ModelRenderPass() {
    uploadCmdAlloc_->Reset();
}

void ModelRenderPass::Execute(ID3D12CommandQueue *queue) {
}

void ModelRenderPass::CreateRootSignature() {
    // We have two root parameters, one is a pointer to a descriptor heap
    // with a SRV, the second is a constant buffer view
    CD3DX12_ROOT_PARAMETER parameters[3];

    // Create a descriptor table with one entry in our descriptor heap
    CD3DX12_DESCRIPTOR_RANGE range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 256, 0 };
    parameters[0].InitAsDescriptorTable (1, &range);

    // Our constant buffer view
    parameters[1].InitAsConstantBufferView (0);

    // Per draw texture index
    parameters[2].InitAsConstants(4, 1);

    // We don't use another descriptor heap for the sampler, instead we use a
    // static sampler
    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init (0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

    CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;

    // Create the root signature
    descRootSignature.Init (3, parameters,
        1, samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootBlob;
    ComPtr<ID3DBlob> errorBlob;
    D3D12SerializeRootSignature (&descRootSignature,
        D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

    device_->CreateRootSignature (0,
        rootBlob->GetBufferPointer (),
        rootBlob->GetBufferSize (), IID_PPV_ARGS (&rootSignature_));

}

}
