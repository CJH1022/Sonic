#ifndef _DEBUG_RENDER
#define _DEBUG_RENDER

#include "value.fx"

#define COLOR g_vec4_0

struct VS_IN
{
    float3 vPos : POSITION;     
    float2 vUV : TEXCOORD;    
};

struct VS_OUT
{
    float4 vPosition : SV_Position; // 래스터라이져로 보낼때, NDC 좌표
    float2 vUV : TEXCOORD;
};

VS_OUT VS_Debug(VS_IN _input)
{
    VS_OUT output = (VS_OUT) 0.f;
        
    float4 vWorld = mul(float4(_input.vPos, 1.f), g_matWorld);
    float4 vView = mul(vWorld, g_matView);
    float4 vProj = mul(vView, g_matProj);
        
    output.vPosition = vProj;
    output.vUV = _input.vUV;  
    
    return output;
}

float4 PS_Debug(VS_OUT _input) : SV_Target
{
    return COLOR;
}



#endif