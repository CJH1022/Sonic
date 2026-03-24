#ifndef _SPRITE
#define _SPRITE

#include "value.fx"

#define AtlasTex    g_tex_0
#define LeftTopUV   g_vec2_0
#define SliceUV     g_vec2_1
#define BackgroundUV g_vec2_2
#define OffsetUV    g_vec2_3


struct VS_IN
{
    float3 vPos : POSITION; // Sementic : Layout 에서 설명한 이름       
    float2 vUV : TEXCOORD;
};

struct VS_OUT
{
    float4 vPosition : SV_Position; // 래스터라이져로 보낼때, NDC 좌표
    float2 vUV : TEXCOORD;
    float3 vWorldPos : POSITION;
};

VS_OUT VS_Sprite(VS_IN _input)
{
    VS_OUT output = (VS_OUT) 0.f;
             
    float4 vWorld = mul(float4(_input.vPos, 1.f), g_matWorld);
    float4 vView = mul(vWorld, g_matView);
    float4 vProj = mul(vView, g_matProj);
     
    output.vPosition = vProj;
    output.vWorldPos = vWorld;
    output.vUV = _input.vUV;
    
    return output;
}

// 입력된 텍스쳐를 사용해서 픽셀쉐이더의 출력 색상으로 지정한다.
float4 PS_Sprite(VS_OUT _input) : SV_Target
{    
    float2 bgUV = BackgroundUV;
    float2 offsetUV = OffsetUV;
    if (bgUV.x <= 0.f || bgUV.y <= 0.f)
    {
        bgUV = SliceUV;
        offsetUV = float2(0.f, 0.f);
    }
    
    float2 bgLocal = _input.vUV * bgUV;
    if (bgLocal.x < offsetUV.x || bgLocal.y < offsetUV.y
        || bgLocal.x > offsetUV.x + SliceUV.x
        || bgLocal.y > offsetUV.y + SliceUV.y)
    {
        discard;
    }
    
    float2 vSpriteUV = LeftTopUV + (bgLocal - offsetUV);
    float4 vColor = AtlasTex.Sample(g_sam_1, vSpriteUV);
        
    if (vColor.a < 0.1f)
        discard;
    
    // 물체가 받는 빛의 총량
    float3 LightColor = float3(0.f, 0.f, 0.f);
    
    // 반복문 돌면서, 모든 광원으로부터 어느정도의 빛을 받는지 합산
    for (int i = 0; i < Light2DCount; ++i)
    {
        LightColor += CalcLight2D(i, _input.vWorldPos);
    }
    
    // 물체의 색상에, 자신이 받는 최종빛 총량을 곱한다.
    vColor.rgb *= LightColor;
    
    
    return vColor;
}





#endif
