#include "pch.h"
#include "CCoinScript.h"
#include "CCoinMgrScript.h"
#include "func.h"
#include "AFlipbook.h"
#include "AssetMgr.h"
#include "LevelMgr.h"
#include "CEffectScript.h"

#include "CSurfaceScript.h"
#include "CTileScript.h"
#include "CBlockScript.h"
#include "CBlockMovingScript.h"
#include "CBlockPushingScript.h"
#include "TimeMgr.h"

namespace
{
    constexpr float kKnockbackCoinLifeTime = 5.f;
    constexpr float kKnockbackCoinBlinkStartTime = 4.f;
    constexpr float kKnockbackCoinBlinkInterval = 0.12f;
    constexpr float kKnockbackCoinPickupDelay = 0.45f;

    float Dot2(const Vec2& a, const Vec2& b)
    {
        return a.x * b.x + a.y * b.y;
    }

    float LengthSq(const Vec2& v)
    {
        return v.x * v.x + v.y * v.y;
    }

    Vec2 NormalizeSafe(const Vec2& v, const Vec2& fallback = Vec2(0.f, 1.f))
    {
        const float lenSq = LengthSq(v);
        if (lenSq <= 0.0001f)
            return fallback;

        const float len = sqrtf(lenSq);
        return Vec2(v.x / len, v.y / len);
    }

    float Clamp01(float t)
    {
        if (t < 0.f) return 0.f;
        if (t > 1.f) return 1.f;
        return t;
    }
}

bool CCoinScript::CountsAsCoinPhysicalGround(GameObject* _Obj) const
{
    if (_Obj == nullptr)
        return false;

    if (_Obj->GetScript<CTileScript>() != nullptr)         return true;
    if (_Obj->GetScript<CBlockScript>() != nullptr)        return true;
    if (_Obj->GetScript<CBlockMovingScript>() != nullptr)  return true;
    if (_Obj->GetScript<CBlockPushingScript>() != nullptr) return true;

    return false;
}

CCoinScript::CCoinScript()
	: CScript(SCRIPT_TYPE::COINSCRIPT)
    , m_bgravity(false)
{
}

CCoinScript::~CCoinScript()
{
}

void CCoinScript::Begin()
{
    ADD_DYNAMIC_BEGIN_OVERLAP(CCoinScript::BeginOverlap);
    ADD_DYNAMIC_OVERLAP(CCoinScript::Overlap);
    ADD_DYNAMIC_END_OVERLAP(CCoinScript::EndOverlap);

    m_LifeTime = 0.f;

    if (Collider2D())
    {
        Collider2D()->SetOffset(Vec2(0.f, 0.0f));
        Collider2D()->SetScale(Vec2(0.9f, 0.9f));
    }

    if (FlipbookRender())
    {
        if (0 == FlipbookRender()->GetFlipbookCount() || nullptr == FlipbookRender()->GetFlipbook(0))
        {
            FlipbookRender()->SetFlipbook(0, LOAD(AFlipbook, L"Flipbook\\CoinTurn.flip"));
        }

        FlipbookRender()->Play(0, 10.f, -1);
    }
}

bool CCoinScript::CanPickupByPlayer() const
{
    if (!m_bgravity)
        return true;

    return m_LifeTime >= kKnockbackCoinPickupDelay;
}

void CCoinScript::PickupByPlayer()
{
    if (GetOwner() != nullptr && GetOwner()->IsDead())
        return;

    PlayGameSFX(L"Sound\\코인 먹을때.wav", 0.8f, true);

    GameObject* pEffect = new GameObject;
    pEffect->SetName(L"CoinGetEffect");
    pEffect->AddComponent(new CTransform);
    pEffect->AddComponent(new CFlipbookRender);
    pEffect->AddComponent(new CEffectScript);

    pEffect->Transform()->SetRelativePos(Transform()->GetRelativePos());
    pEffect->Transform()->SetRelativeScale(Transform()->GetRelativeScale());

    Ptr<AFlipbook> pGetAnim = LOAD(AFlipbook, L"Flipbook\\CoinGet.flip");
    if (nullptr != pGetAnim)
    {
        pEffect->FlipbookRender()->SetFlipbook(0, pGetAnim);
        pEffect->FlipbookRender()->Play(0, 14.f, 2);
    }

    LevelMgr::GetInst()->GetCurLevel()->AddObject(5, pEffect);

    CCoinMgrScript::AddCoin((m_CoinValue > 0) ? m_CoinValue : 1);
    Destroy();
}

