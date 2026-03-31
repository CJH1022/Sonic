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

// 버텍스 셰이더에서는 위치 계산만 수행합니다.
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
    
    // (이곳에 있던 discard 로직은 삭제합니다)
    
    return output;
}

// 픽셀 셰이더에서 텍스처 색상을 확인하고 투명 처리를 합니다.
float4 PS_Billboard(VS_OUT _input) : SV_Target
{
    // 1. 텍스처에서 현재 픽셀의 색상을 가져옵니다.
    float4 vTexColor = g_tex_0.Sample(g_sam_1, _input.vUV);
    
    // 2. 지우고자 하는 타겟 색상을 설정합니다.
    float3 targetColor = float3(188.0f / 255.0f, 32.0f / 255.0f, 55.0f / 255.0f);
    float epsilon = 0.1f; // 부동소수점 오차 허용 범위 (필요시 조절)

    // 3. 추출한 텍스처 색상과 타겟 색상의 차이가 오차 범위 내인지 확인합니다.
    if (abs(vTexColor.r - targetColor.r) < epsilon &&
        abs(vTexColor.g - targetColor.g) < epsilon &&
        abs(vTexColor.b - targetColor.b) < epsilon)
    {
        // 배경색과 일치하면 픽셀을 그리지 않고 폐기합니다.
        discard;
    }
    
    // 정점 색상(필요하다면)과 텍스처 색상을 곱해서 최종 반환합니다.
    return vTexColor * _input.vColor;
}

#endif