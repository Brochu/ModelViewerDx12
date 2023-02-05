//
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

#include "D3D12TexturedQuad.h"
#include "ImageIO.h"
#include "RubyTexture.h"
#include "Utility.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/mesh.h"
#include "assimp/postprocess.h"

#include "d3dx12.h"
#include "d3dcompiler.h"
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "Tracy.hpp"

#include <cmath>
#include <fstream>
#include <string>

using namespace Microsoft::WRL;

namespace AMD {
///////////////////////////////////////////////////////////////////////////////
void D3D12TexturedQuad::CreateTexture (ID3D12GraphicsCommandList * uploadCommandList)
{
    int width = 0, height = 0;

    imageData_ = LoadImageFromMemory (RubyTexture, sizeof (RubyTexture),
        1 /* tight row packing */, &width, &height);

    static const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_DEFAULT);
    const auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D (DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, width, height, 1, 1);

    device_->CreateCommittedResource (&defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS (&image_));

    static const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_UPLOAD);
    const auto uploadBufferSize = GetRequiredIntermediateSize (image_.Get (), 0, 1);
    const auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (uploadBufferSize);

    device_->CreateCommittedResource (&uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS (&uploadImage_));

    D3D12_SUBRESOURCE_DATA srcData;
    srcData.pData = imageData_.data ();
    srcData.RowPitch = width * 4;
    srcData.SlicePitch = width * height * 4;

    UpdateSubresources (uploadCommandList, image_.Get (), uploadImage_.Get (), 0, 0, 1, &srcData);
    const auto transition = CD3DX12_RESOURCE_BARRIER::Transition (image_.Get (),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    uploadCommandList->ResourceBarrier (1, &transition);

    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    device_->CreateShaderResourceView (image_.Get (), &shaderResourceViewDesc,
        srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart ());
}

///////////////////////////////////////////////////////////////////////////////
void D3D12TexturedQuad::RenderImpl (ID3D12GraphicsCommandList * commandList)
{
    D3D12Sample::RenderImpl (commandList);

    UpdateConstantBuffer ();

    // Set the descriptor heap containing the texture srv
    ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap_.Get () };
    commandList->SetDescriptorHeaps (1, heaps);

    // Set slot 0 of our root signature to point to our descriptor heap with
    // the texture SRV
    commandList->SetGraphicsRootDescriptorTable (0,
        srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart ());

    // Set slot 1 of our root signature to the constant buffer view
    commandList->SetGraphicsRootConstantBufferView (1,
        constantBuffers_[GetQueueSlot ()]->GetGPUVirtualAddress ());

    commandList->IASetPrimitiveTopology (D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers (0, 1, &vertexBufferView_);
    commandList->IASetIndexBuffer (&indexBufferView_);
    commandList->DrawIndexedInstanced (6, 1, 0, 0, 0);

    // Imgui logic ---------------------
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
        // Main debug parameters window
        ImGui::Begin("ModelViewer - Parameters");

        ImGui::Separator();
        ImGui::Text("Image");
        ImGui::SliderFloat("Scale", &scale_, 0.0f, 1.0f);

        ImGui::Separator();
        ImGui::Text("Background");
        ImGui::ColorEdit3("clear color", clearColor_);
        ImGui::Text("Tint");
        ImGui::ColorEdit3("tint color", tintColor_);

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

        //TODO: Adding controls for camera & light positions+orientation

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
    descriptorHeapDesc.NumDescriptors = 1;
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
    struct Vertex
    {
        float position[3];
        float uv[2];
    };

    Assimp::Importer importer;
    std::string path = "data/models/";
    path.append(models_[modelIndex_]);
    path.append("/model.obj");
    const aiScene *scene = importer.ReadFile(path.c_str(),
                                             aiProcess_ConvertToLeftHanded |
                                             aiProcessPreset_TargetRealtime_MaxQuality);

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    std::vector<aiNode*> stack;
    stack.push_back(scene->mRootNode);
    while (stack.size() > 0) {
        aiNode *current = stack.back();
        stack.pop_back();

        for (unsigned int i = 0; i < current->mNumMeshes; i++) {
            // Extract all the indices
            aiMesh *m = scene->mMeshes[current->mMeshes[i]];

            for (unsigned int j = 0; j < m->mNumVertices; j++) {
                auto pos = m->mVertices[j];
                auto uv = m->mTextureCoords[0][j];

                vertices.push_back( {{ pos.x, pos.y, pos.z },{ uv.x, uv.y }} );
            }

            for (unsigned int j = 0; j < m->mNumFaces; j++) {
                aiFace f = m->mFaces[j];

                for (unsigned int k = 0; k < f.mNumIndices; k++) {
                    indices.push_back(f.mIndices[k]);
                }
            }
        }

        for (unsigned int i = 0; i < current->mNumChildren; i++) {
            // We want to search through all nodes
            stack.push_back(current->mChildren[i]);
        }
    }

    vertexCount_ = (UINT) vertices.size();
    indexCount_ = (UINT) indices.size();

    static const int uploadBufferSize = sizeof (vertices) + sizeof (indices);
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

    static const auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (sizeof (vertices));
    device_->CreateCommittedResource (&defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS (&vertexBuffer_));

    static const auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (sizeof (indices));
    device_->CreateCommittedResource (&defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &indexBufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS (&indexBuffer_));

    // Create buffer views
    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress ();
    vertexBufferView_.SizeInBytes = sizeof (vertices);
    vertexBufferView_.StrideInBytes = sizeof (Vertex);

    indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress ();
    indexBufferView_.SizeInBytes = sizeof (indices);
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    // Copy data on CPU into the upload buffer
    void* p;
    uploadBuffer_->Map (0, nullptr, &p);
    ::memcpy (p, vertices, sizeof (vertices));
    ::memcpy (static_cast<unsigned char*>(p) + sizeof (vertices),
        indices, sizeof (indices));
    uploadBuffer_->Unmap (0, nullptr);

    // Copy data from upload buffer on CPU into the index/vertex buffer on 
    // the GPU
    uploadCommandList->CopyBufferRegion (vertexBuffer_.Get (), 0,
        uploadBuffer_.Get (), 0, sizeof (vertices));
    uploadCommandList->CopyBufferRegion (indexBuffer_.Get (), 0,
        uploadBuffer_.Get (), sizeof (vertices), sizeof (indices));

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
    struct ConstantBuffer
    {
        float x, y, z, w;
    };

    static const ConstantBuffer cb = { 0, 0, 0, 0 };

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
    void* p;

    constantBuffers_[GetQueueSlot ()]->Map (0, nullptr, &p);
    float* f = static_cast<float*>(p);
    f[0] = scale_;
    f[1] = tintColor_[0];
    f[2] = tintColor_[1];
    f[3] = tintColor_[2];
    constantBuffers_[GetQueueSlot ()]->Unmap (0, nullptr);
}

