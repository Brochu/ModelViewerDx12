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

#define NOMINMAX
#include "D3D12Sample.h"
#include "ImageIO.h"
#include "Utility.h"

#include "Tracy.hpp"
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

#include <d3dx12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <fstream>

using namespace Microsoft::WRL;

namespace AMD {

namespace {
struct RenderEnvironment
{
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain> swapChain;
};

///////////////////////////////////////////////////////////////////////////////
/**
Create everything we need for rendering, this includes a device, a command queue,
and a swap chain.
*/
RenderEnvironment CreateDeviceAndSwapChainHelper (
    _In_opt_ IDXGIAdapter* adapter,
    D3D_FEATURE_LEVEL minimumFeatureLevel,
    _In_ const DXGI_SWAP_CHAIN_DESC* swapChainDesc)
{
    RenderEnvironment result;

    auto hr = D3D12CreateDevice (adapter, minimumFeatureLevel,
        IID_PPV_ARGS (&result.device));

    if (FAILED (hr)) {
        throw std::runtime_error ("Device creation failed.");
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    hr = result.device->CreateCommandQueue (&queueDesc, IID_PPV_ARGS (&result.queue));

    if (FAILED (hr)) {
        throw std::runtime_error ("Command queue creation failed.");
    }

    ComPtr<IDXGIFactory4> dxgiFactory;
    hr = CreateDXGIFactory2 (DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS (&dxgiFactory));

    if (FAILED (hr)) {
        throw std::runtime_error ("DXGI factory creation failed.");
    }

    // Must copy into non-const space
    DXGI_SWAP_CHAIN_DESC swapChainDescCopy = *swapChainDesc;
    hr = dxgiFactory->CreateSwapChain (
        result.queue.Get (),
        &swapChainDescCopy,
        &result.swapChain
        );

    if (FAILED (hr)) {
        throw std::runtime_error ("Swap chain creation failed.");
    }

    return result;
}
}

struct ConstantBuffer {
    DirectX::XMMATRIX mvp;
    DirectX::XMMATRIX world;
    DirectX::XMVECTOR lightPos;
};

struct DebugParams {
    bool showWindow = true;
    float clearColor[4] = { 0.042f, 0.042f, 0.042f, 1.0f };

    // Skip Passes
    bool skipModels = true;
    bool skipSmoke = false;

    // Transforms
    float translate[3] { 0.0, 0.0, 0.0 };
    float rotate[3] { 0.0, 0.0, 0.0 };
    float scale[3] { 1.0, 1.0, 1.0 };

    // Camera
    float camPos[3] { 0.0, 0.0, -10.0 };
    float lookAt[3] { 0.0, 0.0, 0.0 };
    float fov = 45.f;

    // Light
    float lightPos[3] { 1.0, 1.0, 0.0 };
    float lightPower = 1.f;

