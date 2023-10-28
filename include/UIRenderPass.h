#pragma once
#include "Config.h"

#include <d3d12.h>
#include <wrl.h>

namespace AMD {
struct UIRenderPass {
    UIRenderPass();
    ~UIRenderPass();

    void Prepare(Microsoft::WRL::ComPtr<ID3D12Device> &device);
    void Execute(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &uiCmdList,
                           Config &config,
                           int &groupIndex, int &modelIndex, bool &swappedModel,
                           float *clearColor,
                           bool *skipModel, bool *skipSmoke,
                           float *translate, float *rotate, float *scale,
                           float *camPos, float *lookAt, float *fov,
                           float *lightPos, float *lightPower);

private:
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> imguiDescriptorHeap_;

    static const int QUEUE_SLOT_COUNT = 3;
};
}
