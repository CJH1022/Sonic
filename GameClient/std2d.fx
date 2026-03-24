#ifndef _TEST
#define _TEST

#include "value.fx"

#define TintColor g_vec4_0

struct VS_IN
{
    float3 vPos     : POSITION; // Sementic : Layout 에서 설명한 이름       
    float2 vUV      : TEXCOORD;    
    float4 vColor   : COLOR;
};

struct VS_OUT
{
    float4 vPosition    : SV_Position;  // 래스터라이져로 보낼때, NDC 좌표
    float2 vUV          : TEXCOORD;
    float4 vColor       : COLOR;
    float3 vWorld       : POSITION;
};

VS_OUT VS_Std2D(VS_IN _input)
{
    VS_OUT output = (VS_OUT) 0.f;
        
    float4 vWorld = mul(float4(_input.vPos, 1.f), g_matWorld);
    float4 vView = mul(vWorld, g_matView);
    float4 vProj = mul(vView, g_matProj);
        
    output.vPosition    = vProj;
    output.vUV          = _input.vUV;
    output.vColor       = _input.vColor;
    output.vWorld       = vWorld;
    
    return output;
}

// 입력된 텍스쳐를 사용해서 픽셀쉐이더의 출력 색상으로 지정한다.
float4 PS_Std2D(VS_OUT _input) : SV_Target
{      
    // 입력 UV 는 정점에사 반환한 값을 보간받아서 픽셀쉐이더에 입력됨    
    float4 vColor = float4(1.f, 0.f, 1.f, 1.f);    
    if (g_btex_0)
    {
        vColor = g_tex_0.Sample(g_sam_1, _input.vUV);
    }   
    
    vColor *= TintColor;    
    
    if (vColor.a == 0.f)
    {
        discard;
    }
        
    // 물체가 받는 빛의 총량
    float3 LightColor = float3(0.f, 0.f, 0.f);
    
    // 반복문 돌면서, 모든 광원으로부터 어느정도의 빛을 받는지 합산
    for (int i = 0; i < Light2DCount; ++i)
    {
        LightColor += CalcLight2D(i, _input.vWorld);
    }
    
    // 물체의 색상에, 자신이 받는 최종빛 총량을 곱한다.
    vColor.rgb *= LightColor;
    
    return vColor;
}




#endif