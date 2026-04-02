#include "pch.h"
#include "CFireScript.h"

#include "AssetMgr.h"
#include "CCollider2D.h"
#include "CFlipbookRender.h"
#include "CTransform.h"
#include "GameObject.h"
#include "TimeMgr.h"
#include "Source/Scripts/CKnockbackScript.h"

#include <cmath>

namespace
{
    constexpr int kLargeFireFlipbookIndex = 0;
    constexpr int kMiddleFireFlipbookIndex = 1;
    constexpr int kSmallFireFlipbookIndex = 2;

    constexpr const wchar_t* kLargeSegmentName1 = L"FireSegment_Large_1";
    constexpr const wchar_t* kLargeSegmentName2 = L"FireSegment_Large_2";
    constexpr const wchar_t* kMiddleSegmentName1 = L"FireSegment_Middle_1";
    constexpr const wchar_t* kMiddleSegmentName2 = L"FireSegment_Middle_2";
    constexpr const wchar_t* kSmallSegmentName = L"FireSegment_Small";
    const Vec2 kFireSegmentColliderScale = Vec2(0.82f, 0.82f);
    const Vec2 kFireSegmentDisabledColliderScale = Vec2(0.001f, 0.001f);

    float Clamp01f(float _Value)
    {
        if (_Value < 0.f)
            return 0.f;
        if (_Value > 1.f)
            return 1.f;
        return _Value;
    }

    float Maxf(float _A, float _B)
    {
        return (_A > _B) ? _A : _B;
    }

    float LengthVec2(const Vec2& _Vec)
    {
        return sqrtf(_Vec.x * _Vec.x + _Vec.y * _Vec.y);
    }

    Vec2 NormalizeSafeVec2(const Vec2& _Vec, const Vec2& _Fallback = Vec2(1.f, 0.f))
    {
        const float length = LengthVec2(_Vec);
        if (length <= 0.0001f)
            return _Fallback;

        return Vec2(_Vec.x / length, _Vec.y / length);
    }

    void NormalizeFireSprite(const Ptr<ASprite>& _Sprite)
    {
        if (_Sprite == nullptr)
            return;

        const Vec2 sliceUV = _Sprite->GetSliceUV();
        const Vec2 backgroundUV = _Sprite->GetBackgroundUV();

        if (backgroundUV.x <= 0.f || backgroundUV.y <= 0.f)
            return;

        if (sliceUV.x <= 0.f || sliceUV.y <= 0.f
            || (backgroundUV.x > sliceUV.x * 4.f && backgroundUV.y > sliceUV.y * 4.f))
        {
            _Sprite->SetSliceUV(backgroundUV);
            _Sprite->SetOffsetUV(Vec2(0.f, 0.f));
        }
    }

    void NormalizeFireFlipbook(const Ptr<AFlipbook>& _Flipbook)
    {
        if (_Flipbook == nullptr)
            return;

        const UINT spriteCount = _Flipbook->GetSpriteCount();
        for (UINT i = 0; i < spriteCount; ++i)
        {
            NormalizeFireSprite(_Flipbook->GetSprite((int)i));
        }
    }
}

CFireScript::CFireScript()
    : CScript(SCRIPT_TYPE::FIRESCRIPT)
    , m_MoveDir(Vec2(1.f, 0.f))
    , m_BaseScale(Vec2(150.f, 110.f))
    , m_KnockBackPower(Vec2(320.f, 220.f))
    , m_MoveSpeed(0.f)
    , m_FlameLength(240.f)
    , m_LifeTime(0.65f)
    , m_FlameFPS(12.f)
    , m_PulseSpeed(18.f)
    , m_PulseAmount(0.18f)
    , m_AccTime(0.f)
    , m_Loop(0)
    , m_Active(true)
{
    AddScriptParam(SCRIPT_PARAM::VEC2, &m_MoveDir, L"MoveDir", false, 0.1f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &m_BaseScale, L"BaseScale", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &m_KnockBackPower, L"KnockBackPower", false, 10.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_MoveSpeed, L"MoveSpeed", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_FlameLength, L"FlameLength", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_LifeTime, L"LifeTime", false, 0.05f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_FlameFPS, L"FlameFPS", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_PulseSpeed, L"PulseSpeed", false, 0.1f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_PulseAmount, L"PulseAmount", false, 0.01f);
    AddScriptParam(SCRIPT_PARAM::INT, &m_Loop, L"Loop", false, 1.f);
}

