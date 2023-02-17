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

#ifndef AMD_TEXTURED_QUAD_D3D12_SAMPLE_H_
#define AMD_TEXTURED_QUAD_D3D12_SAMPLE_H_

#include "D3D12Sample.h"
#include "AiWrapper.h"

#include "string"
#include "vector"

namespace AMD {

typedef Microsoft::WRL::ComPtr<ID3D12Resource> ResPtr;
typedef Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> HeapPtr;

class D3D12TexturedQuad : public D3D12Sample
{
public:
    D3D12TexturedQuad();
    D3D12TexturedQuad(int modelOverride);

private:
    void CreateTexture (ID3D12GraphicsCommandList* uploadCommandList);
    void CreateMeshBuffers (ID3D12GraphicsCommandList* uploadCommandList);
    void CreateConstantBuffer ();
    void UpdateConstantBuffer ();
    void CreateRootSignature ();
    void CreatePipelineStateObject ();
    void LoadConfig ();

    void RenderImpl (ID3D12GraphicsCommandList* commandList) override;
    void InitializeImpl (ID3D12GraphicsCommandList* uploadCommandList) override;

    ResPtr uploadBuffer_;

    ResPtr vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;

    ResPtr indexBuffer_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_;

    std::vector<Material> materials_;
    std::vector<ResPtr> image_;
    std::vector<ResPtr> uploadImage_;

    ResPtr constantBuffers_[QUEUE_SLOT_COUNT];

    HeapPtr srvDescriptorHeap_;
    HeapPtr imguiDescriptorHeap_;

    std::vector<std::string> models_;
    int modelIndex_ = 0;
    Draws draws_;

    // Debug parameters
    bool showWindow_ = true;
    // Transforms
    float translate_[3] { 0.0, 0.0, 0.0 };
    float rotate_[3] { 0.0, 0.0, 0.0 };
    float scale_[3] { 1.0, 1.0, 1.0 };
    // Camera
    float camPos_[3] { 0.0, 0.0, -10.0 };
    float lookAt_[3] { 0.0, 0.0, 0.0 };
    float fov_ = 45.f;
    // Light
    float lightPos_[3] { 1.0, 1.0, 0.0 };

    // Paths
    const char *shaderFile_ = "shaders/shaders.hlsl";
    const char *configFile_ = "config.ini";
};
}

#endif
