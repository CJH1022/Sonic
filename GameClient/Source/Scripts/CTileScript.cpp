#include "pch.h"
#include "CTileScript.h"

#include "KeyMgr.h"
#include "TimeMgr.h"
#include "RenderMgr.h"
#include "CTransform.h"
#include "LevelMgr.h"
#include "GameObject.h"
#include "CCollider2D.h"
#include "TaskMgr.h"
#include "AssetMgr.h"
#include "CTileRender.h"
#include "CPlayerScript.h"

CTileScript::CTileScript()
    : CScript(SCRIPT_TYPE::TILESCRIPT)
{
}

CTileScript::~CTileScript()
{
}

void CTileScript::Begin()
{
    if (Collider2D())
    {
        Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CTileScript::BeginOverlap);
        Collider2D()->AddDynamicEndOverlap(this, (COLLISION_EVENT)&CTileScript::EndOverlap);
        Collider2D()->AddDynamicOverlap(this, (COLLISION_EVENT)&CTileScript::Overlap);
    }
}

void CTileScript::Tick()
{
    DbgInfo info = {};
    info.Color = Vec4(0.f, 1.f, 0.f, 1.f);
    info.DepthTest = false;
    info.Life = 0.f;

    Vec3 vWorldPos = Transform()->GetWorldPos();
    Vec3 vWorldScale = Transform()->GetRelativeScale();

    if (m_eType == TILETYPE::CIRCLE_BLOCK_1 || m_eType == TILETYPE::CIRCLE_BLOCK_2)
    {
        info.Shape = DBG_SHAPE::CIRCLE;
        info.Pos = Vec3(
            vWorldPos.x + (center_x - 0.5f) * vWorldScale.x,
            vWorldPos.y - (center_y - 0.5f) * vWorldScale.y,
            vWorldPos.z
        );
        info.Scale = Vec3(r * vWorldScale.x * 2.f, r * vWorldScale.y * 2.f, 1.f);
    }
    else
    {
        info.Shape = DBG_SHAPE::RECT;
        info.Pos = vWorldPos;
        info.Scale = vWorldScale;
    }

    RenderMgr::GetInst()->AddDebugInfo(info);
}

void CTileScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    Overlap(_OwnCollider, _OtherCollider);

    auto pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    if (m_eType == TILETYPE::CIRCLE_BLOCK_3)
    {
        Vec3 vPlayerPos = _OtherCollider->GetOwner()->Transform()->GetWorldPos();
        Vec3 vTilePos = Transform()->GetWorldPos();
        Vec3 vTileScale = Transform()->GetRelativeScale();
  
        float LeftX = vTilePos.x - vTileScale.x * 0.5f;
        if (vPlayerPos.x < LeftX)
        {
            if (s_bHalfChecker == true)
            {
                s_bHalfChecker = false;
                RefreshHalfCheckerTiles();
            }
        }
    }

    if (m_eType == TILETYPE::CIRCLE_BLOCK_4)
    {
        Vec3 vPlayerPos = _OtherCollider->GetOwner()->Transform()->GetWorldPos();
        Vec3 vTilePos = Transform()->GetWorldPos();
        Vec3 vTileScale = Transform()->GetRelativeScale();
     
        float RightX = vTilePos.x + vTileScale.x * 0.5f;

        if (vPlayerPos.x > RightX)
        {
            if (s_bHalfChecker == false)
            {
                s_bHalfChecker = true;
                RefreshHalfCheckerTiles();
            }
        }
    }

    if (m_eType == TILETYPE::CIRCLE_BLOCK_1)
    {
        if (s_bHalfChecker == false)
        {
            s_bHalfChecker = true;
            RefreshHalfCheckerTiles();
        }
    }
    else if (m_eType == TILETYPE::CIRCLE_BLOCK_2)
    {
        if (s_bHalfChecker == true)
        {
            s_bHalfChecker = false;
            RefreshHalfCheckerTiles();
        }
    }
        
}

void CTileScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    auto pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    if (m_eType == TILETYPE::CIRCLE_BLOCK_2 && pPlayer->GetIsGround() == true)
    {
        Vec3 vPlayerPos = _OtherCollider->GetOwner()->Transform()->GetWorldPos();
        Vec3 vTilePos = Transform()->GetWorldPos();
        Vec3 vTileScale = Transform()->GetRelativeScale();

        // CIRCLE_BLOCK_2의 좌측 경계(= 원 중앙 세로 분할선)를 기준으로
        // 왼쪽으로 빠져나간 경우에는 half-check 상태를 유지한다.
        float splitX = vTilePos.x - vTileScale.x * 0.5f;
        bool bExitToRight = (vPlayerPos.x >= splitX);

        bool bNextHalfChecker = bExitToRight ? false : true;
        if (s_bHalfChecker != bNextHalfChecker)
        {
            s_bHalfChecker = bNextHalfChecker;
            RefreshHalfCheckerTiles();
        }
    }
}

void CTileScript::RefreshHalfCheckerTiles()
{
    Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();
    if (pCurLevel == nullptr)
        return;

    for (int layerIdx = 0; layerIdx < MAX_LAYER; ++layerIdx)
    {
        const vector<Ptr<GameObject>>& vecObjects = pCurLevel->GetLayer(layerIdx)->GetAllObjects();
        for (const Ptr<GameObject>& pObject : vecObjects)
        {
            if (pObject == nullptr)
                continue;

            auto pTileScript = pObject->GetScript<CTileScript>();
            if (pTileScript == nullptr)
                continue;

            if (pTileScript->m_eType != TILETYPE::CIRCLE_BLOCK_3 &&
                pTileScript->m_eType != TILETYPE::CIRCLE_BLOCK_4)
            {
                continue;
            }

            pTileScript->TileMapSetting(pTileScript->m_eType);
        }
    }
}