CFireScript::CFireScript(const CFireScript& _Origin)
    : CScript(_Origin)
    , m_MoveDir(_Origin.m_MoveDir)
    , m_BaseScale(_Origin.m_BaseScale)
    , m_KnockBackPower(_Origin.m_KnockBackPower)
    , m_MoveSpeed(_Origin.m_MoveSpeed)
    , m_FlameLength(_Origin.m_FlameLength)
    , m_LifeTime(_Origin.m_LifeTime)
    , m_FlameFPS(_Origin.m_FlameFPS)
    , m_PulseSpeed(_Origin.m_PulseSpeed)
    , m_PulseAmount(_Origin.m_PulseAmount)
    , m_AccTime(0.f)
    , m_Loop(_Origin.m_Loop)
    , m_Active(_Origin.m_Active)
{
    AddScriptParam(SCRIPT_PARAM::VEC2, &m_MoveDir, L"MoveDir", false, 0.1f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &m_BaseScale, L"BaseScale", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &m_KnockBackPower, L"KnockBackPower", false, 10.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_MoveSpeed, L"MoveSpeed", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_FlameLength, L"FlameLength", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_LifeTime, L"LifeTime", false, 0.05f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_FlameFPS, L"FlameFPS", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_PulseSpeed, L"PulseSpeed", false, 0.1f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_PulseAmount, L"PulseAmount", false, 0.01f);
    AddScriptParam(SCRIPT_PARAM::INT, &m_Loop, L"Loop", false, 1.f);
}

CFireScript::~CFireScript()
{
}

void CFireScript::Begin()
{
    m_AccTime = 0.f;

    if (Transform() != nullptr)
        Transform()->SetIndependentScale(true);

    EnsureSegments();
    if (!m_Active)
        SetAllSegmentsVisible(false);
}

void CFireScript::Tick()
{
    if (FindSegment(kLargeSegmentName1) == nullptr
        || FindSegment(kLargeSegmentName2) == nullptr
        || FindSegment(kMiddleSegmentName1) == nullptr
        || FindSegment(kMiddleSegmentName2) == nullptr
        || FindSegment(kSmallSegmentName) == nullptr)
    {
        EnsureSegments();
    }

    if (!m_Active)
    {
        SetAllSegmentsVisible(false);
        return;
    }

    Vec2 dir = NormalizeSafeVec2(m_MoveDir);
    const float rotationZ = atan2f(dir.y, dir.x);

    if (m_MoveSpeed != 0.f)
    {
        Vec3 pos = Transform()->GetRelativePos();
        pos.x += dir.x * m_MoveSpeed * DT;
        pos.y += dir.y * m_MoveSpeed * DT;
        Transform()->SetRelativePos(pos);
    }

    if (m_LifeTime <= 0.01f)
        m_LifeTime = 0.01f;
    if (m_FlameFPS <= 0.1f)
        m_FlameFPS = 0.1f;

    m_AccTime += DT;

    if (m_Loop != 0)
    {
        while (m_AccTime >= m_LifeTime)
            m_AccTime -= m_LifeTime;
    }
    else if (m_AccTime >= m_LifeTime)
    {
        GetOwner()->Destroy();
        return;
    }

    const float lifeRatio = Clamp01f(m_AccTime / m_LifeTime);
    const float warmupRatio = 0.18f;
    const float cooldownStart = 0.78f;

    float deployRatio = 1.f;
    if (lifeRatio < warmupRatio)
        deployRatio = lifeRatio / warmupRatio;
    else if (cooldownStart < lifeRatio)
        deployRatio = 1.f - ((lifeRatio - cooldownStart) / (1.f - cooldownStart));

    deployRatio = Clamp01f(deployRatio);

    const float squarePulse = 1.f + sinf(m_AccTime * m_PulseSpeed) * (m_PulseAmount * 0.55f);
    const float baseSide = Maxf(1.f, fminf(fabsf(m_BaseScale.x), fabsf(m_BaseScale.y)));
    const float activeLength = Maxf(baseSide * 0.9f,
                                    m_FlameLength * (0.28f + 0.72f * deployRatio) * squarePulse);

    const float smallDistance = activeLength * 0.12f;
    const float middleDistance1 = activeLength * 0.30f;
    const float middleDistance2 = activeLength * 0.48f;
    const float largeDistance1 = activeLength * 0.66f;
    const float largeDistance2 = activeLength * 0.84f;

    const float smallSide = baseSide * 0.58f * deployRatio * squarePulse;
    const float middleSide1 = baseSide * 0.82f * deployRatio * squarePulse;
    const float middleSide2 = baseSide * 0.92f * deployRatio * squarePulse;
    const float largeSide1 = baseSide * 1.02f * deployRatio * squarePulse;
    const float largeSide2 = baseSide * 1.10f * deployRatio * squarePulse;

    const Vec2 smallScale = Vec2(smallSide, smallSide);
    const Vec2 middleScale1 = Vec2(middleSide1, middleSide1);
    const Vec2 middleScale2 = Vec2(middleSide2, middleSide2);
    const Vec2 largeScale1 = Vec2(largeSide1, largeSide1);
    const Vec2 largeScale2 = Vec2(largeSide2, largeSide2);

    // Boss side -> flame tip order: small, middle, middle, large, large.
    UpdateSegment(kSmallSegmentName, dir, smallDistance, smallScale, rotationZ,
                  (deployRatio > 0.04f), kSmallFireFlipbookIndex, 1.f);
    UpdateSegment(kMiddleSegmentName1, dir, middleDistance1, middleScale1, rotationZ,
                  (deployRatio > 0.10f), kMiddleFireFlipbookIndex, m_FlameFPS * 1.05f);
    UpdateSegment(kMiddleSegmentName2, dir, middleDistance2, middleScale2, rotationZ,
                  (deployRatio > 0.16f), kMiddleFireFlipbookIndex, m_FlameFPS * 1.15f);
    UpdateSegment(kLargeSegmentName1, dir, largeDistance1, largeScale1, rotationZ,
                  (deployRatio > 0.22f), kLargeFireFlipbookIndex, m_FlameFPS);
    UpdateSegment(kLargeSegmentName2, dir, largeDistance2, largeScale2, rotationZ,
                  (deployRatio > 0.28f), kLargeFireFlipbookIndex, m_FlameFPS * 0.95f);
}

