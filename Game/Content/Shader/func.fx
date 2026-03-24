#ifndef _FUNC
#define _FUNC

#include "value.fx"

float3 CalcLight2D(int _LightIdx, float3 _PixelPos)
{
    float3 LightColor = float3(0.f, 0.f, 0.f);
    
    // 광원과 물체의 거리에 따른 빛의 세기 비율
    float DistRatio = 1.f;
    
    // DirectionalLight
    if (0 == g_Light2D[_LightIdx].Type )
    {
        LightColor = g_Light2D[_LightIdx].Color;//+g_Light2D[_LightIdx].Ambient;
    }
    
    // Point
    else if (1 == g_Light2D[_LightIdx].Type)
    {
        // LightColor
        LightColor = g_Light2D[_LightIdx].Color;
        
        // 광원에서 픽셀을 향하는 방향벡터(월드공간 기준)
        float3 LightToPixel = _PixelPos - g_Light2D[_LightIdx].WorldPos;
        
        // 2차원으로만 거리를 따질 것이기 때문에, z 성분 제거
        LightToPixel.z = 0.f;        
        
        // 벡터의 길이 측정(광원과 픽셀 사이의 거리)
        float Dist = length(LightToPixel);    
        
        // 물체와 광원의 거리가 멀어질수록 빛의 세기가 0 에 가까워진다.
        //DistRatio = saturate(1.f - (Dist / g_Light2D[_LightIdx].Radius) );                   
        //DistRatio = saturate(cos(Dist * ((PI / 2.f) / g_Light2D[_LightIdx].Radius)));
        DistRatio = saturate(cos(saturate(Dist / g_Light2D[_LightIdx].Radius) * (PI / 2.f)));
    }
    
    // Spot
    else
    {
        // LightColor
        LightColor = g_Light2D[_LightIdx].Color;
        
        // 광원에서 픽셀을 향하는 방향벡터(월드공간 기준)
        float3 LightToPixel = _PixelPos - g_Light2D[_LightIdx].WorldPos;
        
        // 2차원으로만 거리를 따질 것이기 때문에, z 성분 제거
        LightToPixel.z = 0.f;
        
        // 벡터의 길이 측정(광원과 픽셀 사이의 거리)
        float Dist = length(LightToPixel);
                
        // SpotLight 각도 범위 이내인지 검사
        float3 vLightDir = normalize(g_Light2D[_LightIdx].LightDir);        
        float3 vLightToPixel = normalize(LightToPixel);
        float Theta = acos(dot(vLightToPixel, vLightDir));
                
        if (Theta < g_Light2D[_LightIdx].Angle * 0.5f)
            DistRatio = saturate(cos(saturate(Dist / g_Light2D[_LightIdx].Radius) * (PI / 2.f)));
        else
            DistRatio = 0.f;
    }
    
    return LightColor * DistRatio;
}

#endif


