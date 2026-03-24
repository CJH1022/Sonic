#ifndef _BILLBOARD
#define _BILLBOARD

#include "value.fx"


#define BILLBOARD_SCALE g_vec2_0

struct VS_IN
{
    float3 vPos : POSITION; // Sementic : Layout 에서 설명한 이름       
    float2 vUV : TEXCOORD;
    float4 vColor : COLOR;
};

struct VS_OUT
{
    float4 vPosition : SV_Position; // 래스터라이져로 보낼때, NDC 좌표
    float2 vUV : TEXCOORD;
    float4 vColor : COLOR;
};

VS_OUT VS_Billboard(VS_IN _input)
{
    VS_OUT output = (VS_OUT) 0.f;
             
    float4 vWorld = mul(float4(0.f, 0.f, 0.f, 1.f), g_matWorld);
    float4 vView = mul(vWorld, g_matView);
    
    vView.xy += _input.vPos.xy * BILLBOARD_SCALE;    
    float4 vProj = mul(vView, g_matProj);
    
    output.vPosition = vProj;
    output.vUV = _input.vUV;
    output.vColor = _input.vColor;
    
    return output;
}

// 입력된 텍스쳐를 사용해서 픽셀쉐이더의 출력 색상으로 지정한다.
float4 PS_Billboard(VS_OUT _input) : SV_Target
{    
    return g_tex_0.Sample(g_sam_1, _input.vUV);
}


#endif