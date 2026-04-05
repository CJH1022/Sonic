#include "pch.h"
#include "ASurfaceSet.h"

#include "func.h"

namespace
{
    constexpr UINT kSurfaceSetFileMagic = 0x54455353; // 'SSET'
    constexpr UINT kSurfaceSetFileVersion = 2;
}

ASurfaceSet::ASurfaceSet()
    : Asset(ASSET_TYPE::SURFACESET)
{
}

ASurfaceSet::~ASurfaceSet()
{
}

UINT ASurfaceSet::AddSurface(const SURFACE_DESC& _Desc)
{
    m_vecSurfaceDescs.push_back(_Desc);
    return (UINT)m_vecSurfaceDescs.size() - 1;
}

bool ASurfaceSet::RemoveSurface(UINT _Idx)
{
    if (m_vecSurfaceDescs.size() <= _Idx)
        return false;

    m_vecSurfaceDescs.erase(m_vecSurfaceDescs.begin() + _Idx);
    return true;
}

int ASurfaceSet::Save(const wstring& _FilePath)
{
    FILE* pFile = nullptr;
    _wfopen_s(&pFile, _FilePath.c_str(), L"wb");
    if (nullptr == pFile)
        return E_FAIL;

    fwrite(&kSurfaceSetFileMagic, sizeof(UINT), 1, pFile);
    fwrite(&kSurfaceSetFileVersion, sizeof(UINT), 1, pFile);

    const UINT surfaceCount = (UINT)m_vecSurfaceDescs.size();
    fwrite(&surfaceCount, sizeof(UINT), 1, pFile);

    for (const SURFACE_DESC& desc : m_vecSurfaceDescs)
    {
        SaveWString(pFile, desc.Name);

        const UINT geometry = (UINT)desc.Geometry;
        const UINT role = (UINT)desc.Role;
        const UINT arcCorner = (UINT)desc.ArcCorner;

        fwrite(&geometry, sizeof(UINT), 1, pFile);
        fwrite(&role, sizeof(UINT), 1, pFile);
        fwrite(&arcCorner, sizeof(UINT), 1, pFile);
        fwrite(&desc.LocalPosition, sizeof(Vec3), 1, pFile);
        fwrite(&desc.LocalStart, sizeof(Vec2), 1, pFile);
        fwrite(&desc.LocalEnd, sizeof(Vec2), 1, pFile);
        fwrite(&desc.FillAbove, sizeof(bool), 1, pFile);
        fwrite(&desc.FillInside, sizeof(bool), 1, pFile);
        fwrite(&desc.Attachable, sizeof(bool), 1, pFile);
        fwrite(&desc.HasCircleGuide, sizeof(bool), 1, pFile);
        fwrite(&desc.CircleGuideEnabled, sizeof(bool), 1, pFile);
        fwrite(&desc.GuideEntryAngleDeg, sizeof(float), 1, pFile);
        fwrite(&desc.GuideHalfCheckAngleDeg, sizeof(float), 1, pFile);
        fwrite(&desc.GuideExitAngleDeg, sizeof(float), 1, pFile);
        fwrite(&desc.GuideCorrectionLineStartLocal, sizeof(Vec2), 1, pFile);
        fwrite(&desc.GuideCorrectionLineEndLocal, sizeof(Vec2), 1, pFile);
        SaveWString(pFile, desc.LinkedCorrectionLineName);
    }

    fclose(pFile);
    return S_OK;
}

int ASurfaceSet::Load(const wstring& _FilePath)
{
    FILE* pFile = nullptr;
    _wfopen_s(&pFile, _FilePath.c_str(), L"rb");
    if (nullptr == pFile)
        return E_FAIL;

    UINT magic = 0;
    fread(&magic, sizeof(UINT), 1, pFile);
    if (magic != kSurfaceSetFileMagic)
    {
        fclose(pFile);
        return E_FAIL;
    }

    UINT version = 0;
    fread(&version, sizeof(UINT), 1, pFile);
    if (version > kSurfaceSetFileVersion)
    {
        fclose(pFile);
        return E_FAIL;
    }

    UINT surfaceCount = 0;
    fread(&surfaceCount, sizeof(UINT), 1, pFile);

    m_vecSurfaceDescs.clear();
    m_vecSurfaceDescs.resize(surfaceCount);

    for (UINT i = 0; i < surfaceCount; ++i)
    {
        SURFACE_DESC& desc = m_vecSurfaceDescs[i];
        desc.Name = LoadWString(pFile);

        UINT geometry = 0;
        UINT role = 0;
        UINT arcCorner = 0;

        fread(&geometry, sizeof(UINT), 1, pFile);
        fread(&role, sizeof(UINT), 1, pFile);
        fread(&arcCorner, sizeof(UINT), 1, pFile);
        fread(&desc.LocalPosition, sizeof(Vec3), 1, pFile);
        fread(&desc.LocalStart, sizeof(Vec2), 1, pFile);
        fread(&desc.LocalEnd, sizeof(Vec2), 1, pFile);
        fread(&desc.FillAbove, sizeof(bool), 1, pFile);
        fread(&desc.FillInside, sizeof(bool), 1, pFile);
        fread(&desc.Attachable, sizeof(bool), 1, pFile);

        if (version >= 2)
        {
            fread(&desc.HasCircleGuide, sizeof(bool), 1, pFile);
            fread(&desc.CircleGuideEnabled, sizeof(bool), 1, pFile);
            fread(&desc.GuideEntryAngleDeg, sizeof(float), 1, pFile);
            fread(&desc.GuideHalfCheckAngleDeg, sizeof(float), 1, pFile);
            fread(&desc.GuideExitAngleDeg, sizeof(float), 1, pFile);
            fread(&desc.GuideCorrectionLineStartLocal, sizeof(Vec2), 1, pFile);
            fread(&desc.GuideCorrectionLineEndLocal, sizeof(Vec2), 1, pFile);
            desc.LinkedCorrectionLineName = LoadWString(pFile);
        }
        else
        {
            desc.HasCircleGuide = false;
            desc.CircleGuideEnabled = false;
            desc.GuideEntryAngleDeg = -35.f;
            desc.GuideHalfCheckAngleDeg = 90.f;
            desc.GuideExitAngleDeg = -145.f;
            desc.GuideCorrectionLineStartLocal = Vec2(0.f, 0.f);
            desc.GuideCorrectionLineEndLocal = Vec2(0.f, 0.f);
            desc.LinkedCorrectionLineName.clear();
        }

        desc.Geometry = (SURFACE_SET_GEOMETRY)geometry;
        desc.Role = (SURFACE_SET_ROLE)role;
        desc.ArcCorner = (SURFACE_SET_ARC_CORNER)arcCorner;
    }

    fclose(pFile);
    return S_OK;
}
