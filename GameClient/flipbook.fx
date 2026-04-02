#ifndef _FLIPBOOK
#define _FLIPBOOK

#include "value.fx"

#define AtlasTex        g_tex_0
#define LeftTopUV       g_vec2_0
#define SliceUV         g_vec2_1
#define BackgroundUV    g_vec2_2
#define OffsetUV        g_vec2_3
#define FlashColor      g_vec4_0
#define UseChromaKey    g_int_0
#define ChromaKeyData   g_vec4_1
#define ChromaKeyData2  g_vec4_2


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

VS_OUT VS_Flipbook(VS_IN _input)
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
// 픽셀 셰이더 수정
float4 PS_Flipbook(VS_OUT _input) : SV_Target
{
    float4 vColor = float4(1.f, 0.f, 1.f, 1.f); // 에러 시 분홍색
    
    if (g_btex_0)
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
        
        float2 SampleUV = LeftTopUV + (bgLocal - offsetUV);
        vColor = AtlasTex.Sample(g_sam_1, SampleUV);

        if (0 != UseChromaKey)
        {
            float3 keyDiff = abs(vColor.rgb - ChromaKeyData.rgb);
            if (keyDiff.r <= ChromaKeyData.a
                && keyDiff.g <= ChromaKeyData.a
                && keyDiff.b <= ChromaKeyData.a)
            {
                discard;
            }

            if (0.f < ChromaKeyData2.a)
            {
                float3 keyDiff2 = abs(vColor.rgb - ChromaKeyData2.rgb);
                if (keyDiff2.r <= ChromaKeyData2.a
                    && keyDiff2.g <= ChromaKeyData2.a
                    && keyDiff2.b <= ChromaKeyData2.a)
                {
                    discard;
                }
            }
        }

        // 기존 알파 체크도 유지
        if (vColor.a < 0.1f)
            discard;
       
    }
    
    // 광원 처리 로직 (기존 유지)
    float3 LightColor = float3(0.f, 0.f, 0.f);
    for (int i = 0; i < Light2DCount; ++i)
    {
        LightColor += CalcLight2D(i, _input.vWorldPos);
    }
    vColor.rgb *= LightColor;
    vColor.rgb = lerp(vColor.rgb, FlashColor.rgb, saturate(FlashColor.a));
    
    return vColor;
}
#endif
