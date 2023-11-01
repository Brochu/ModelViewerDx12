cbuffer SmokeConstants : register (b0)
{
    float4 values;
}

SamplerState texureSampler : register(s0);

void VS_main(in uint VertID : SV_VertexID, out float4 Pos : SV_Position, out float2 Tex : TexCoord0) {
    // Texture coordinates range [0, 2], but only [0, 1] appears on screen.
    Tex = float2(uint2(VertID, VertID << 1) & 2);
    Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0, 1);
}

float4 PS_main(float4 pos : SV_Position, float2 tex : TEXCOORD0) : SV_TARGET {
    return float4(tex.r, tex.g, values.b, 1.0);
}
