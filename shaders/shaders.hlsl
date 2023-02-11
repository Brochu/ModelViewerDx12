cbuffer PerFrameConstants : register (b0)
{
    float4x4 mvp;
    float4x4 world;
    float4 lightPos;
}

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float4 worldpos : POSITION;
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
    output.worldpos = mul(world, position);
    output.normal = normal;
    output.uv = uv;

    return output;
}

Texture2D<float4> anteruTexture : register(t0);
SamplerState texureSampler      : register(s0);

float4 PS_main (VertexShaderOutput input) : SV_TARGET
{
    input.normal = normalize(input.normal);

    float3 ambient = float3(0.02, 0.02, 0.02);
    float3 color = float3(0.3, 0.3, 0.3);
    float3 final = float3(0.0, 0.0, 0.0);

    float3 light2pix = lightPos.xyz - input.worldpos.xyz;
    float dist = length(light2pix);

    light2pix /= dist;
    float3 att = float3(0.0, 0.01, 0.0);
    float theta = dot(light2pix, input.normal);
    if (theta > 0.0) {
        final += theta * color;
        final /= att.x + (att.y * dist) + (att.z * (dist * dist));
    }

    final = saturate(final + ambient);
    return float4(final, 1.0);
}