void CTileScript::Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OtherCollider->GetOwner()->GetName() != L"Player")
        return;

    auto pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    const bool wasGround = pPlayer->GetIsGround();
    GetTileState();

    Vec3 vPlayerPos = _OtherCollider->GetOwner()->Transform()->GetWorldPos();
    Vec3 vPlayerScale = _OtherCollider->GetOwner()->Transform()->GetRelativeScale();

    Vec3 vTilePos = Transform()->GetWorldPos();
    Vec3 vTileScale = Transform()->GetRelativeScale();

    Vec2 vFootPos = Vec2(vPlayerPos.x, vPlayerPos.y);

    if (wasGround)
    {
        float fRotZ = _OtherCollider->GetOwner()->Transform()->GetRelativeRot().z;
        Vec2 vLocalDown = Vec2(sinf(fRotZ), -cosf(fRotZ));
        vFootPos += vLocalDown * (vPlayerScale.y * 0.5f);
    }
    else
    {
        // 낙하 중에는 회전된 사각형 모서리 대신 원형 하단점을 사용해 착지 판정 안정화
        float halfW = fabsf(vPlayerScale.x) * 0.5f;
        float halfH = fabsf(vPlayerScale.y) * 0.5f;
        float radius = (halfW < halfH) ? halfW : halfH;
        vFootPos.y -= radius;
    }

    float left = vTilePos.x - vTileScale.x * 0.5f;
    float top = vTilePos.y + vTileScale.y * 0.5f;

    Vec2 localPos = {};
    localPos.x = (vFootPos.x - left) / vTileScale.x;
    localPos.y = (top - vFootPos.y) / vTileScale.y;

    const float edgeEpsilon = 0.02f;
    if (localPos.x < -edgeEpsilon || localPos.x > 1.f + edgeEpsilon ||
        localPos.y < -edgeEpsilon || localPos.y > 1.f + edgeEpsilon)
    {
        return;
    }

    if (localPos.x < 0.f) localPos.x = 0.f;
    else if (localPos.x > 1.f) localPos.x = 1.f;
    if (localPos.y < 0.f) localPos.y = 0.f;
    else if (localPos.y > 1.f) localPos.y = 1.f;

    float fValue = GetFvalue(localPos);
    const float detachThreshold = wasGround ? 0.02f : 0.005f;
    const float transitionSnapThreshold = wasGround ? 0.08f : detachThreshold;
    if (fValue > transitionSnapThreshold)
    {
        // 경계면에서 타일 간 콜백 순서로 지면 판정이 깜빡이는 현상 방지
        return;
    }
    const bool bTransitionSurface = (fValue > detachThreshold);

    Vec2 normalLocal = {};
    float mag = 0.f;
    GetNormal(localPos, normalLocal, mag);

    if (mag <= 0.001f)
    {
        return;
    }

    Vec2 worldNormal = Vec2(
        normalLocal.x * vTileScale.x,
        -normalLocal.y * vTileScale.y
    );

    float worldNormalLen = sqrtf(worldNormal.x * worldNormal.x + worldNormal.y * worldNormal.y);
    if (worldNormalLen <= 0.001f)
        return;

    worldNormal.x /= worldNormalLen;
    worldNormal.y /= worldNormalLen;

    // 1. 보간된 법선 결정 (경계면 지터 완화)
    Vec2 vOldNormal = pPlayer->GetNormal();
    if (fabsf(vOldNormal.x) < 0.0001f && fabsf(vOldNormal.y) < 0.0001f)
        vOldNormal = worldNormal;
    else
        vOldNormal.Normalize();

    float normalDot = vOldNormal.x * worldNormal.x + vOldNormal.y * worldNormal.y;
    if (normalDot > 1.f) normalDot = 1.f;
    if (normalDot < -1.f) normalDot = -1.f;

    float normalBlend = wasGround ? 0.2f : 1.f;
    if (m_TileState == TILESTATE::CIRCLE_BLOCK && wasGround)
        normalBlend = 0.3f;
    if (wasGround)
    {
        if (normalDot < 0.75f)
            normalBlend = 1.f;
        else if (normalDot < 0.9f && normalBlend < 0.6f)
            normalBlend = 0.6f;
    }
    if (bTransitionSurface)
        normalBlend = 1.f;

    vCurNormal = Lerp(vOldNormal, worldNormal, normalBlend);
    if (fabsf(vCurNormal.x) < 0.0001f && fabsf(vCurNormal.y) < 0.0001f)
        vCurNormal = worldNormal;
    else
        vCurNormal.Normalize();

    pPlayer->SetNormal(vCurNormal);

    // 2. [중요] Push 값의 월드 단위 변환
    // localNormal의 magnitude(mag)는 로컬 기준이므로, 
    // 실제 밀어낼 거리(World Distance)는 타일 스케일을 반영해야 합니다.
    float worldPush = 0.001f;
    if (fValue < 0.f)
    {
        float push = (-fValue / mag);

        // 타일의 평균 스케일을 곱해 로컬 push를 월드 push로 변환합니다.
        // (정밀도를 위해 해당 방향의 스케일 성분을 고려하는 것이 좋음)
        worldPush = push * ((vTileScale.x + vTileScale.y) * 0.5f) + 0.001f;
        const float maxPush = 12.f;
        if (worldPush > maxPush)
            worldPush = maxPush;

        Vec3 vPos = _OtherCollider->GetOwner()->Transform()->GetRelativePos();

        // 3. 보간된 방향 * 월드 거리로 보정
        vPos.x += vCurNormal.x * worldPush;
        vPos.y += vCurNormal.y * worldPush;

        _OtherCollider->GetOwner()->Transform()->SetRelativePos(vPos);
    }

    Vec2 tangent = Vec2(vCurNormal.y, -vCurNormal.x);

    Vec2 v = pPlayer->GetVelocity();

    float rawVn = v.x * vCurNormal.x + v.y * vCurNormal.y;
    const float incomingVn = rawVn;
    float vt = v.x * tangent.x + v.y * tangent.y;

    const bool isCircleTile = (m_TileState == TILESTATE::CIRCLE_BLOCK);
    const float breakDropSpeedThreshold = 100.f;
    const bool blockedByWallLikeTile =
        pPlayer->IsBreakOrRollAction() &&
        (fabsf(vCurNormal.y) < 0.35f) &&
        (fabsf(incomingVn) > 20.f);

    const bool bBreakCircleDrop =
        isCircleTile &&
        pPlayer->GetAction() == ActionState::Break &&
        (fabsf(vCurNormal.y) < 0.35f) &&
        (pPlayer->GetBreakSpeed() < breakDropSpeedThreshold);
    if (bBreakCircleDrop)
    {
        Vec2 vDrop = pPlayer->GetVelocity();
        vDrop.x = 0.f;
        pPlayer->SetVelocity(vDrop);
        pPlayer->SetIsGround(false);
        pPlayer->StopBlockedAction();
        return;
    }

    if (blockedByWallLikeTile && pPlayer->GetAction() == ActionState::Break)
    {
        pPlayer->RequestBreakWallStop();

        pPlayer->SetVelocity(Vec2(0.f, 0.f));
        return;
    }

    if (rawVn < 0.f)
    {
        v.x -= vCurNormal.x * rawVn;
        v.y -= vCurNormal.y * rawVn;
        rawVn = 0.f;
    }

    // 위쪽/옆면 속도가 있을 때 + 히스테리시스
    const float stickMinSpeed = 500.f;
    const float normalEnterY = 0.65f;
    const float normalKeepY = 0.45f;
    const float vnEnterTolerance = 5.f;
    const float vnKeepTolerance = 80.f;
    const float normalThreshold = wasGround ? normalKeepY : normalEnterY;
    const float vnTolerance = wasGround ? vnKeepTolerance : vnEnterTolerance;

    bool bSpeedStick = (fabsf(vt) > stickMinSpeed) && (vCurNormal.y > 0.2f);
    bool bCanStick =
        (rawVn <= vnTolerance) &&
        (vCurNormal.y > normalThreshold || bSpeedStick);

    if (bTransitionSurface && vCurNormal.y < normalThreshold)
        bCanStick = false;

    if (blockedByWallLikeTile)
    {
        if (pPlayer->GetAction() == ActionState::Break)
            pPlayer->RequestBreakWallStop();
        else
            pPlayer->StopBlockedAction();

        if (pPlayer->GetAction() == ActionState::Break)
        {
            Vec2 v = pPlayer->GetVelocity();
            v.x = 0.f;
            pPlayer->SetVelocity(v);
            return;
        }
    }

    if (bCanStick)
    {
        pPlayer->SetIsGround(true);
        pPlayer->SetNormal(vCurNormal);
        pPlayer->SetGroundTangent(tangent);

        // 바닥 마찰
        if (vCurNormal.y > 0.f)
        {
            float dt = DT;
            float friction = 100.f;

            if (vt > 0.f)
            {
                vt -= friction * dt;
                if (vt < 0.f) vt = 0.f;
            }
            else if (vt < 0.f)
            {
                vt += friction * dt;
                if (vt > 0.f) vt = 0.f;
            }
        }

        // Ground에서는 접선 성분만 유지
        v = tangent * vt;
  
    }
    else
    {
        const bool definiteAirborne =
            (rawVn > vnKeepTolerance) ||
            (vCurNormal.y < 0.2f && fabsf(vt) < stickMinSpeed);

        if (definiteAirborne)
            pPlayer->SetIsGround(false);

        pPlayer->SetNormal(vCurNormal);
        pPlayer->SetGroundTangent(tangent);

        // 옆/윗부분에서 속도 부족으로 떨어질 때 위로 튀지 않게 보정
        if (vCurNormal.y <= 0.65f && fabsf(vt) < stickMinSpeed)
        {
            if (v.y > 0.f)
            {
                v.y = 0.f;
            } 
        }
    }

    pPlayer->SetVelocity(v);
}

