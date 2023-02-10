cbuffer PerFrameConstants : register (b0)
{
    float4x4 mvp;
    float4 lightPos;
}

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};

VertexShaderOutput VS_main(
    float4 position : POSITION,
    float4 normal : NORMAL,
    float2 uv : TEXCOORD)
{
    VertexShaderOutput output;

    output.position = mul(mvp, position);
    output.normal = normal;
    output.uv = uv;

    return output;
}

Texture2D<float4> anteruTexture : register(t0);
SamplerState texureSampler      : register(s0);

float4 PS_main (float4 position : SV_POSITION,
                float4 normal: NORMAL,
                float2 uv : TEXCOORD) : SV_TARGET
{
    //return anteruTexture.Sample (texureSampler, uv);

    float factor = saturate(dot(normal.xyz, lightPos.xyz));
    factor += 0.1; //Abient

    return float4(factor, factor, factor, 1.0);
}