void CCoinScript::Tick()
{
    if (m_bgravity == true)
    {
        m_LifeTime += DT;

        if (m_LifeTime >= kKnockbackCoinLifeTime)
        {
            Destroy();
            return;
        }

        if (FlipbookRender())
        {
            if (m_LifeTime >= kKnockbackCoinBlinkStartTime)
            {
                const float blinkTime = m_LifeTime - kKnockbackCoinBlinkStartTime;
                const int blinkStep = (int)(blinkTime / kKnockbackCoinBlinkInterval);
                FlipbookRender()->SetVisible((blinkStep % 2) == 0);
            }
            else
            {
                FlipbookRender()->SetVisible(true);
            }
        }

        ++m_LineGroundProbeFrame;
        if (m_LineGroundProbeFrame >= 3)
        {
            m_LineGroundProbeFrame = 0;
            m_bCachedLineGround = ResolveLineGround();
        }

        const bool groundedByLine = m_bCachedLineGround;
        const bool groundedByBlockOrTile = (m_PhysicalGroundOverlapCount > 0);

        m_IsGround = groundedByLine || groundedByBlockOrTile;

        if (!m_IsGround)
            m_Velocity.y -= m_Gravity * DT;
        else if (m_Velocity.y < 0.f)
            m_Velocity.y = 0.f;

        Vec3 pos = Transform()->GetRelativePos();
        pos.x += m_Velocity.x * DT;
        pos.y += m_Velocity.y * DT;
        Transform()->SetRelativePos(pos);
    }
    else if (FlipbookRender())
    {
        FlipbookRender()->SetVisible(true);
    }
}



void CCoinScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
        return;

    GameObject* pOther = _OtherCollider->GetOwner();

    if (pOther->GetName() == L"Player")
    {
        if (CanPickupByPlayer())
            PickupByPlayer();
    }
    else
    {
        if (m_bgravity == true)
        {
            if (CountsAsCoinPhysicalGround(pOther))
                ++m_PhysicalGroundOverlapCount;
        }
    }
}

void CCoinScript::Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
        return;

    if (_OtherCollider->GetOwner()->GetName() != L"Player")
        return;

    if (CanPickupByPlayer())
        PickupByPlayer();
}


void CCoinScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (m_bgravity == true)
    {
        if (_OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
            return;

        GameObject* pOther = _OtherCollider->GetOwner();

        if (pOther->GetName() != L"Player")
        {
            if (CountsAsCoinPhysicalGround(pOther))
            {
                --m_PhysicalGroundOverlapCount;
                if (m_PhysicalGroundOverlapCount < 0)
                    m_PhysicalGroundOverlapCount = 0;
            }
        }

    }
}

bool CCoinScript::ResolveLineGround()
{
    const Vec3 pos3 = Transform()->GetRelativePos();
    const Vec3 scale3 = Transform()->GetRelativeScale();

    const float radius = min(fabsf(scale3.x), fabsf(scale3.y)) * 0.5f;
    const Vec2 center = Vec2(pos3.x, pos3.y);
    const Vec2 foot = center + Vec2(0.f, -radius);

    std::vector<GameObject*> nearby;
    CSurfaceScript::QueryNearbySurfaceObjects(
        foot - Vec2(radius + 16.f, radius + 16.f),
        foot + Vec2(radius + 16.f, radius + 16.f),
        nearby,
        8.f,
        false);

    bool found = false;
    float bestAbsDist = 999999.f;
    Vec2 bestNormal = Vec2(0.f, 1.f);
    float bestSignedDist = 0.f;

    for (GameObject* pObj : nearby)
    {
        auto pSurface = pObj->GetScript<CSurfaceScript>();
        if (pSurface == nullptr)
            continue;

        if (pSurface->GetGeometry() != CSurfaceScript::SURFACE_GEOMETRY::LINE)
            continue;

        if (pSurface->GetRole() != CSurfaceScript::SURFACE_ROLE::SURFACE)
            continue;

        Vec2 a = {};
        Vec2 b = {};
        pSurface->GetWorldEndpoints(a, b);

        Vec2 ab = b - a;
        const float abLenSq = LengthSq(ab);
        if (abLenSq <= 0.0001f)
            continue;

        const float t = Clamp01(Dot2(foot - a, ab) / abLenSq);
        const Vec2 closest = a + ab * t;

        Vec2 tangent = NormalizeSafe(ab, Vec2(1.f, 0.f));
        Vec2 normal(-tangent.y, tangent.x);
        if (normal.y < 0.f)
            normal = normal * -1.f;

        if (normal.y < 0.5f)
            continue;

        const float signedDist = Dot2(foot - closest, normal);

        const bool nearSurface = (signedDist <= 8.f && signedDist >= -6.f);
        const bool fallingOrRest = (m_Velocity.y <= 0.f);

        if (!nearSurface || !fallingOrRest)
            continue;

        const float absDist = fabsf(signedDist);
        if (absDist < bestAbsDist)
        {
            bestAbsDist = absDist;
            bestNormal = normal;
            bestSignedDist = signedDist;
            found = true;
        }
    }

    if (!found)
        return false;

    Vec3 pos = Transform()->GetRelativePos();
    pos.x -= bestNormal.x * bestSignedDist;
    pos.y -= bestNormal.y * bestSignedDist;
    Transform()->SetRelativePos(pos);

    if (Dot2(m_Velocity, bestNormal) < 0.f)
    {
        const float intoSurface = Dot2(m_Velocity, bestNormal);
        m_Velocity -= bestNormal * intoSurface;
    }

    return true;
}
