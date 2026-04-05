#include "pch.h"
#include "CSurfaceCircleGuideScript.h"

#include "func.h"

#include <cmath>

namespace
{
    float NormalizeAngleDegrees(float _AngleDeg)
    {
        while (_AngleDeg <= -180.f)
            _AngleDeg += 360.f;

        while (_AngleDeg > 180.f)
            _AngleDeg -= 360.f;

        return _AngleDeg;
    }

    Vec2 MakePointFromAngleDegrees(float _AngleDeg, float _Radius)
    {
        const float angleRad = _AngleDeg * XM_PI / 180.f;
        return Vec2(cosf(angleRad) * _Radius, sinf(angleRad) * _Radius);
    }
}

CSurfaceCircleGuideScript::CSurfaceCircleGuideScript()
    : CScript(SCRIPT_TYPE::SURFACECIRCLEGUIDESCRIPT)
    , m_Enabled(false)
    , m_EntryAngleDeg(-35.f)
    , m_HalfCheckAngleDeg(90.f)
    , m_ExitAngleDeg(-145.f)
    , m_CorrectionLineStartLocal(Vec2(-160.f, -110.f))
    , m_CorrectionLineEndLocal(Vec2(160.f, -110.f))
{
    ResetToDefaultGuide(200.f);

    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_EntryAngleDeg, L"Guide Entry Angle", true, 1.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_HalfCheckAngleDeg, L"Guide Half Angle", true, 1.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_ExitAngleDeg, L"Guide Exit Angle", true, 1.f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &m_CorrectionLineStartLocal, L"Guide Corr Start", true, 1.f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &m_CorrectionLineEndLocal, L"Guide Corr End", true, 1.f);
}

CSurfaceCircleGuideScript::CSurfaceCircleGuideScript(const CSurfaceCircleGuideScript& _Origin)
    : CScript(_Origin)
    , m_Enabled(_Origin.m_Enabled)
    , m_EntryAngleDeg(_Origin.m_EntryAngleDeg)
    , m_HalfCheckAngleDeg(_Origin.m_HalfCheckAngleDeg)
    , m_ExitAngleDeg(_Origin.m_ExitAngleDeg)
    , m_CorrectionLineStartLocal(_Origin.m_CorrectionLineStartLocal)
    , m_CorrectionLineEndLocal(_Origin.m_CorrectionLineEndLocal)
    , m_LinkedCorrectionLineName(_Origin.m_LinkedCorrectionLineName)
{
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_EntryAngleDeg, L"Guide Entry Angle", true, 1.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_HalfCheckAngleDeg, L"Guide Half Angle", true, 1.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_ExitAngleDeg, L"Guide Exit Angle", true, 1.f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &m_CorrectionLineStartLocal, L"Guide Corr Start", true, 1.f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &m_CorrectionLineEndLocal, L"Guide Corr End", true, 1.f);
}

CSurfaceCircleGuideScript::~CSurfaceCircleGuideScript()
{
}

void CSurfaceCircleGuideScript::Begin()
{
}

void CSurfaceCircleGuideScript::Tick()
{
}

void CSurfaceCircleGuideScript::SetEntryAngleDeg(float _AngleDeg)
{
    m_EntryAngleDeg = NormalizeAngleDegrees(_AngleDeg);
}

void CSurfaceCircleGuideScript::SetHalfCheckAngleDeg(float _AngleDeg)
{
    m_HalfCheckAngleDeg = NormalizeAngleDegrees(_AngleDeg);
}

void CSurfaceCircleGuideScript::SetExitAngleDeg(float _AngleDeg)
{
    m_ExitAngleDeg = NormalizeAngleDegrees(_AngleDeg);
}

void CSurfaceCircleGuideScript::ResetToDefaultGuide(float _Radius)
{
    const float safeRadius = max(_Radius, 1.f);

    m_EntryAngleDeg = -35.f;
    m_HalfCheckAngleDeg = 90.f;
    m_ExitAngleDeg = -145.f;
    m_CorrectionLineStartLocal = MakePointFromAngleDegrees(-145.f, safeRadius);
    m_CorrectionLineEndLocal = MakePointFromAngleDegrees(-35.f, safeRadius);
}

void CSurfaceCircleGuideScript::CopyFrom(const CSurfaceCircleGuideScript& _Other)
{
    if (this == &_Other)
        return;

    m_Enabled = _Other.m_Enabled;
    m_EntryAngleDeg = _Other.m_EntryAngleDeg;
    m_HalfCheckAngleDeg = _Other.m_HalfCheckAngleDeg;
    m_ExitAngleDeg = _Other.m_ExitAngleDeg;
    m_CorrectionLineStartLocal = _Other.m_CorrectionLineStartLocal;
    m_CorrectionLineEndLocal = _Other.m_CorrectionLineEndLocal;
    m_LinkedCorrectionLineName = _Other.m_LinkedCorrectionLineName;
}

void CSurfaceCircleGuideScript::SaveToLevelFile(FILE* _File)
{
    fwrite(&m_Enabled, sizeof(bool), 1, _File);
    fwrite(&m_EntryAngleDeg, sizeof(float), 1, _File);
    fwrite(&m_HalfCheckAngleDeg, sizeof(float), 1, _File);
    fwrite(&m_ExitAngleDeg, sizeof(float), 1, _File);
    fwrite(&m_CorrectionLineStartLocal, sizeof(Vec2), 1, _File);
    fwrite(&m_CorrectionLineEndLocal, sizeof(Vec2), 1, _File);
    SaveWString(_File, m_LinkedCorrectionLineName);
}

void CSurfaceCircleGuideScript::LoadFromLevelFile(FILE* _File)
{
    fread(&m_Enabled, sizeof(bool), 1, _File);
    fread(&m_EntryAngleDeg, sizeof(float), 1, _File);
    fread(&m_HalfCheckAngleDeg, sizeof(float), 1, _File);
    fread(&m_ExitAngleDeg, sizeof(float), 1, _File);
    fread(&m_CorrectionLineStartLocal, sizeof(Vec2), 1, _File);
    fread(&m_CorrectionLineEndLocal, sizeof(Vec2), 1, _File);
    m_LinkedCorrectionLineName = LoadWString(_File);

    m_EntryAngleDeg = NormalizeAngleDegrees(m_EntryAngleDeg);
    m_HalfCheckAngleDeg = NormalizeAngleDegrees(m_HalfCheckAngleDeg);
    m_ExitAngleDeg = NormalizeAngleDegrees(m_ExitAngleDeg);
}
