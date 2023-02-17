// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#define NOMINMAX
#include "D3D12TexturedQuad.h"
#include "ImageIO.h"
#include "RubyTexture.h"
#include "Utility.h"

#include "Tracy.hpp"

#include <cmath>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <fstream>
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

using namespace Microsoft::WRL;

namespace AMD {
struct ConstantBuffer
{
    DirectX::XMMATRIX mvp;
    DirectX::XMMATRIX world;
    DirectX::XMVECTOR lightPos;
};

D3D12TexturedQuad::D3D12TexturedQuad() { }
D3D12TexturedQuad::D3D12TexturedQuad(int modelOverride) {
    modelIndex_ = modelOverride;
}

///////////////////////////////////////////////////////////////////////////////
void D3D12TexturedQuad::CreateTexture (ID3D12GraphicsCommandList * uploadCommandList)
{
    static const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_DEFAULT);

    for (size_t i = 0; i < materials_.size(); i++) {
        //TODO: This is pretty ugly, why do we have textures with empty names?
        image_.emplace_back();
        uploadImage_.emplace_back();
        
        Material m = materials_[i];
        if (m.textureName.size() <= 0) continue;

        int width = 0, height = 0;
        std::string path = "data/models/" + models_[modelIndex_] + "/" + m.textureName;
        std::vector<std::uint8_t> imgdata = LoadImageFromFile(path.c_str(), 1, &width, &height);

        std::string out(path);
        out.append("; size = ");
        out.append(std::to_string(imgdata.size()));
        TracyMessage(out.c_str(), out.size());

        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D ( DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, width, height, 1, 1 );
        device_->CreateCommittedResource (&defaultHeapProperties,
            D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS (&image_[i]));

        static const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_UPLOAD);
        const auto uploadBufferSize = GetRequiredIntermediateSize (image_[i].Get (), 0, 1);
        const auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (uploadBufferSize);

        device_->CreateCommittedResource (&uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS (&uploadImage_[i]));

        D3D12_SUBRESOURCE_DATA srcData;
        srcData.pData = imgdata.data ();
        srcData.RowPitch = width * 4;
        srcData.SlicePitch = width * height * 4;

        UpdateSubresources (uploadCommandList, image_[i].Get (), uploadImage_[i].Get (), 0, 0, 1, &srcData);
        const auto transition = CD3DX12_RESOURCE_BARRIER::Transition (image_[i].Get (),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        uploadCommandList->ResourceBarrier (1, &transition);

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
            srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart (),
            (UINT)i, incrementSize);

        device_->CreateShaderResourceView (image_[i].Get (), &shaderResourceViewDesc, srvHandle);
    }
}