void CTileScript::TileMapSetting(TILETYPE i)
{
    m_eType = i;

    switch (i)
    {
    case TILETYPE::EMPTY_BLOCK:
        f = [](float x, float y) { return 1.0f; };
        dfdx = [](float x, float y) { return 1.f; };
        dfdy = [](float x, float y) { return 1.f; };
        break;

    case TILETYPE::LINE_BLOCK_1:
        a = 0.1f; b = 0.4f; c = -1.0f;
        f = [this](float x, float y) { return c * (y - (b - a) * x - a); };
        dfdx = [this](float x, float y) { return c * (a - b); };
        dfdy = [this](float x, float y) { return c; };
        break;

    case TILETYPE::LINE_BLOCK_2:
        a = 0.4f; b = 0.8f; c = -1.f;
        f = [this](float x, float y) { return c * (y - (b - a) * x - a); };
        dfdx = [this](float x, float y) { return c * (a - b); };
        dfdy = [this](float x, float y) { return c; };
        break;

    case TILETYPE::LINE_BLOCK_3:
        a = 0.8f; b = 0.8f; c = -1.f;
        f = [this](float x, float y) { return c * (y - (b - a) * x - a); };
        dfdx = [this](float x, float y) { return c * (a - b); };
        dfdy = [this](float x, float y) { return c; };
        break;

    case TILETYPE::LINE_BLOCK_4:
        a = 0.8f; b = 0.4f; c = -1.f;
        f = [this](float x, float y) { return c * (y - (b - a) * x - a); };
        dfdx = [this](float x, float y) { return c * (a - b); };
        dfdy = [this](float x, float y) { return c; };
        break;

    case TILETYPE::LINE_BLOCK_5:
        a = 0.4f; b = 0.1f; c = -1.f;
        f = [this](float x, float y) { return c * (y - (b - a) * x - a); };
        dfdx = [this](float x, float y) { return c * (a - b); };
        dfdy = [this](float x, float y) { return c; };
        break;

    case TILETYPE::LINE_BLOCK_6:
        a = 0.1f; b = 0.1f; c = -1.f;
        f = [this](float x, float y) { return c * (y - (b - a) * x - a); };
        dfdx = [this](float x, float y) { return c * (a - b); };
        dfdy = [this](float x, float y) { return c; };
        break;

    case TILETYPE::CIRCLE_BLOCK_1:
        r = 0.8f; center_x = 1.f; center_y = 1.f;
        f = [this](float x, float y)
            {
                return r * r - (x - center_x) * (x - center_x) - (y - center_y) * (y - center_y);
            };
        dfdx = [this](float x, float y) { return -2.f * (x - center_x); };
        dfdy = [this](float x, float y) { return -2.f * (y - center_y); };
        break;

    case TILETYPE::CIRCLE_BLOCK_2:
        r = 0.8f; center_x = 0.f; center_y = 1.f;
        f = [this](float x, float y)
            {
                return r * r - (x - center_x) * (x - center_x) - (y - center_y) * (y - center_y);
            };
        dfdx = [this](float x, float y) { return -2.f * (x - center_x); };
        dfdy = [this](float x, float y) { return -2.f * (y - center_y); };
        break;

    case TILETYPE::CIRCLE_BLOCK_3:
        // 하프 체커와 닿지 않았을 경우
        if (s_bHalfChecker == false)
        {
            a = 0.8f; b = 0.8f; c = -1.f;
            f = [this](float x, float y) { return c * (y - (b - a) * x - a); };
            dfdx = [this](float x, float y) { return c * (a - b); };
            dfdy = [this](float x, float y) { return c; };
            break;
        }
        else
        {
            r = 0.8f; center_x = 1.f; center_y = 0.f;
            f = [this](float x, float y)
                {
                    return r * r - (x - center_x) * (x - center_x) - (y - center_y) * (y - center_y);
                };
            dfdx = [this](float x, float y) { return -2.f * (x - center_x); };
            dfdy = [this](float x, float y) { return -2.f * (y - center_y); };
            break;
        }


    case TILETYPE::CIRCLE_BLOCK_4:
        // 하프 체커와 닿지 않았을 경우
        if (s_bHalfChecker == true)
        {
            a = 0.8f; b = 0.8f; c = -1.f;
            f = [this](float x, float y) { return c * (y - (b - a) * x - a); };
            dfdx = [this](float x, float y) { return c * (a - b); };
            dfdy = [this](float x, float y) { return c; };
            break;
        }
        else
        {
            r = 0.8f; center_x = 0.f; center_y = 0.f; // 
            f = [this](float x, float y)
                {
                    return r * r - (x - center_x) * (x - center_x) - (y - center_y) * (y - center_y);
                };
            dfdx = [this](float x, float y) { return -2.f * (x - center_x); };
            dfdy = [this](float x, float y) { return -2.f * (y - center_y); };
            break;
        }

    default:
        f = [](float x, float y) { return -1.0f; };
        dfdx = [](float x, float y) { return 0.f; };
        dfdy = [](float x, float y) { return 0.f; };
        break;
    }
}

float CTileScript::GetFvalue(Vec2 _pos)
{
    if (!f)
        return -1.f;

    return f(_pos.x, _pos.y);
}

void CTileScript::GetNormal(Vec2 _pos, Vec2& _normal, float& _mag)
{
    if (!dfdx || !dfdy)
    {
        _normal = Vec2(0.f, 0.f);
        _mag = 0.f;
        return;
    }

    float dx = dfdx(_pos.x, _pos.y);
    float dy = dfdy(_pos.x, _pos.y);

    _mag = sqrtf(dx * dx + dy * dy);

    if (_mag > 0.f)
    {
        _normal.x = dx / _mag;
        _normal.y = dy / _mag;
    }
    else
    {
        _normal = Vec2(0.f, 0.f);
    }
}
