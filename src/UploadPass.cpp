#define NOMINMAX
#include "UploadPass.h"
#include "ImageIO.h"

#include "Tracy.hpp"
#include "d3dx12.h"

using namespace Microsoft::WRL;

namespace AMD {
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
