#include "ModelRenderPass.h"

using namespace Microsoft::WRL;

namespace AMD {
ModelRenderPass::ModelRenderPass(ComPtr<ID3D12Device> &device) {
    device_ = device;
}

ModelRenderPass::~ModelRenderPass() {
}

void ModelRenderPass::Execute(ID3D12CommandQueue *queue) {
}

}
