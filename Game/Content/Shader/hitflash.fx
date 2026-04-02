#ifndef _HITFLASH
#define _HITFLASH

#include "value.fx"

#define FlashColor g_vec4_0

struct VS_IN
{
    float3 vPos : POSITION;
    float2 vUV : TEXCOORD;
};

struct VS_OUT
{
    float4 vPosition : SV_Position;
    float2 vUV : TEXCOORD;
};

VS_OUT VS_HitFlash(VS_IN _input)
{
    VS_OUT output = (VS_OUT)0.f;

    float4 vWorld = mul(float4(_input.vPos, 1.f), g_matWorld);
    float4 vView = mul(vWorld, g_matView);
    float4 vProj = mul(vView, g_matProj);

    output.vPosition = vProj;
    output.vUV = _input.vUV;

    return output;
}

float4 PS_HitFlash(VS_OUT _input) : SV_Target
{
    float2 centeredUV = (_input.vUV - float2(0.5f, 0.5f)) * 2.f;
    float radius = length(centeredUV);

    float alpha = saturate(1.f - radius);
    alpha *= alpha;

    if (alpha <= 0.01f || FlashColor.a <= 0.f)
        discard;

    float4 color = FlashColor;
    color.a *= alpha;

    return color;
}

#endif