///////////////////////////////////////////////////////////////////////////////
void D3D12TexturedQuad::RenderImpl (ID3D12GraphicsCommandList * commandList)
{
    D3D12Sample::RenderImpl (commandList);

    UpdateConstantBuffer ();

    // Set the descriptor heap containing the texture srv
    ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap_.Get () };
    commandList->SetDescriptorHeaps (1, heaps);

    // Set slot 1 of our root signature to the constant buffer view
    commandList->SetGraphicsRootConstantBufferView (1,
        constantBuffers_[GetQueueSlot ()]->GetGPUVirtualAddress ());

    commandList->IASetPrimitiveTopology (D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers (0, 1, &vertexBufferView_);
    commandList->IASetIndexBuffer (&indexBufferView_);
    for (size_t i = 0; i < draws_.numDraws; i++) {
        //TODO: Find out how I can use texture arrays later, just want to know if texture memory is okay
        // Set slot 0 of our root signature to point to our descriptor heap with
        // the texture SRV
        UINT incSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_GPU_DESCRIPTOR_HANDLE start = srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
        CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle {};
        srvHandle.InitOffsetted(start, (INT)draws_.materialIndices[i], incSize);
        commandList->SetGraphicsRootDescriptorTable (0, srvHandle);

        commandList->SetGraphicsRoot32BitConstant(2, 0, 0);
        commandList->DrawIndexedInstanced (
            (INT)draws_.indexCounts[i],
            1,
            (INT)draws_.indexOffsets[i],
            (INT)draws_.vertexOffsets[i],
            0
        );
    }

    // Imgui logic ---------------------
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
        // Main debug parameters window
        ImGui::Begin("ModelViewer - Parameters");

        ImGui::Separator();
        ImGui::Text("Background");
        ImGui::ColorEdit3("clear color", clearColor_);

        ImGui::Separator();
        ImGui::Text("Model Viewer");

        assert(models_.size() <= 32);
        char* models[32];
        for (int i = 0; i < models_.size(); i++) {
            models[i] = models_[i].data();
        }
        if (ImGui::Combo("Model", &modelIndex_, models, (int)models_.size())) {
            // Start thinking on how to reload model data when selecting a new model
        }

        ImGui::Separator();
        ImGui::Text("Transform");
        ImGui::DragFloat3("translate", translate_, 0.1f, -100.f, 100.f);
        ImGui::DragFloat3("rotate", rotate_, 1.f, -359.f, 359.f);
        ImGui::DragFloat3("scale", scale_, 0.01f, 0.f, 10.f);

        ImGui::Separator();
        ImGui::Text("Camera");
        ImGui::DragFloat3("position", camPos_, 1.f, -500.f, 500.f);
        ImGui::DragFloat3("focus", lookAt_, 1.f, -500.f, 500.f);
        ImGui::DragFloat("FOV", &fov_, 0.25f, 5.f, 110.f);

        ImGui::Separator();
        ImGui::Text("Light");
        ImGui::DragFloat3("pos", lightPos_, 1.f, -500.f, 500.f);

        ImGui::Separator();
        ImGui::End();
    }
    ImGui::Render();

    ID3D12DescriptorHeap* imguiHeaps[] = { imguiDescriptorHeap_.Get() };
    commandList->SetDescriptorHeaps(1, imguiHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

///////////////////////////////////////////////////////////////////////////////
void D3D12TexturedQuad::InitializeImpl (ID3D12GraphicsCommandList * uploadCommandList)
{
    D3D12Sample::InitializeImpl (uploadCommandList);
    LoadConfig ();

    // We need one descriptor heap to store our texture SRV which cannot go
    // into the root signature. So create a SRV type heap with one entry
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = 256;
    // This heap contains SRV, UAV or CBVs -- in our case one SRV
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descriptorHeapDesc.NodeMask = 0;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    device_->CreateDescriptorHeap (&descriptorHeapDesc, IID_PPV_ARGS (&srvDescriptorHeap_));
    device_->CreateDescriptorHeap (&descriptorHeapDesc, IID_PPV_ARGS (&imguiDescriptorHeap_));

    CreateRootSignature ();
    CreatePipelineStateObject ();
    CreateConstantBuffer ();
    CreateMeshBuffers (uploadCommandList);
    CreateTexture (uploadCommandList);

    // Imgui render side init
    ImGui_ImplDX12_Init(device_.Get(),
                        QUEUE_SLOT_COUNT,
                        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                        imguiDescriptorHeap_.Get(),
                        imguiDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
                        imguiDescriptorHeap_->GetGPUDescriptorHandleForHeapStart());
}

///////////////////////////////////////////////////////////////////////////////
void D3D12TexturedQuad::CreateMeshBuffers (ID3D12GraphicsCommandList* uploadCommandList)
{
    //TODO: How do we call this again after changing the selected model
    std::string path = "data/models/" + models_[modelIndex_] + "/model.obj";
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    draws_ = ExtractAiScene(path.c_str(), vertices, indices, materials_);
    unsigned int vertexCount = (UINT) vertices.size();
    unsigned int indexCount = (UINT) indices.size();

    static const int uploadBufferSize = vertexCount * sizeof(Vertex) + indexCount * sizeof(unsigned int);
    static const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_UPLOAD);
    static const auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (uploadBufferSize);

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

    static const auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (vertexCount * sizeof(Vertex));
    device_->CreateCommittedResource (&defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS (&vertexBuffer_));

    static const auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (indexCount * sizeof(unsigned int));
    device_->CreateCommittedResource (&defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &indexBufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS (&indexBuffer_));

    // Create buffer views
    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress ();
    vertexBufferView_.SizeInBytes = vertexCount * sizeof(Vertex);
    vertexBufferView_.StrideInBytes = sizeof (Vertex);

    indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress ();
    indexBufferView_.SizeInBytes = indexCount * sizeof(unsigned int);
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    // Copy data on CPU into the upload buffer
    void* p;
    uploadBuffer_->Map (0, nullptr, &p);
    ::memcpy (p, vertices.data(), vertexCount * sizeof(Vertex));
    ::memcpy (static_cast<unsigned char*>(p) + vertexCount * sizeof(Vertex),
        indices.data(), indexCount * sizeof(unsigned int));
    uploadBuffer_->Unmap (0, nullptr);

    // Copy data from upload buffer on CPU into the index/vertex buffer on 
    // the GPU
    uploadCommandList->CopyBufferRegion (vertexBuffer_.Get (), 0,
        uploadBuffer_.Get (), 0, vertexCount * sizeof(Vertex));
    uploadCommandList->CopyBufferRegion (indexBuffer_.Get (), 0,
        uploadBuffer_.Get (), vertexCount * sizeof(Vertex), indexCount * sizeof(unsigned int));

    // Barriers, batch them together
    const CD3DX12_RESOURCE_BARRIER barriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition (vertexBuffer_.Get (),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
        CD3DX12_RESOURCE_BARRIER::Transition (indexBuffer_.Get (),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER)
    };

    uploadCommandList->ResourceBarrier (2, barriers);
}

