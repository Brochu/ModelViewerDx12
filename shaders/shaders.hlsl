#ifdef D3D12_SAMPLE_TEXTURE
cbuffer PerFrameConstants : register (b0)
{
    float4 scale;
}

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

VertexShaderOutput VS_main(
    float4 position : POSITION,
    float2 uv : TEXCOORD)
{
    VertexShaderOutput output;

    output.position = position;
    output.position.xy *= scale.x;
    output.uv = uv;

    return output;
}

Texture2D<float4> anteruTexture : register(t0);
SamplerState texureSampler      : register(s0);

float4 PS_main (float4 position : SV_POSITION,
                float2 uv : TEXCOORD) : SV_TARGET
{
    float4 tint = float4(scale.y, scale.z, scale.w, 1.0);
    return tint * anteruTexture.Sample (texureSampler, uv);
}
#endif