///////////////////////////////////////////////////////////////////////////////
void D3D12TexturedQuad::CreateRootSignature ()
{
    // We have two root parameters, one is a pointer to a descriptor heap
    // with a SRV, the second is a constant buffer view
    CD3DX12_ROOT_PARAMETER parameters[2];

    // Create a descriptor table with one entry in our descriptor heap
    CD3DX12_DESCRIPTOR_RANGE range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };
    parameters[0].InitAsDescriptorTable (1, &range);

    // Our constant buffer view
    parameters[1].InitAsConstantBufferView (0);

    // We don't use another descriptor heap for the sampler, instead we use a
    // static sampler
    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init (0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

    CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;

    // Create the root signature
    descRootSignature.Init (2, parameters,
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
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    static const D3D_SHADER_MACRO macros[] = {
        { "D3D12_SAMPLE_TEXTURE", "1" },
        { nullptr, nullptr }
    };

    ComPtr<ID3DBlob> vertexShader;
    D3DCompile (code.data(), code.size(),
        "", macros, nullptr,
        "VS_main", "vs_5_0", 0, 0, &vertexShader, nullptr);

    ComPtr<ID3DBlob> pixelShader;
    D3DCompile (code.data(), code.size(),
        "", macros, nullptr,
        "PS_main", "ps_5_0", 0, 0, &pixelShader, nullptr);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.VS.BytecodeLength = vertexShader->GetBufferSize ();
    psoDesc.VS.pShaderBytecode = vertexShader->GetBufferPointer ();
    psoDesc.PS.BytecodeLength = pixelShader->GetBufferSize ();
    psoDesc.PS.pShaderBytecode = pixelShader->GetBufferPointer ();
    psoDesc.pRootSignature = rootSignature_.Get ();
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
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
    psoDesc.DepthStencilState.DepthEnable = false;
    psoDesc.DepthStencilState.StencilEnable = false;
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
