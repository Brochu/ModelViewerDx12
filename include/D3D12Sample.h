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

#include <d3d12.h>
#include <dxgi.h>
#include <wrl.h>

namespace AMD {
typedef Microsoft::WRL::ComPtr<ID3D12Resource> ResPtr;
typedef Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> HeapPtr;

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

protected:
    int GetQueueSlot () const
    {
        return currentBackBuffer_;
    }

    static const int QUEUE_SLOT_COUNT = 3;

    static constexpr int GetQueueSlotCount ()
    {
        return QUEUE_SLOT_COUNT;
    }

    D3D12_VIEWPORT viewport_;
    D3D12_RECT rectScissor_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets_ [QUEUE_SLOT_COUNT];
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencil_ [QUEUE_SLOT_COUNT];
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;

    HANDLE frameFenceEvents_ [QUEUE_SLOT_COUNT];
    Microsoft::WRL::ComPtr<ID3D12Fence> frameFences_ [QUEUE_SLOT_COUNT];
    UINT64 currentFenceValue_;
    UINT64 fenceValues_[QUEUE_SLOT_COUNT];

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> renderTargetDescriptorHeap_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> depthStencilDescriptorHeap_;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;

    float clearColor_[4] = { 0.042f, 0.042f, 0.042f, 1.0f };

    int width_ = -1;
    int height_ = -1;

    int modelIndex_ = 0;

    virtual void InitializeImpl (ID3D12GraphicsCommandList* uploadCommandList);
    virtual void RenderImpl (ID3D12GraphicsCommandList* commandList);

private:
    void Initialize ();
    void Shutdown ();

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

    void CreateTexture (ID3D12GraphicsCommandList* uploadCommandList);
    void CreateMeshBuffers (ID3D12GraphicsCommandList* uploadCommandList);
    void CreateConstantBuffer ();
    void UpdateConstantBuffer ();
    void CreateRootSignature ();
    void LoadConfig ();

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators_[QUEUE_SLOT_COUNT];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandLists_[QUEUE_SLOT_COUNT];

    int currentBackBuffer_ = 0;
    HWND hwnd_;
    
    std::int32_t renderTargetViewDescriptorSize_;
    std::int32_t depthStencilViewDescriptorSize_;

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