    // Smoke / Cloud
    float smokePos[3] { 0.0, 0.0, 0.0 };
    float sigmaA = 1.f;
    float distMult = 1.f;
};
DebugParams dparams = {};

namespace {
void WaitForFence (ID3D12Fence* fence, UINT64 completionValue, HANDLE waitEvent)
{
    if (fence->GetCompletedValue () < completionValue) {
        fence->SetEventOnCompletion (completionValue, waitEvent);
        WaitForSingleObject (waitEvent, INFINITE);
    }
}
}

///////////////////////////////////////////////////////////////////////////////
D3D12Sample::D3D12Sample ()
{
    modelIndex_ = 0;
}
D3D12Sample::D3D12Sample (int modelOverride)
{
    modelIndex_ = modelOverride;
}

///////////////////////////////////////////////////////////////////////////////
D3D12Sample::~D3D12Sample ()
{
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::PrepareRender ()
{
    ZoneScopedN("Sample::PrepareRender");
    commandAllocators_ [currentBackBuffer_]->Reset ();

    if (swappedModel_) {
        // Drain the queue, wait for everything to finish
        for (int i = 0; i < QUEUE_SLOT_COUNT; ++i) {
            WaitForFence (frameFences_[i].Get (), fenceValues_[i], frameFenceEvents_[i]);
        }

        UploadPass upload(device_);
        LoadContent(upload);

        upload.Execute(commandQueue_.Get());
        upload.WaitForUpload();

        swappedModel_ = false;
    }

    auto commandList = commandLists_ [currentBackBuffer_].Get ();
    commandList->Reset (
        commandAllocators_ [currentBackBuffer_].Get (), nullptr);

    D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted (renderTargetHandle,
        renderTargetDescriptorHeap_->GetCPUDescriptorHandleForHeapStart (),
        currentBackBuffer_, renderTargetViewDescriptorSize_);

    D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(depthStencilHandle,
        depthStencilDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
        currentBackBuffer_, depthStencilViewDescriptorSize_);

    commandList->OMSetRenderTargets (1, &renderTargetHandle, true, &depthStencilHandle);
    commandList->RSSetViewports (1, &viewport_);
    commandList->RSSetScissorRects (1, &rectScissor_);

    // Transition back buffer
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Transition.pResource = renderTargets_ [currentBackBuffer_].Get ();
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList->ResourceBarrier (1, &barrier);

    commandList->ClearRenderTargetView (renderTargetHandle,
        dparams.clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(depthStencilHandle, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::Render ()
{
    ZoneScopedN("Sample::Render");
    PrepareRender();
    
    UpdateConstantBuffer ();
    auto commandList = commandLists_ [currentBackBuffer_];

    if (!dparams.skipModels) {
        renderpass_.Execute(commandList,
                            draws_,
                            vertexBufferView_,
                            indexBufferView_,
                            constantBuffers_[currentBackBuffer_],
                            srvDescriptorHeap_);
    }

    if (!dparams.skipSmoke) {
        smokepass_.Execute(commandList, currentBackBuffer_);
    }

    uipass_.Execute(commandList,
                    config_,
                    groupIndex_, modelIndex_, swappedModel_,
                    dparams.clearColor,
                    &dparams.skipModels, &dparams.skipSmoke,
                    dparams.translate, dparams.rotate, dparams.scale,
                    dparams.camPos, dparams.lookAt, &dparams.fov,
                    dparams.lightPos, &dparams.lightPower,
                    &dparams.sigmaA, &dparams.distMult);

    FinalizeRender ();
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::FinalizeRender ()
{
    ZoneScopedN("Sample::FinalizeRender");

    // Transition the swap chain back to present
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Transition.pResource = renderTargets_ [currentBackBuffer_].Get ();
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    auto commandList = commandLists_ [currentBackBuffer_].Get ();
    commandList->ResourceBarrier (1, &barrier);

    commandList->Close ();

    // Execute our commands
    ID3D12CommandList* commandLists [] = { commandList };
    commandQueue_->ExecuteCommandLists (std::extent<decltype(commandLists)>::value, commandLists);
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::Run (int w, int h, HWND hwnd)
{
    width_ = w;
    height_ = h;
    hwnd_ = hwnd;

    Initialize ();
}

void D3D12Sample::Step ()
{
    FrameMarkNamed("Frame");
    ZoneScopedN("Sample::Step");

    WaitForFence (frameFences_[currentBackBuffer_].Get (), 
        fenceValues_[currentBackBuffer_], frameFenceEvents_[currentBackBuffer_]);
    
    Render ();
    Present ();

    //TODO: This makes the last frame captured last some crazy amount of time
    // Need to look into how to capture GPU zones with Tracy
    //TracyD3D12Collect(tracyCtx_);
}

void D3D12Sample::Stop ()
{
    // Drain the queue, wait for everything to finish
    for (int i = 0; i < QUEUE_SLOT_COUNT; ++i) {
        WaitForFence (frameFences_[i].Get (), fenceValues_[i], frameFenceEvents_[i]);
    }

    Shutdown ();
}

///////////////////////////////////////////////////////////////////////////////
/**
Setup all render targets. This creates the render target descriptor heap and
render target views for all render targets.
This function does not use a default view but instead changes the format to
_SRGB.
*/
void D3D12Sample::SetupRenderTargets ()
{
    //TODO: Need to setup new render target views for extra RTs
    // Goal is to render the scene first in the extra RTs
    // Send this scene render to the smoke pass
    // Render the smoke pass in the present RTs
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = QUEUE_SLOT_COUNT;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    device_->CreateDescriptorHeap (&heapDesc, IID_PPV_ARGS (&renderTargetDescriptorHeap_));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{ 
        renderTargetDescriptorHeap_->GetCPUDescriptorHandleForHeapStart ()
    };

    for (int i = 0; i < QUEUE_SLOT_COUNT; ++i) {
        D3D12_RENDER_TARGET_VIEW_DESC viewDesc;
        viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MipSlice = 0;
        viewDesc.Texture2D.PlaneSlice = 0;

        device_->CreateRenderTargetView (renderTargets_ [i].Get (), &viewDesc,
            rtvHandle);

        rtvHandle.Offset (renderTargetViewDescriptorSize_);
    }

    // Depth Stencil Views
    D3D12_DESCRIPTOR_HEAP_DESC depthHeapDesc = {};
    depthHeapDesc.NumDescriptors = QUEUE_SLOT_COUNT;
    depthHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    depthHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    device_->CreateDescriptorHeap(&depthHeapDesc, IID_PPV_ARGS(&depthStencilDescriptorHeap_));

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle {
        depthStencilDescriptorHeap_->GetCPUDescriptorHandleForHeapStart()
    };

    for (int i = 0; i < QUEUE_SLOT_COUNT; i++) {
        D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc {};
        viewDesc.Format = DXGI_FORMAT_D32_FLOAT;
        viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MipSlice = 0;
        
        device_->CreateDepthStencilView(depthStencil_[i].Get(), &viewDesc, dsvHandle);
        dsvHandle.Offset (depthStencilViewDescriptorSize_);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
Present the current frame by swapping the back buffer, then move to the
next back buffer and also signal the fence for the current queue slot entry.
*/
void D3D12Sample::Present ()
{
    ZoneScopedN("Sample::Present");
    swapChain_->Present (1, 0);

    // Mark the fence for the current frame.
    const auto fenceValue = currentFenceValue_;
    commandQueue_->Signal (frameFences_ [currentBackBuffer_].Get (), fenceValue);
    fenceValues_[currentBackBuffer_] = fenceValue;
    ++currentFenceValue_;

    // Take the next back buffer from our chain
    currentBackBuffer_ = (currentBackBuffer_ + 1) % QUEUE_SLOT_COUNT;
}

///////////////////////////////////////////////////////////////////////////////
/**
Set up swap chain related resources, that is, the render target view, the
fences, and the descriptor heap for the render target.
*/
void D3D12Sample::SetupSwapChain ()
{
    currentFenceValue_ = 1;

    // Create fences for each frame so we can protect resources and wait for
    // any given frame
    for (int i = 0; i < QUEUE_SLOT_COUNT; ++i) {
        frameFenceEvents_ [i] = CreateEvent (nullptr, FALSE, FALSE, nullptr);
        fenceValues_ [i] = 0;
        device_->CreateFence (0, D3D12_FENCE_FLAG_NONE, 
            IID_PPV_ARGS (&frameFences_ [i]));
    }

    for (int i = 0; i < QUEUE_SLOT_COUNT; ++i) {
        swapChain_->GetBuffer (i, IID_PPV_ARGS (&renderTargets_ [i]));
    }

    auto depthProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT,
            width_, height_,
            1, 0, 1, 0,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
        );

    D3D12_CLEAR_VALUE depthClearValue = {};
    depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthClearValue.DepthStencil.Depth = 0.f;
    depthClearValue.DepthStencil.Stencil = 0;

    for (int i = 0; i < QUEUE_SLOT_COUNT; i++) {
        HRESULT hr = device_->CreateCommittedResource(
                &depthProp,
                D3D12_HEAP_FLAG_NONE,
                &depthDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &depthClearValue,
                __uuidof(ID3D12Resource),
                &depthStencil_[i]
            );
    }

    SetupRenderTargets ();
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::Initialize ()
{
    config_ = ParseConfig();

    CreateDeviceAndSwapChain ();
    CreateAllocatorsAndCommandLists ();
    CreateViewportScissor ();

    // We need one descriptor heap to store our texture SRVs which cannot go
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = 256;
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descriptorHeapDesc.NodeMask = 0;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device_->CreateDescriptorHeap (&descriptorHeapDesc, IID_PPV_ARGS (&srvDescriptorHeap_));

    UploadPass upload(device_);
    LoadContent(upload);

    upload.Execute(commandQueue_.Get());
    upload.WaitForUpload();

    renderpass_.Prepare(device_);
    smokepass_.Prepare(device_);
    uipass_.Prepare(device_);
    CreateConstantBuffer ();
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::Shutdown ()
{
    for (auto event : frameFenceEvents_) {
        CloseHandle (event);
    }
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::CreateDeviceAndSwapChain ()
{
    // Enable the debug layers when in debug mode
    // If this fails, install the Graphics Tools for Windows. On Windows 10,
    // open settings, Apps, Apps & Features, Optional features, Add Feature,
    // and add the graphics tools
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debugController;
    D3D12GetDebugInterface (IID_PPV_ARGS (&debugController));
    debugController->EnableDebugLayer ();
#endif

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ::ZeroMemory (&swapChainDesc, sizeof (swapChainDesc));

    swapChainDesc.BufferCount = QUEUE_SLOT_COUNT;
    // This is _UNORM but we'll use a _SRGB view on this. See 
    // SetupRenderTargets() for details, it must match what
    // we specify here
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferDesc.Width = width_;
    swapChainDesc.BufferDesc.Height = height_;
    swapChainDesc.OutputWindow = hwnd_;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Windowed = true;

    auto renderEnv = CreateDeviceAndSwapChainHelper (nullptr, D3D_FEATURE_LEVEL_11_0,
        &swapChainDesc);

    device_ = renderEnv.device;
    commandQueue_ = renderEnv.queue;
    swapChain_ = renderEnv.swapChain;

    renderTargetViewDescriptorSize_ =
        device_->GetDescriptorHandleIncrementSize (D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    depthStencilViewDescriptorSize_ =
        device_->GetDescriptorHandleIncrementSize (D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    SetupSwapChain ();
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::CreateAllocatorsAndCommandLists ()
{
    for (int i = 0; i < QUEUE_SLOT_COUNT; ++i) {
        device_->CreateCommandAllocator (D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS (&commandAllocators_ [i]));
        device_->CreateCommandList (0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            commandAllocators_ [i].Get (), nullptr,
            IID_PPV_ARGS (&commandLists_ [i]));
        commandLists_ [i]->Close ();
    }
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::CreateViewportScissor ()
{
    rectScissor_ = { 0, 0, width_, height_ };

    viewport_ = { 0.0f, 0.0f,
        static_cast<float>(width_),
        static_cast<float>(height_),
        0.0f, 1.0f
    };
}

void D3D12Sample::LoadContent (UploadPass &upload) {
    const GroupEntry group = config_.groups[groupIndex_];
    const ModelEntry model = config_.models[group.modelrefs[modelIndex_]];

    draws_ = {};
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Material> materials;
    std::vector<Texture> textures;
    size_t matIdx = 0;

    for (const auto f : model.files) {
        const std::string model_folder = "data/models/" + f.folder + "/";
        const std::string model_path = model_folder + f.file;

        ExtractAiScene(model_path.c_str(), draws_, vertices, indices, materials, matIdx);

        for (; matIdx < materials.size(); matIdx++) {
            const std::string textureName = materials[matIdx].textureName;

            if (textureName.empty()) {
                //TODO: This should not be like this, WHY EMPTY TEXTURE NAMES?
                textures.push_back({ 0, 0, {} });
            } else {
                std::string tex_path = model_folder + textureName;

                int w, h = 0;
                const std::vector<std::uint8_t> imgdata = LoadImageFromFile(tex_path.c_str(), 1, &w, &h);
                textures.push_back({ w, h, imgdata });
            }
        }
    }
    upload.CreateMeshBuffers(vertices, indices, vertexBuffer_, vertexBufferView_, indexBuffer_, indexBufferView_);
    upload.UploadTextures(srvDescriptorHeap_, textures, image_);
}

void D3D12Sample::CreateConstantBuffer ()
{
    const ConstantBuffer cb = {
        DirectX::XMMatrixIdentity(),
        DirectX::XMMatrixIdentity(),
        DirectX::XMVectorSet(0.0 ,0.0, 0.0, 1.0)
    };

    for (int i = 0; i < QUEUE_SLOT_COUNT; ++i) {
        const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_UPLOAD);
        const auto constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (sizeof (ConstantBuffer));

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

void D3D12Sample::UpdateConstantBuffer ()
{
    using namespace DirectX;

    XMMATRIX model = DirectX::XMMatrixScaling(
            dparams.scale[0],
            dparams.scale[1],
            dparams.scale[2]
        );
    model = DirectX::XMMatrixMultiply(model, DirectX::XMMatrixRotationRollPitchYaw(
                                          DirectX::XMConvertToRadians(dparams.rotate[0]),
                                          DirectX::XMConvertToRadians(dparams.rotate[1]),
                                          DirectX::XMConvertToRadians(dparams.rotate[2])
                                      ));
    model = DirectX::XMMatrixMultiply(model, DirectX::XMMatrixTranslation(
                                          dparams.translate[0],
                                          dparams.translate[1],
                                          dparams.translate[2]
                                      ));

    XMVECTOR camPos = XMVectorSet(dparams.camPos[0], dparams.camPos[1], dparams.camPos[2], 1.0f);
    XMVECTOR lookAt = XMVectorSet(dparams.lookAt[0], dparams.lookAt[1], dparams.lookAt[2], 1.0f );
    XMVECTOR up = XMVectorSet(0.0, 1.0, 0.0, 0.0);
    XMMATRIX view = DirectX::XMMatrixLookAtLH(camPos, lookAt, up);

    // Projection
    float aspect = (float)width_ / (float)height_;
    XMMATRIX projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(dparams.fov), aspect, 100000.f, 0.1f);

    XMMATRIX mvp = XMMatrixMultiply(model, view);
    mvp = XMMatrixMultiply(mvp, projection);
    XMMATRIX world = XMMatrixTranspose(model); // Need to convert local space to world space

    ConstantBuffer cb {
        mvp,
        world,
        XMVectorSet(dparams.lightPos[0], dparams.lightPos[1], dparams.lightPos[2], dparams.lightPower)
    };

    void* p;
    HRESULT r = constantBuffers_[currentBackBuffer_]->Map (0, nullptr, &p);
    ::memcpy(p, &cb, sizeof(ConstantBuffer));
    constantBuffers_[currentBackBuffer_]->Unmap (0, nullptr);

    smokepass_.Update(currentBackBuffer_, dparams.smokePos, view, mvp, dparams.sigmaA, dparams.distMult);
}
}
