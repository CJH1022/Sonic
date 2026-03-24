#ifndef _TILE
#define _TILE

#include "value.fx"


struct SpriteInfo
{
    float2 LeftTop;
    float2 Slice;
};
StructuredBuffer<SpriteInfo> g_Buffer : register(t20);


#define AtlasTex    g_tex_0
#define ROW         g_int_0
#define COL         g_int_1


struct VS_IN
{
    float3 vPos : POSITION; // Sementic : Layout 에서 설명한 이름       
    float2 vUV : TEXCOORD;
};

struct VS_OUT
{
    float4 vPosition : SV_Position; // 래스터라이져로 보낼때, NDC 좌표
    float2 vUV : TEXCOORD;
    float3 vWorld : POSITION;
};

VS_OUT VS_Tile(VS_IN _input)
{
    VS_OUT output = (VS_OUT) 0.f;
             
    _input.vPos.xy += float2(0.5f, -0.5f);    
    
    float4 vWorld = mul(float4(_input.vPos, 1.f), g_matWorld);
    float4 vView = mul(vWorld, g_matView);
    float4 vProj = mul(vView, g_matProj);
     
    output.vPosition = vProj;
    output.vWorld = vWorld;
    output.vUV = _input.vUV * float2(COL, ROW);
    
    return output;
}

// 입력된 텍스쳐를 사용해서 픽셀쉐이더의 출력 색상으로 지정한다.
float4 PS_Tile(VS_OUT _input) : SV_Target
{
    int2 ColRow = int2(_input.vUV);
    int Idx = ColRow.y * COL + ColRow.x;
    
    float2 vSpriteUV = frac(_input.vUV) * g_Buffer[Idx].Slice + g_Buffer[Idx].LeftTop;
    float4 vColor = AtlasTex.Sample(g_sam_1, vSpriteUV);
    
    if (vColor.a == 0.f)
        discard;

    vColor.a *= g_float_0;
    
    // 광원 적용
    
    // 구조화버퍼의 요소 개수, 요소 하나의 크기 정보 가져오기
    // 성능 이슈가 있다. 웬만하면 상수버퍼로 해당 데이터를 따로 전달받는것이 나음
    //uint count = 0;
    //uint stride = 0;
    //g_Light2D.GetDimensions(count, stride);
       
        
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
