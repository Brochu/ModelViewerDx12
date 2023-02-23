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

#ifndef ANTERU_D3D12_SAMPLE_D3D12SAMPLE_H_
#define ANTERU_D3D12_SAMPLE_D3D12SAMPLE_H_
#include "AiWrapper.h"
#include "Config.h"

#include <d3d12.h>
#include <dxgi.h>
#include <wrl.h>

namespace AMD {

///////////////////////////////////////////////////////////////////////////////
class D3D12Sample
{
public:
    D3D12Sample (const D3D12Sample&) = delete;
    D3D12Sample& operator= (const D3D12Sample&) = delete;

    D3D12Sample ();
    D3D12Sample (int modelOverride);
    virtual ~D3D12Sample ();

    void Run (int w, int h, HWND hwnd);
    void Step ();
    void Stop ();

private:
    void Initialize ();
    void Shutdown ();

    //TODO: Check if this is still needed
    void PrepareRender ();
    void FinalizeRender ();

    void Render ();
    void Present ();

    void CreateDeviceAndSwapChain ();
    void CreateAllocatorsAndCommandLists ();
    void CreateViewportScissor ();
    void CreatePipelineStateObject ();
    void SetupSwapChain ();
    void SetupRenderTargets ();

    //TODO: Move these to a separate helper namespace
    void CreateTexture (ID3D12GraphicsCommandList* uploadCommandList);
    void CreateMeshBuffers (ID3D12GraphicsCommandList* uploadCommandList);

    void CreateConstantBuffer ();
    void UpdateConstantBuffer ();
    void CreateRootSignature ();

    static const int QUEUE_SLOT_COUNT = 3;
    int currentBackBuffer_ = 0;

    HWND hwnd_;
    int width_ = -1;
    int height_ = -1;
    int modelIndex_ = 0;
    Config config_;

    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets_ [QUEUE_SLOT_COUNT];
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencil_ [QUEUE_SLOT_COUNT];
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
    D3D12_VIEWPORT viewport_;
    D3D12_RECT rectScissor_;

    HANDLE frameFenceEvents_ [QUEUE_SLOT_COUNT];
    Microsoft::WRL::ComPtr<ID3D12Fence> frameFences_ [QUEUE_SLOT_COUNT];
    UINT64 currentFenceValue_;
    UINT64 fenceValues_[QUEUE_SLOT_COUNT];

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> renderTargetDescriptorHeap_;
    std::int32_t renderTargetViewDescriptorSize_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> depthStencilDescriptorHeap_;
    std::int32_t depthStencilViewDescriptorSize_;

    //TODO: Find a way to split this into a pass structure?
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators_[QUEUE_SLOT_COUNT];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandLists_[QUEUE_SLOT_COUNT];
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffers_[QUEUE_SLOT_COUNT];

    //TODO: Try to move these to an upload pass namespace
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_;

    //TODO: Try to move these to an upload pass namespace
    std::vector<Material> materials_;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadImage_;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> image_;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> imguiDescriptorHeap_;

    Draws draws_;

    // Paths
    const char *shaderFile_ = "shaders/shaders.hlsl";
};
}

#endif
