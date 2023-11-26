#include "UIRenderPass.h"

#include <d3dx12.h>
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <time.h>

using namespace Microsoft::WRL;

namespace AMD {
const size_t MAX_GROUP_COUNT = 64;
const size_t MAX_MODEL_COUNT = 128;

UIRenderPass::UIRenderPass() { }

UIRenderPass::~UIRenderPass() { }

void UIRenderPass::Prepare(ComPtr<ID3D12Device> &device) {
    device_ = device;

    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = 1;
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descriptorHeapDesc.NodeMask = 0;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device_->CreateDescriptorHeap (&descriptorHeapDesc, IID_PPV_ARGS (&imguiDescriptorHeap_));

    // Imgui render side init
    ImGui_ImplDX12_Init(device_.Get(),
                        QUEUE_SLOT_COUNT,
                        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                        imguiDescriptorHeap_.Get(),
                        imguiDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
                        imguiDescriptorHeap_->GetGPUDescriptorHandleForHeapStart());
}
void UIRenderPass::Execute(ComPtr<ID3D12GraphicsCommandList> &uiCmdList,
                           Config &config,
                           int &groupIndex, int &modelIndex, bool &swappedModel,
                           float *clearColor,
                           bool *skipModel, bool *skipSmoke,
                           float *translate, float *rotate, float *scale,
                           float *camPos, float *lookAt, float *fov,
                           float *lightPos, float *lightPower,
                           float *sigmaA, float *distMult, float *smokePos, float *smokeSize) {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("ModelViewer - Parameters");

        ImGui::Separator();
        ImGui::Text("Seconds elapsed: %f", (double)clock() / CLOCKS_PER_SEC);

        ImGui::Separator();
        ImGui::Text("Passes Override");
        ImGui::Checkbox("Skip Model Pass", skipModel);
        ImGui::Checkbox("Skip Smoke Pass", skipSmoke);

        ImGui::Separator();
        ImGui::Text("Background");
        ImGui::ColorEdit3("clear color", clearColor);

        ImGui::Separator();
        ImGui::Text("Model Viewer");

        std::vector<GroupEntry> g = config.groups;
        assert(g.size() <= MAX_GROUP_COUNT);
        char* groups[MAX_GROUP_COUNT];
        for (int i = 0; i < g.size(); i++) {
            groups[i] = const_cast<char*>(g[i].name.data());
        }
        if (ImGui::Combo("Game", &groupIndex, groups, (int)g.size())) {
            modelIndex = 0;
            swappedModel = true;
        }

        std::vector<ModelEntry> m = config.models;
        assert(m.size() <= MAX_MODEL_COUNT);
        char* models[MAX_MODEL_COUNT];
        for (int i = 0; i < g[groupIndex].modelrefs.size(); i++) {
            const size_t modelIdx = g[groupIndex].modelrefs[i];
            models[i] = const_cast<char*>(m[modelIdx].files[0].folder.data());
        }
        if (ImGui::Combo("Model", &modelIndex, models, (int)g[groupIndex].modelrefs.size())) {
            swappedModel = true;
        }

        ImGui::Separator();
        ImGui::Text("Transform");
        ImGui::DragFloat3("translate", translate, 0.1f, -100.f, 100.f);
        ImGui::DragFloat3("rotate", rotate, 1.f, -359.f, 359.f);
        ImGui::DragFloat3("scale", scale, 0.01f, -10.f, 10.f);

        ImGui::Separator();
        ImGui::Text("Camera");
        ImGui::DragFloat3("cam pos", camPos, 1.f, -500.f, 500.f);
        ImGui::DragFloat3("focus", lookAt, 1.f, -500.f, 500.f);
        ImGui::DragFloat("FOV", fov, 0.25f, 5.f, 110.f);

        ImGui::Separator();
        ImGui::Text("Light");
        ImGui::DragFloat3("lit pos", lightPos, 1.f, -500.f, 500.f);
        ImGui::DragFloat("power", lightPower, 0.05f, 0.05f, 5.f);

        ImGui::Separator();
        ImGui::Text("Smoke / Cloud");
        ImGui::DragFloat3("smoke pos", smokePos, 1.f, -500.f, 500.f);
        ImGui::DragFloat("smoke size", smokeSize, 1.f, 0.f, 100.f);
        ImGui::DragFloat("sigma_a", sigmaA, 0.01f, 0.f, 1.f);
        ImGui::DragFloat("dist_mult", distMult, 0.01f, 0.01f, 10.f);

        ImGui::Separator();
        ImGui::End();
    }
    ImGui::Render();

    ID3D12DescriptorHeap* imguiHeaps[] = { imguiDescriptorHeap_.Get() };
    uiCmdList->SetDescriptorHeaps(1, imguiHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), uiCmdList.Get());
}
}
