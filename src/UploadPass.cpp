#define NOMINMAX
#include "UploadPass.h"
#include "ImageIO.h"

#include "Tracy.hpp"
#include "d3dx12.h"

using namespace Microsoft::WRL;

namespace AMD {
namespace Meshes {
void CreateMeshBuffers (
    const std::string &folder,
    const Microsoft::WRL::ComPtr<ID3D12Device> &device,
    Draws &draws,
    std::vector<Material> &mats,
    Microsoft::WRL::ComPtr<ID3D12Resource> &uploadBuffer,
    Microsoft::WRL::ComPtr<ID3D12Resource> &vertexBuffer,
    D3D12_VERTEX_BUFFER_VIEW &vertexBufferView,
    Microsoft::WRL::ComPtr<ID3D12Resource> &indexBuffer,
    D3D12_INDEX_BUFFER_VIEW &indexBufferView,
    ID3D12GraphicsCommandList* uploadCommandList)
{
    std::string path = folder + "/model.obj";
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    draws = ExtractAiScene(path.c_str(), vertices, indices, mats);
    unsigned int vertexCount = (UINT) vertices.size();
    unsigned int indexCount = (UINT) indices.size();

    static const int uploadBufferSize = vertexCount * sizeof(Vertex) + indexCount * sizeof(unsigned int);
    static const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_UPLOAD);
    static const auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (uploadBufferSize);

    // Create upload buffer on CPU
    device->CreateCommittedResource (&uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS (&uploadBuffer));

    // Create vertex & index buffer on the GPU
    // HEAP_TYPE_DEFAULT is on GPU, we also initialize with COPY_DEST state
    // so we don't have to transition into this before copying into them
    static const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_DEFAULT);

    static const auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (vertexCount * sizeof(Vertex));
    device->CreateCommittedResource (&defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS (&vertexBuffer));

    static const auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (indexCount * sizeof(unsigned int));
    device->CreateCommittedResource (&defaultHeapProperties,
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
    uploadBuffer->Map (0, nullptr, &p);
    ::memcpy (p, vertices.data(), vertexCount * sizeof(Vertex));
    ::memcpy (static_cast<unsigned char*>(p) + vertexCount * sizeof(Vertex),
        indices.data(), indexCount * sizeof(unsigned int));
    uploadBuffer->Unmap (0, nullptr);

    // Copy data from upload buffer on CPU into the index/vertex buffer on 
    // the GPU
    uploadCommandList->CopyBufferRegion (vertexBuffer.Get (), 0,
        uploadBuffer.Get (), 0, vertexCount * sizeof(Vertex));
    uploadCommandList->CopyBufferRegion (indexBuffer.Get (), 0,
        uploadBuffer.Get (), vertexCount * sizeof(Vertex), indexCount * sizeof(unsigned int));

    // Barriers, batch them together
    const CD3DX12_RESOURCE_BARRIER barriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition (vertexBuffer.Get (),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
        CD3DX12_RESOURCE_BARRIER::Transition (indexBuffer.Get (),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER)
    };

    uploadCommandList->ResourceBarrier (2, barriers);
}
}

namespace Textures {
void UploadTextures (
        const std::string &folder,
        const ComPtr<ID3D12Device> &device,
        const ComPtr<ID3D12DescriptorHeap> &srvHeap,
        const std::vector<Material> &mats,
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> &imgs,
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> &uploadImgs,
        ID3D12GraphicsCommandList* uploadCommandList)
{
    static const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_DEFAULT);

    for (size_t i = 0; i < mats.size(); i++) {
        //TODO: This is pretty ugly, why do we have textures with empty names?
        imgs.emplace_back();
        uploadImgs.emplace_back();
        
        Material m = mats[i];
        if (m.textureName.size() <= 0) continue;

        int width = 0, height = 0;
        std::string path = folder + m.textureName;
        std::vector<std::uint8_t> imgdata = LoadImageFromFile(path.c_str(), 1, &width, &height);

        std::string out(path);
        out.append("; size = ");
        out.append(std::to_string(imgdata.size()));
        TracyMessage(out.c_str(), out.size());

        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D ( DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, width, height, 1, 1 );
        device->CreateCommittedResource (&defaultHeapProperties,
            D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS (&imgs[i]));

        static const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_UPLOAD);
        const auto uploadBufferSize = GetRequiredIntermediateSize (imgs[i].Get (), 0, 1);
        const auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (uploadBufferSize);

        device->CreateCommittedResource (&uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS (&uploadImgs[i]));

        D3D12_SUBRESOURCE_DATA srcData;
        srcData.pData = imgdata.data ();
        srcData.RowPitch = width * 4;
        srcData.SlicePitch = width * height * 4;

        UpdateSubresources (uploadCommandList, imgs[i].Get (), uploadImgs[i].Get (), 0, 0, 1, &srcData);
        const auto transition = CD3DX12_RESOURCE_BARRIER::Transition (imgs[i].Get (),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        uploadCommandList->ResourceBarrier (1, &transition);

        D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
        shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        shaderResourceViewDesc.Texture2D.MipLevels = 1;
        shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
        shaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        UINT incrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
        CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted (srvHandle,
            srvHeap->GetCPUDescriptorHandleForHeapStart (),
            (UINT)i, incrementSize);

        device->CreateShaderResourceView (imgs[i].Get (), &shaderResourceViewDesc, srvHandle);
    }
}
}
}
