#include "UIRenderPass.h"
#include <d3dx12.h>

using namespace Microsoft::WRL;

namespace AMD {
UIRenderPass::UIRenderPass() { }

UIRenderPass::~UIRenderPass() { }

void UIRenderPass::Prepare(ComPtr<ID3D12Device> &device) {
}
void UIRenderPass::Execute() {
}
}