void CFireScript::SaveToLevelFile(FILE* _File)
{
    fwrite(&m_MoveDir, sizeof(Vec2), 1, _File);
    fwrite(&m_BaseScale, sizeof(Vec2), 1, _File);
    fwrite(&m_MoveSpeed, sizeof(float), 1, _File);
    fwrite(&m_FlameLength, sizeof(float), 1, _File);
    fwrite(&m_LifeTime, sizeof(float), 1, _File);
    fwrite(&m_FlameFPS, sizeof(float), 1, _File);
    fwrite(&m_PulseSpeed, sizeof(float), 1, _File);
    fwrite(&m_PulseAmount, sizeof(float), 1, _File);
    fwrite(&m_Loop, sizeof(int), 1, _File);
}

void CFireScript::LoadFromLevelFile(FILE* _File)
{
    fread(&m_MoveDir, sizeof(Vec2), 1, _File);
    fread(&m_BaseScale, sizeof(Vec2), 1, _File);
    fread(&m_MoveSpeed, sizeof(float), 1, _File);
    fread(&m_FlameLength, sizeof(float), 1, _File);
    fread(&m_LifeTime, sizeof(float), 1, _File);
    fread(&m_FlameFPS, sizeof(float), 1, _File);
    fread(&m_PulseSpeed, sizeof(float), 1, _File);
    fread(&m_PulseAmount, sizeof(float), 1, _File);
    fread(&m_Loop, sizeof(int), 1, _File);
    m_AccTime = 0.f;
}

void CFireScript::SetActive(bool _Active)
{
    if (m_Active == _Active)
        return;

    m_Active = _Active;
    m_AccTime = 0.f;

    if (!m_Active)
        SetAllSegmentsVisible(false);
}

GameObject* CFireScript::FindSegment(const wchar_t* _Name)
{
    if (GetOwner() == nullptr)
        return nullptr;

    const vector<Ptr<GameObject>>& children = GetOwner()->GetChild();
    for (const auto& child : children)
    {
        if (child != nullptr && child->GetName() == _Name)
            return child.Get();
    }

    return nullptr;
}

