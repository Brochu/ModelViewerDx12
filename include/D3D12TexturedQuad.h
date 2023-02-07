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

#include <vector>

namespace AMD {
class D3D12TexturedQuad : public D3D12Sample
{
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

    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer_;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;

    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_;

    Microsoft::WRL::ComPtr<ID3D12Resource>	image_;
    Microsoft::WRL::ComPtr<ID3D12Resource>	uploadImage_;
    std::vector<std::uint8_t>			imageData_;

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffers_[QUEUE_SLOT_COUNT];

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    srvDescriptorHeap_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    imguiDescriptorHeap_;

    std::vector<std::string> models_;
    int modelIndex_ = 0;

    unsigned int vertexCount_;
    unsigned int indexCount_;

    // Debug parameters
    bool showWindow_ = true;
    float scale_ = 0.5f;
    float tintColor_[4] = { 1.0, 1.0, 1.0, 1.0 };

    // Paths
    const char *shaderFile_ = "shaders/shaders.hlsl";
    const char *configFile_ = "config.ini";
};
}

#endif
