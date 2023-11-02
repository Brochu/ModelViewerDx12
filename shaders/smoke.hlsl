cbuffer SmokeConstants : register (b0)
{
    float4 bgColor;
    float4 values; // (sigma_a, dist_mult, reserved, reserved)
}

SamplerState texureSampler : register(s0);

void VS_main(in uint VertID : SV_VertexID, out float4 Pos : SV_Position, out float2 Tex : TexCoord0) {
    // Texture coordinates range [0, 2], but only [0, 1] appears on screen.
    Tex = float2(uint2(VertID, VertID << 1) & 2);
    Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0, 1);
}

float4 PS_main(float4 pos : SV_Position, float2 tex : TEXCOORD0) : SV_TARGET {
    float cx = tex.x - 0.5;
    float cy = tex.y - 0.5;
    float check = sqrt((cx * cx) + (cy * cy));

    if (check < 0.5) {
        float diff = (0.5 - check) * 2;
        float T = exp((-diff * values.y * 2.0) * values.x);

        float4 volColor = float4(0.8, 0.1, 0.5, 1.0);
        return (T * bgColor) + ((1 - T) * volColor);
    }

    return bgColor;
}