void CFireScript::EnsureSegments()
{
    if (GetOwner() == nullptr)
        return;

    struct SegmentInit
    {
        const wchar_t* Name;
        const wchar_t* FlipbookPath;
        int FlipbookIndex;
        float FPS;
        int StartSprite;
    };

    const SegmentInit segments[] =
    {
        // 1번 애니메이션 2개
        { kLargeSegmentName1,  L"Flipbook\\Boss_FireAttack1.flip", kLargeFireFlipbookIndex,  m_FlameFPS, 0 },
        { kLargeSegmentName2,  L"Flipbook\\Boss_FireAttack1.flip", kLargeFireFlipbookIndex,  m_FlameFPS, 2 },

        // 2번 애니메이션 2개
        { kMiddleSegmentName1, L"Flipbook\\Boss_FireAttack2.flip", kMiddleFireFlipbookIndex, m_FlameFPS * 1.1f, 0 },
        { kMiddleSegmentName2, L"Flipbook\\Boss_FireAttack2.flip", kMiddleFireFlipbookIndex, m_FlameFPS * 1.2f, 2 },

        // 3번 애니메이션 1개
        { kSmallSegmentName,   L"Flipbook\\Boss_FireAttack3.flip", kSmallFireFlipbookIndex, 1.f, 0 },
    };

    for (const SegmentInit& segmentInit : segments)
    {
        GameObject* pSegment = FindSegment(segmentInit.Name);
        if (pSegment == nullptr)
        {
            Ptr<GameObject> pNewSegment = new GameObject;
            pNewSegment->SetName(segmentInit.Name);
            pNewSegment->AddComponent(new CTransform);
            pNewSegment->AddComponent(new CCollider2D);
            pNewSegment->AddComponent(new CFlipbookRender);
            pNewSegment->AddComponent(new CKnockbackScript);
            pNewSegment->Transform()->SetIndependentScale(true);
            GetOwner()->AddChild(pNewSegment);
            pSegment = pNewSegment.Get();
        }

        if (pSegment == nullptr || pSegment->FlipbookRender() == nullptr)
            continue;

        if (pSegment->Collider2D() == nullptr)
            pSegment->AddComponent(new CCollider2D);

        Ptr<CKnockbackScript> pKnockbackScript = pSegment->GetScript<CKnockbackScript>();
        if (pKnockbackScript == nullptr)
        {
            pSegment->AddComponent(new CKnockbackScript);
            pKnockbackScript = pSegment->GetScript<CKnockbackScript>();
        }

        if (pSegment->Collider2D() != nullptr)
            pSegment->Collider2D()->SetScale(kFireSegmentColliderScale);

        if (pKnockbackScript != nullptr)
            pKnockbackScript->SetKnockBackPower(m_KnockBackPower);

        Ptr<AFlipbook> pFlipbook = LOAD(AFlipbook, segmentInit.FlipbookPath);
        if (pFlipbook != nullptr)
        {
            NormalizeFireFlipbook(pFlipbook);
            pSegment->FlipbookRender()->SetFlipbook(segmentInit.FlipbookIndex, pFlipbook);
            pSegment->FlipbookRender()->Play(segmentInit.FlipbookIndex, segmentInit.StartSprite, segmentInit.FPS, -1);
        }
    }
}

void CFireScript::SetSegmentVisible(const wchar_t* _Name, bool _Visible)
{
    GameObject* pSegment = FindSegment(_Name);
    if (pSegment == nullptr || pSegment->FlipbookRender() == nullptr)
        return;

    pSegment->FlipbookRender()->SetVisible(_Visible);

    if (pSegment->Collider2D() != nullptr)
        pSegment->Collider2D()->SetScale(_Visible ? kFireSegmentColliderScale : kFireSegmentDisabledColliderScale);
}

void CFireScript::SetAllSegmentsVisible(bool _Visible)
{
    SetSegmentVisible(kLargeSegmentName1, _Visible);
    SetSegmentVisible(kLargeSegmentName2, _Visible);
    SetSegmentVisible(kMiddleSegmentName1, _Visible);
    SetSegmentVisible(kMiddleSegmentName2, _Visible);
    SetSegmentVisible(kSmallSegmentName, _Visible);
}

void CFireScript::UpdateSegment(const wchar_t* _Name, const Vec2& _Dir, float _Distance, const Vec2& _Scale,
                                float _RotationZ, bool _Visible, int _FlipbookIndex, float _FPS)
{
    GameObject* pSegment = FindSegment(_Name);
    if (pSegment == nullptr || pSegment->Transform() == nullptr || pSegment->FlipbookRender() == nullptr)
        return;

    pSegment->Transform()->SetRelativePos(Vec3(_Dir.x * _Distance, _Dir.y * _Distance, 0.f));
    pSegment->Transform()->SetRelativeRot(Vec3(0.f, 0.f, _RotationZ));
    pSegment->Transform()->SetRelativeScale(Vec3(_Scale.x, _Scale.y, 1.f));

    pSegment->FlipbookRender()->SetVisible(_Visible);

    if (pSegment->Collider2D() != nullptr)
        pSegment->Collider2D()->SetScale(_Visible ? kFireSegmentColliderScale : kFireSegmentDisabledColliderScale);

    Ptr<CKnockbackScript> pKnockbackScript = pSegment->GetScript<CKnockbackScript>();
    if (pKnockbackScript != nullptr)
        pKnockbackScript->SetKnockBackPower(m_KnockBackPower);

    if (_Visible)
        pSegment->FlipbookRender()->Play(_FlipbookIndex, _FPS, -1);
}