///////////////////////////////////////////////////////////////////////////////
void D3D12TexturedQuad::CreateConstantBuffer ()
{
    static const ConstantBuffer cb = {
        DirectX::XMMatrixIdentity(),
        DirectX::XMMatrixIdentity(),
        DirectX::XMVectorSet(0.0 ,0.0, 0.0, 1.0)
    };

    for (int i = 0; i < GetQueueSlotCount (); ++i) {
        static const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_UPLOAD);
        static const auto constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (sizeof (ConstantBuffer));

        // These will remain in upload heap because we use them only once per
        // frame.
        device_->CreateCommittedResource (&uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &constantBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS (&constantBuffers_[i]));

        void* p;
        constantBuffers_[i]->Map (0, nullptr, &p);
        ::memcpy (p, &cb, sizeof (cb));
        constantBuffers_[i]->Unmap (0, nullptr);
    }
}

///////////////////////////////////////////////////////////////////////////////
void D3D12TexturedQuad::UpdateConstantBuffer ()
{
    using namespace DirectX;

    XMMATRIX model = DirectX::XMMatrixScaling(
            scale_[0],
            scale_[1],
            scale_[2]
        );
    model = DirectX::XMMatrixMultiply(model, DirectX::XMMatrixRotationRollPitchYaw(
                                          DirectX::XMConvertToRadians(rotate_[0]),
                                          DirectX::XMConvertToRadians(rotate_[1]),
                                          DirectX::XMConvertToRadians(rotate_[2])
                                      ));
    model = DirectX::XMMatrixMultiply(model, DirectX::XMMatrixTranslation(
                                          translate_[0],
                                          translate_[1],
                                          translate_[2]
                                      ));

    XMVECTOR camPos = XMVectorSet(camPos_[0], camPos_[1], camPos_[2], 1.0f);
    XMVECTOR lookAt = XMVectorSet(lookAt_[0], lookAt_[1], lookAt_[2], 1.0f );
    XMVECTOR up = XMVectorSet(0.0, 1.0, 0.0, 0.0);
    XMMATRIX view = DirectX::XMMatrixLookAtLH(camPos, lookAt, up);

    // Projection
    float aspect = (float)width_ / (float)height_;
    XMMATRIX projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(fov_), aspect, 0.1f, 100000.f);

    XMMATRIX mvp = XMMatrixMultiply(model, view);
    mvp = XMMatrixMultiply(mvp, projection);
    XMMATRIX world = XMMatrixTranspose(model); // Need to convert local space to world space

    ConstantBuffer cb { mvp, world, XMVectorSet(lightPos_[0], lightPos_[1], lightPos_[2], 1.f) };

    void* p;
    constantBuffers_[GetQueueSlot ()]->Map (0, nullptr, &p);
    ::memcpy(p, &cb, sizeof(ConstantBuffer));
    constantBuffers_[GetQueueSlot ()]->Unmap (0, nullptr);
}

///////////////////////////////////////////////////////////////////////////////
void D3D12TexturedQuad::CreateRootSignature ()
{
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

///////////////////////////////////////////////////////////////////////////////
void D3D12TexturedQuad::CreatePipelineStateObject ()
{
    std::vector<unsigned char> code = ReadFile(shaderFile_);

    static const D3D12_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    static const D3D_SHADER_MACRO macros[] = {
        { nullptr, nullptr }
    };

    ComPtr<ID3DBlob> error;
    ComPtr<ID3DBlob> vertexShader;
    D3DCompile (code.data(), code.size(),
        "", macros, nullptr,
        "VS_main", "vs_5_1", 0, 0, &vertexShader, &error);
    //unsigned char *vserr = reinterpret_cast<unsigned char*>(error->GetBufferPointer());

    ComPtr<ID3DBlob> pixelShader;
    D3DCompile (code.data(), code.size(),
        "", macros, nullptr,
        "PS_main", "ps_5_1", 0, 0, &pixelShader, &error);
    //unsigned char *pserr = reinterpret_cast<unsigned char*>(error->GetBufferPointer());

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.VS.BytecodeLength = vertexShader->GetBufferSize ();
    psoDesc.VS.pShaderBytecode = vertexShader->GetBufferPointer ();
    psoDesc.PS.BytecodeLength = pixelShader->GetBufferSize ();
    psoDesc.PS.pShaderBytecode = pixelShader->GetBufferPointer ();
    psoDesc.pRootSignature = rootSignature_.Get ();
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.InputLayout.NumElements = std::extent<decltype(layout)>::value;
    psoDesc.InputLayout.pInputElementDescs = layout;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC (D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC (D3D12_DEFAULT);
    // Simple alpha blending
    psoDesc.BlendState.RenderTarget[0].BlendEnable = true;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = 0xFFFFFFFF;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    device_->CreateGraphicsPipelineState (&psoDesc, IID_PPV_ARGS (&pso_));
}

void D3D12TexturedQuad::LoadConfig ()
{
    std::fstream fs(configFile_);
    std::string line;

    while (std::getline(fs, line)) {
        if (line[0] == '[' && line.find("models") != std::string::npos) {
            // Parsing possible models to render
            std::string line;

            while (std::getline(fs, line)) {
                if (line.size() == 0) break;
                models_.push_back(line);
            }
        }
    }
}
}
