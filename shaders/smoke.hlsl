cbuffer SmokeConstants : register (b0)
{
    float4 bgColor;
    float4 smokePos;
    float4x4 mvp;
    float4 verts[36];
    float4 values; // (sigma_a, dist_mult, unused, unused)
}

SamplerState texureSampler : register(s0);

static float2 UVs[6] = {
    float2(0, 0),
    float2(1, 0),
    float2(1, 1),
    float2(1, 1),
    float2(0, 1),
    float2(0, 0),
};

void VS_main(in uint VertID : SV_VertexID, out float4 Pos : SV_Position, out float2 Tex : TexCoord0) {
    Pos = mul(mvp, verts[VertID]);
    Tex = UVs[VertID % 6];
}

float4 PS_main(float4 pos : SV_Position, float2 tex : TEXCOORD0) : SV_TARGET {
    float cx = tex.x - 0.5;
    float cy = tex.y - 0.5;
    float check = sqrt((cx * cx) + (cy * cy));

    if (check < 0.5) {
        float diff = (0.5 - check) * 2;
        float T = exp((-diff * values.y * 2.0) * values.x);

        float4 volColor = float4(0.8, 0.1, 0.5, 0.9);
        return (T * bgColor) + ((1 - T) * volColor);
    }

    return bgColor;
}
