#ifndef _TILE
#define _TILE

#include "value.fx"


struct TileInfo
{
    float4 FuncParam0; // a, b, c, r
    float4 FuncParam1; // center_x, center_y, mode, tileType
    float4 FuncParam2; // customNormal.x, customNormal.y, flags, reserved
    float4 FuncParam3; // scale.x, scale.y, reserved, reserved
};
StructuredBuffer<TileInfo> g_Buffer : register(t20);

cbuffer FUNCTION : register(b3)
{
    float4 g_ContourColor;
    float4 g_NormalColor;
    float g_LineThickness;
    float g_NormalLength;
    float g_NormalThickness;
    float g_ShowOverlay;
}


#define ROW g_int_0
#define COL g_int_1
#define TILE_FLAG_USE_CUSTOM_NORMAL 4u


struct VS_IN
{
    float3 vPos : POSITION;
    float2 vUV : TEXCOORD;
};

struct VS_OUT
{
    float4 vPosition : SV_Position;
    float2 vUV : TEXCOORD;
    float3 vWorld : POSITION;
};

float DistToSegment(float2 p, float2 a, float2 b)
{
    float2 ab = b - a;
    float lenSq = max(dot(ab, ab), 1e-6f);
    float t = saturate(dot(p - a, ab) / lenSq);
    float2 q = a + ab * t;
    return length(p - q);
}

float2 ResolveOverlayNormal(float2 gradNormal, float2 customNormal, uint flags)
{
    // 커스텀 노말은 C++/충돌 코드 기준으로 "월드에서 위가 +Y"라는 감각으로 입력한다.
    // HLSL 타일 UV 는 아래 방향이 +Y 라서, 디버그 오버레이로 그릴 때만 Y 를 뒤집어 맞춘다.
    if ((flags & TILE_FLAG_USE_CUSTOM_NORMAL) != 0u)
    {
        float2 debugNormal = float2(customNormal.x, -customNormal.y);
        float lenSq = dot(debugNormal, debugNormal);
        if (lenSq > 1e-6f)
        {
            return debugNormal * rsqrt(lenSq);
        }
    }

    return gradNormal;
}

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

