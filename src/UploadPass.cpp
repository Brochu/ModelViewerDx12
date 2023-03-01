#include <d3d12.h>
#include <stdexcept>
#define NOMINMAX
#include "UploadPass.h"
#include "ImageIO.h"

#include "d3dx12.h"

using namespace Microsoft::WRL;

namespace AMD {
UploadPass::UploadPass(ComPtr<ID3D12Device> &device) {
    device_ = device;
    // Create our upload fence, command list and command allocator
    // This will be only used while creating the mesh buffer and the texture
    // to upload data to the GPU.
    device_->CreateFence (0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS (&uploadFence_));

    device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&uploadCmdAlloc_));
    device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        uploadCmdAlloc_.Get(), nullptr, IID_PPV_ARGS(&uploadCmdList_));
}

UploadPass::~UploadPass() {
    uploadCmdAlloc_->Reset();

    uploadBuffer_.Reset();
    for (auto& img : uploadImages_) {
        img.Reset();
    }
    uploadImages_.clear();
}

void UploadPass::CreateMeshBuffers (
    std::vector<Vertex> &vertices,
    std::vector<unsigned int> &indices,
    ComPtr<ID3D12Resource> &vertexBuffer,
    D3D12_VERTEX_BUFFER_VIEW &vertexBufferView,
    ComPtr<ID3D12Resource> &indexBuffer,
    D3D12_INDEX_BUFFER_VIEW &indexBufferView)
{
    unsigned int vertexCount = (UINT) vertices.size();
    unsigned int indexCount = (UINT) indices.size();

    const int uploadBufferSize = vertexCount * sizeof(Vertex) + indexCount * sizeof(unsigned int);
    const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_UPLOAD);
    const auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (uploadBufferSize);

    // Create upload buffer on CPU
    device_->CreateCommittedResource (&uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS (&uploadBuffer_));

    // Create vertex & index buffer on the GPU
    // HEAP_TYPE_DEFAULT is on GPU, we also initialize with COPY_DEST state
    // so we don't have to transition into this before copying into them
    static const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_DEFAULT);

    const auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (vertexCount * sizeof(Vertex));
    device_->CreateCommittedResource (&defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS (&vertexBuffer));

    const auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (indexCount * sizeof(unsigned int));
    device_->CreateCommittedResource (&defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &indexBufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS (&indexBuffer));

    // Create buffer views
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress ();
    vertexBufferView.SizeInBytes = vertexCount * sizeof(Vertex);
    vertexBufferView.StrideInBytes = sizeof (Vertex);

    indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress ();
    indexBufferView.SizeInBytes = indexCount * sizeof(unsigned int);
    indexBufferView.Format = DXGI_FORMAT_R32_UINT;

    // Copy data on CPU into the upload buffer
    void* p;
    uploadBuffer_->Map (0, nullptr, &p);
    ::memcpy (p, vertices.data(), vertexCount * sizeof(Vertex));
    ::memcpy (static_cast<unsigned char*>(p) + vertexCount * sizeof(Vertex),
        indices.data(), indexCount * sizeof(unsigned int));
    uploadBuffer_->Unmap (0, nullptr);

    // Copy data from upload buffer on CPU into the index/vertex buffer on 
    // the GPU
    uploadCmdList_->CopyBufferRegion (vertexBuffer.Get (), 0,
        uploadBuffer_.Get (), 0, vertexCount * sizeof(Vertex));
    uploadCmdList_->CopyBufferRegion (indexBuffer.Get (), 0,
        uploadBuffer_.Get (), vertexCount * sizeof(Vertex), indexCount * sizeof(unsigned int));

    // Barriers, batch them together
    const CD3DX12_RESOURCE_BARRIER barriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition (vertexBuffer.Get (),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
        CD3DX12_RESOURCE_BARRIER::Transition (indexBuffer.Get (),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER)
    };

    uploadCmdList_->ResourceBarrier (2, barriers);
}

void UploadPass::UploadTextures (
        const ComPtr<ID3D12DescriptorHeap> &srvHeap,
        const std::vector<Texture> &textures,
        std::vector<ComPtr<ID3D12Resource>> &imgs)
{
    static const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_DEFAULT);

    for (size_t i = 0; i < textures.size(); i++) {
        //TODO: This is pretty ugly, why do we have textures with empty names?
        imgs.emplace_back();
        uploadImages_.emplace_back();
        
        Texture t = textures[i];
        if (t.data.size() <= 0) continue;

        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D (
                DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                t.width, t.height,
                1, 1 );
        device_->CreateCommittedResource (&defaultHeapProperties,
            D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS (&imgs[i]));

        static const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_UPLOAD);
        const auto uploadBufferSize = GetRequiredIntermediateSize (imgs[i].Get (), 0, 1);
        const auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (uploadBufferSize);

        device_->CreateCommittedResource (&uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS (&uploadImages_[i]));

        D3D12_SUBRESOURCE_DATA srcData;
        srcData.pData = t.data.data ();
        srcData.RowPitch = t.width * 4;
        srcData.SlicePitch = t.width * t.height * 4;

        UpdateSubresources (uploadCmdList_.Get(), imgs[i].Get (), uploadImages_[i].Get (), 0, 0, 1, &srcData);
        const auto transition = CD3DX12_RESOURCE_BARRIER::Transition (imgs[i].Get (),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        uploadCmdList_->ResourceBarrier (1, &transition);

        D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
        shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        shaderResourceViewDesc.Texture2D.MipLevels = 1;
        shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
        shaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        UINT incrementSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
        CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted (srvHandle,
            srvHeap->GetCPUDescriptorHandleForHeapStart (),
            (UINT)i, incrementSize);

        device_->CreateShaderResourceView (imgs[i].Get (), &shaderResourceViewDesc, srvHandle);
    }
}

void UploadPass::Execute(ID3D12CommandQueue *queue) {
    uploadCmdList_->Close();

    ID3D12CommandList* commandLists [] = { uploadCmdList_.Get() };
    queue->ExecuteCommandLists (std::extent<decltype(commandLists)>::value, commandLists);
    queue->Signal (uploadFence_.Get (), 1);
}

void UploadPass::WaitForUpload() {
    auto waitEvent = CreateEvent (nullptr, FALSE, FALSE, nullptr);

    if (waitEvent == NULL) {
        throw std::runtime_error ("Could not create wait event.");
    }

    if (uploadFence_->GetCompletedValue () < 1) {
        uploadFence_->SetEventOnCompletion (1, waitEvent);
        WaitForSingleObject (waitEvent, INFINITE);
    }
    CloseHandle (waitEvent);
}
}
