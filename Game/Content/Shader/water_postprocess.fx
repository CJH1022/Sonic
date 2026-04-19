#ifndef _WATER_POSTPROCESS
#define _WATER_POSTPROCESS

#include "value.fx"

#define TintColor   g_vec4_0
#define WaveParams  g_vec4_1

struct VS_IN
{
    float3 vPos : POSITION;
    float2 vUV : TEXCOORD;
    float4 vColor : COLOR;
};

struct VS_OUT
{
    float4 vPosition : SV_Position;
    float2 vUV : TEXCOORD;
};

VS_OUT VS_WaterPostProcess(VS_IN _input)
{
    VS_OUT output = (VS_OUT)0.f;

    float4 vWorld = mul(float4(_input.vPos, 1.f), g_matWorld);
    float4 vView = mul(vWorld, g_matView);
    output.vPosition = mul(vView, g_matProj);
    output.vUV = _input.vUV;
    return output;
}

float4 PS_WaterPostProcess(VS_OUT _input) : SV_Target
{
    float2 uv = _input.vUV;
    float waveTime = Time * WaveParams.x;
    float waveTiling = max(0.1f, WaveParams.y);
    float shimmer = max(0.0f, WaveParams.z);
    float alpha = saturate(WaveParams.w);

    float noiseSample = 0.5f;
    if (g_btex_0)
    {
        float2 noiseUV = uv * waveTiling + float2(waveTime, waveTime * 0.37f);
        noiseSample = g_tex_0.Sample(g_sam_0, noiseUV).r;
    }

    float ripple = sin((uv.x + uv.y + waveTime) * 6.28318f * waveTiling) * 0.5f + 0.5f;
    float shimmerMask = saturate(noiseSample * 0.7f + ripple * 0.3f);

    float4 color = TintColor;
    color.rgb *= (0.75f + shimmerMask * 0.35f * shimmer);
    color.a *= alpha * (0.7f + shimmerMask * 0.3f);

    return color;
}

#endif
