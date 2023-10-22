#pragma once
#include <d3d12.h>
#include <wrl.h>

namespace AMD {
struct UIRenderPass {
    UIRenderPass();
    ~UIRenderPass();

    void Prepare(Microsoft::WRL::ComPtr<ID3D12Device> &device);
    void Execute();
};
}
