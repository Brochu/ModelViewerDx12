#include <d3d12.h>
#include <wrl.h>

namespace AMD {
struct ModelRenderPass {
    ModelRenderPass(Microsoft::WRL::ComPtr<ID3D12Device> &device);
    ~ModelRenderPass();

private:
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
};
}