float4 PS_Tile(VS_OUT _input) : SV_Target
{
    int2 colRow = int2(_input.vUV);
    int idx = colRow.y * COL + colRow.x;

    TileInfo info = g_Buffer[idx];
    float tileType = info.FuncParam1.w;

    // 0, 1 번 슬롯은 비어 있는 셀/호환용 슬롯이다.
    if (tileType < 1.5f)
        discard;

    float2 tileUV = frac(_input.vUV);
    float2 tileScale = max(info.FuncParam3.xy, float2(0.001f, 0.001f));
    float2 evalUV = (tileUV - 0.5f) / tileScale + 0.5f;
    float boxDist = max(abs(tileUV.x - 0.5f) - 0.5f * tileScale.x, abs(tileUV.y - 0.5f) - 0.5f * tileScale.y);
    float aaBox = fwidth(boxDist);
    float cellMask = 1.f - smoothstep(0.f, aaBox, boxDist);
    float overlayScale = min(tileScale.x, tileScale.y);
    float mode = info.FuncParam1.z;
    uint flags = (uint)(info.FuncParam2.z + 0.5f);

    float contourMask = 0.f;
    float normalMask = 0.f;
    float solidMask = 0.f;

    if (mode > 0.5f && mode < 1.5f)
    {
        float a = info.FuncParam0.x;
        float b = info.FuncParam0.y;
        float c = info.FuncParam0.z;

        float fValue = c * (evalUV.y - (b - a) * evalUV.x - a);
        float2 grad = float2(c * (a - b), c);
        float gradLen = max(length(grad), 1e-6f);
        float signedDist = (fValue / gradLen) * overlayScale;

        float aaSolid = fwidth(signedDist);
        solidMask = (1.f - smoothstep(0.f, aaSolid, signedDist)) * cellMask;

        float distContour = abs(signedDist);
        float aaContour = fwidth(distContour);
        contourMask = 1.f - smoothstep(g_LineThickness, g_LineThickness + aaContour, distContour);

        float2 p0 = float2(0.f, a);
        float2 p1 = float2(1.f, b);
        float2 midLocal = 0.5f * (p0 + p1);
        float2 normalDir = ResolveOverlayNormal(normalize(grad), info.FuncParam2.xy, flags);
        float2 normalEndLocal = midLocal + normalDir * g_NormalLength;
        float2 mid = (midLocal - 0.5f) * tileScale + 0.5f;
        float2 normalEnd = (normalEndLocal - 0.5f) * tileScale + 0.5f;
        float distNormal = DistToSegment(tileUV, mid, normalEnd);
        float aaNormal = fwidth(distNormal);
        normalMask = 1.f - smoothstep(g_NormalThickness, g_NormalThickness + aaNormal, distNormal);
    }
    else if (mode > 1.5f && mode < 2.5f)
    {
        float r = info.FuncParam0.w;
        float2 center = info.FuncParam1.xy;
        float2 delta = evalUV - center;
        float fValue = r * r - dot(delta, delta);
        float2 grad = -2.f * delta;
        float gradLen = max(length(grad), 1e-6f);
        float signedDist = (fValue / gradLen) * overlayScale;

        float aaSolid = fwidth(signedDist);
        solidMask = (1.f - smoothstep(0.f, aaSolid, signedDist)) * cellMask;

        float distContour = abs(signedDist);
        float aaContour = fwidth(distContour);
        contourMask = 1.f - smoothstep(g_LineThickness, g_LineThickness + aaContour, distContour);

        float2 sampleDir = float2(0.5f, 0.5f) - center;
        float sampleLenSq = dot(sampleDir, sampleDir);
        if (sampleLenSq <= 1e-6f)
        {
            sampleDir = float2(0.f, -1.f);
        }
        else
        {
            sampleDir = normalize(sampleDir);
        }

        float2 normalStartLocal = center + sampleDir * r;
        float2 normalDir = center - normalStartLocal;
        float normalLenSq = dot(normalDir, normalDir);
        if (normalLenSq <= 1e-6f)
        {
            normalDir = float2(0.f, -1.f);
        }
        else
        {
            normalDir = normalize(normalDir);
        }

        normalDir = ResolveOverlayNormal(normalDir, info.FuncParam2.xy, flags);

        float2 normalEndLocal = normalStartLocal + normalDir * g_NormalLength;
        float2 normalStart = (normalStartLocal - 0.5f) * tileScale + 0.5f;
        float2 normalEnd = (normalEndLocal - 0.5f) * tileScale + 0.5f;
        float distNormal = DistToSegment(tileUV, normalStart, normalEnd);
        float aaNormal = fwidth(distNormal);
        normalMask = 1.f - smoothstep(g_NormalThickness, g_NormalThickness + aaNormal, distNormal);
    }
    else if (mode > 2.5f && mode < 3.5f)
    {
        float a = info.FuncParam0.x;
        float c = info.FuncParam0.z;

        float fValue = c * (evalUV.x - a);
        float gradLen = max(abs(c), 1e-6f);
        float signedDist = (fValue / gradLen) * overlayScale;

        float aaSolid = fwidth(signedDist);
        solidMask = (1.f - smoothstep(0.f, aaSolid, signedDist)) * cellMask;

        float distContour = abs(signedDist);
        float aaContour = fwidth(distContour);
        contourMask = 1.f - smoothstep(g_LineThickness, g_LineThickness + aaContour, distContour);

        float2 midLocal = float2(a, 0.5f);
        float2 normalDir = ResolveOverlayNormal(float2(sign(c), 0.f), info.FuncParam2.xy, flags);
        float2 normalEndLocal = midLocal + normalDir * g_NormalLength;
        float2 mid = (midLocal - 0.5f) * tileScale + 0.5f;
        float2 normalEnd = (normalEndLocal - 0.5f) * tileScale + 0.5f;
        float distNormal = DistToSegment(tileUV, mid, normalEnd);
        float aaNormal = fwidth(distNormal);
        normalMask = 1.f - smoothstep(g_NormalThickness, g_NormalThickness + aaNormal, distNormal);
    }

    float frame = min(min(tileUV.x, tileUV.y), min(1.f - tileUV.x, 1.f - tileUV.y));
    float aaFrame = fwidth(frame);
    float frameMask = 1.f - smoothstep(0.035f, 0.035f + aaFrame, frame);

    float isCircle = (mode > 1.5f && mode < 2.5f) ? 1.f : 0.f;
    float isVertical = (mode > 2.5f && mode < 3.5f) ? 1.f : 0.f;

    float3 emptyColor = float3(0.11f, 0.14f, 0.19f);
    float3 solidColor = float3(0.26f, 0.47f, 0.72f);
    float3 lineColor = float3(0.09f, 0.10f, 0.13f);

    emptyColor = lerp(emptyColor, float3(0.10f, 0.16f, 0.13f), isCircle);
    solidColor = lerp(solidColor, float3(0.23f, 0.57f, 0.39f), isCircle);
    lineColor = lerp(lineColor, float3(0.08f, 0.10f, 0.08f), isCircle);

    emptyColor = lerp(emptyColor, float3(0.17f, 0.12f, 0.08f), isVertical);
    solidColor = lerp(solidColor, float3(0.59f, 0.39f, 0.19f), isVertical);
    lineColor = lerp(lineColor, float3(0.10f, 0.07f, 0.04f), isVertical);

    float3 color = lerp(emptyColor, solidColor, solidMask);
    color = lerp(color, lineColor, frameMask * 0.5f);

    float3 lightColor = float3(0.f, 0.f, 0.f);
    for (int i = 0; i < Light2DCount; ++i)
    {
        lightColor += CalcLight2D(i, _input.vWorld);
    }

    color *= lightColor;

    if (g_ShowOverlay > 0.5f)
    {
        color = lerp(color, g_ContourColor.rgb, saturate(contourMask * g_ContourColor.a));
        color = lerp(color, g_NormalColor.rgb, saturate(normalMask * g_NormalColor.a));
    }

    return float4(color, g_float_0);
}


#endif
