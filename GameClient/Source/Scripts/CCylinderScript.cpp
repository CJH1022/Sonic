#include "pch.h"
#include "CCylinderScript.h"

#include "AssetMgr.h"
#include "CCollider2D.h"
#include "CSpriteRender.h"
#include "CTransform.h"
#include "GameObject.h"
#include "KeyMgr.h"
#include "RenderMgr.h"
#include "TimeMgr.h"
#include "func.h"
#include "Source/Scripts/CPlayerScript.h"

#include <cmath>

namespace
{
    constexpr float kCylinderClimbSpeed = 420.f;
    constexpr float kCylinderBounceSpeed = 620.f;
    constexpr float kCylinderEntryMaxDownSpeed = 140.f;
    constexpr float kCylinderFirstBounceMinSpeed = 260.f;
    constexpr float kCylinderBounceSpeedCap = 760.f;
    constexpr float kCylinderWallRideStartSpeedRatio = 0.82f;
    constexpr float kCylinderExitHeightRatio = 0.82f;
    constexpr float kCylinderWallInset = 18.f;
    constexpr float kCylinderInternalFallAccel = 1800.f;
    constexpr float kCylinderInternalFallMaxSpeed = 840.f;
    constexpr float kCylinderResolveEpsilon = 0.01f;
    constexpr float kCylinderMinHalfExtent = 8.f;
    constexpr float kCylinderDoorDefaultWidth = 54.f;
    constexpr float kCylinderDoorDefaultHeight = 72.f;
    constexpr float kCylinderDoorMinHalfWidth = 16.f;
    constexpr float kCylinderDoorMinHalfHeight = 20.f;
    constexpr float kCylinderDoorEdgeInset = 6.f;
    constexpr float kCylinderEntryDoorApproachPaddingX = 20.f;
    constexpr float kCylinderEntryDoorApproachPaddingY = 24.f;
    constexpr float kCylinderDebugDoorZBias = 0.2f;
    constexpr unsigned int kCylinderScriptDataMagic = 0x4344594Cu;
    constexpr unsigned int kCylinderScriptDataVersion = 2u;
    const wchar_t* kCylinderOutsideSpriteKey = L"Sprite\\Sonic_CylinderTree.sprite";
    const wchar_t* kCylinderInsideSpriteKey = L"Sprite\\Sonic_CylinderTree_1.sprite";
    const Vec2  kCylinderExitVelocity = Vec2(420.f, 320.f);
    const Vec2  kCylinderFallVelocity = Vec2(0.f, -220.f);

    struct Rect2D
    {
        Vec2 Center;
        Vec2 HalfExtent;
    };

    float Clamp01(float _Value)
    {
        if (_Value < 0.f)
            return 0.f;
        if (_Value > 1.f)
            return 1.f;
        return _Value;
    }

    float Maxf(float _A, float _B)
    {
        return (_A >= _B) ? _A : _B;
    }

    float Minf(float _A, float _B)
    {
        return (_A <= _B) ? _A : _B;
    }

    Rect2D MakeRect(const Vec2& _Center, float _HalfWidth, float _HalfHeight)
    {
        Rect2D rect = {};
        rect.Center = _Center;
        rect.HalfExtent = Vec2(_HalfWidth, _HalfHeight);
        return rect;
    }

    Rect2D InflateRect(const Rect2D& _Rect, float _PadX, float _PadY)
    {
        return MakeRect(_Rect.Center,
                        _Rect.HalfExtent.x + Maxf(0.f, _PadX),
                        _Rect.HalfExtent.y + Maxf(0.f, _PadY));
    }

    bool Overlaps(const Rect2D& _A, const Rect2D& _B)
    {
        return fabsf(_A.Center.x - _B.Center.x) <= (_A.HalfExtent.x + _B.HalfExtent.x)
            && fabsf(_A.Center.y - _B.Center.y) <= (_A.HalfExtent.y + _B.HalfExtent.y);
    }

    Rect2D MakeCylinderDoorRect(const Vec2& _CylinderCenter,
                                float _HalfWidth,
                                float _HalfHeight,
                                const Vec2& _DoorSize,
                                const Vec2& _DoorOffset,
                                int _SideSign,
                                bool _EntryDoor);

    bool IsNearCylinderEntryDoor(const Rect2D& _PlayerRect,
                                 const Vec2& _CylinderCenter,
                                 float _HalfWidth,
                                 float _HalfHeight,
                                 const Vec2& _EntryDoorSize,
                                 const Vec2& _EntryDoorOffset)
    {
        const Rect2D leftEntryDoor = InflateRect(
            MakeCylinderDoorRect(_CylinderCenter, _HalfWidth, _HalfHeight, _EntryDoorSize, _EntryDoorOffset, -1, true),
            kCylinderEntryDoorApproachPaddingX,
            kCylinderEntryDoorApproachPaddingY);
        const Rect2D rightEntryDoor = InflateRect(
            MakeCylinderDoorRect(_CylinderCenter, _HalfWidth, _HalfHeight, _EntryDoorSize, _EntryDoorOffset, 1, true),
            kCylinderEntryDoorApproachPaddingX,
            kCylinderEntryDoorApproachPaddingY);

        return Overlaps(_PlayerRect, leftEntryDoor) || Overlaps(_PlayerRect, rightEntryDoor);
    }

    Rect2D MakeCylinderDoorRect(const Vec2& _CylinderCenter,
                                float _HalfWidth,
                                float _HalfHeight,
                                const Vec2& _DoorSize,
                                const Vec2& _DoorOffset,
                                int _SideSign,
                                bool _EntryDoor)
    {
        const float requestedDoorHalfWidth = fabsf(_DoorSize.x) * 0.5f;
        const float requestedDoorHalfHeight = fabsf(_DoorSize.y) * 0.5f;
        const float rawDoorHalfWidth = Maxf(kCylinderDoorMinHalfWidth, requestedDoorHalfWidth);
        const float rawDoorHalfHeight = Maxf(kCylinderDoorMinHalfHeight, requestedDoorHalfHeight);

        const float doorHalfWidth = Minf(rawDoorHalfWidth, Maxf(kCylinderMinHalfExtent, _HalfWidth - kCylinderDoorEdgeInset));
        const float doorHalfHeight = Minf(rawDoorHalfHeight, Maxf(kCylinderMinHalfExtent, _HalfHeight * 0.55f));

        const float sideX = _CylinderCenter.x + (float)_SideSign * (_HalfWidth - doorHalfWidth - kCylinderDoorEdgeInset) + _DoorOffset.x;
        const float bottomY = _CylinderCenter.y - _HalfHeight;
        const float topY = _CylinderCenter.y + _HalfHeight;
        const float doorY = (_EntryDoor ? bottomY : topY) + _DoorOffset.y;
        return MakeRect(Vec2(sideX, doorY), doorHalfWidth, doorHalfHeight);
    }

    void DrawDoorRectDebug(const Rect2D& _Rect, float _Z, const Vec4& _Color)
    {
        DrawDebugRect(Vec3(_Rect.Center.x, _Rect.Center.y, _Z),
                      Vec3(_Rect.HalfExtent.x * 2.f, _Rect.HalfExtent.y * 2.f, 1.f),
                      Vec3(0.f, 0.f, 0.f),
                      _Color,
                      0.f,
                      false);
    }

    void DrawCylinderDoorDebug(const Vec2& _CylinderCenter,
                               float _HalfWidth,
                               float _HalfHeight,
                               const Vec2& _EntryDoorSize,
                               const Vec2& _EntryDoorOffset,
                               const Vec2& _ExitDoorSize,
                               const Vec2& _ExitDoorOffset,
                               float _Z)
    {
        if (!RenderMgr::GetInst()->IsDebugRender())
            return;

        const float pulse = 0.55f + 0.45f * (0.5f + 0.5f * sinf(TIME * 8.f));
        const Rect2D entryLeft = MakeCylinderDoorRect(_CylinderCenter, _HalfWidth, _HalfHeight,
                                                      _EntryDoorSize, _EntryDoorOffset, -1, true);
        const Rect2D entryRight = MakeCylinderDoorRect(_CylinderCenter, _HalfWidth, _HalfHeight,
                                                       _EntryDoorSize, _EntryDoorOffset, 1, true);
        const Rect2D exitLeft = MakeCylinderDoorRect(_CylinderCenter, _HalfWidth, _HalfHeight,
                                                     _ExitDoorSize, _ExitDoorOffset, -1, false);
        const Rect2D exitRight = MakeCylinderDoorRect(_CylinderCenter, _HalfWidth, _HalfHeight,
                                                      _ExitDoorSize, _ExitDoorOffset, 1, false);

        DrawDoorRectDebug(entryLeft, _Z, Vec4(0.15f, 0.95f, 1.f, pulse));
        DrawDoorRectDebug(entryRight, _Z, Vec4(0.15f, 0.95f, 1.f, pulse));
        DrawDoorRectDebug(exitLeft, _Z, Vec4(1.f, 0.65f, 0.15f, pulse));
        DrawDoorRectDebug(exitRight, _Z, Vec4(1.f, 0.65f, 0.15f, pulse));

        DrawDebugRect(Vec3(entryLeft.Center.x, entryLeft.Center.y, _Z),
                      Vec3(10.f, 10.f, 1.f),
                      Vec3(0.f, 0.f, 0.f),
                      Vec4(0.15f, 0.95f, 1.f, 1.f),
                      0.f,
                      false);
        DrawDebugRect(Vec3(entryRight.Center.x, entryRight.Center.y, _Z),
                      Vec3(10.f, 10.f, 1.f),
                      Vec3(0.f, 0.f, 0.f),
                      Vec4(0.15f, 0.95f, 1.f, 1.f),
                      0.f,
                      false);
        DrawDebugRect(Vec3(exitLeft.Center.x, exitLeft.Center.y, _Z),
                      Vec3(10.f, 10.f, 1.f),
                      Vec3(0.f, 0.f, 0.f),
                      Vec4(1.f, 0.65f, 0.15f, 1.f),
                      0.f,
                      false);
        DrawDebugRect(Vec3(exitRight.Center.x, exitRight.Center.y, _Z),
                      Vec3(10.f, 10.f, 1.f),
                      Vec3(0.f, 0.f, 0.f),
                      Vec4(1.f, 0.65f, 0.15f, 1.f),
                      0.f,
                      false);
    }

    void SetCylinderSpriteVisual(GameObject* _CylinderObject, bool _Inside)
    {
        if (_CylinderObject == nullptr || _CylinderObject->SpriteRender() == nullptr)
            return;

        static Ptr<ASprite> s_OutsideSprite = LOAD(ASprite, kCylinderOutsideSpriteKey);
        static Ptr<ASprite> s_InsideSprite = LOAD(ASprite, kCylinderInsideSpriteKey);

        Ptr<ASprite> pTargetSprite = _Inside ? s_InsideSprite : s_OutsideSprite;
        if (pTargetSprite == nullptr)
            return;

        if (_CylinderObject->SpriteRender()->GetSprite() == pTargetSprite)
            return;

        _CylinderObject->SpriteRender()->SetSprite(pTargetSprite);
    }

}

CCylinderScript::CCylinderScript()
    : CScript(SCRIPT_TYPE::CYLINDERSCRIPT)
    , fTheta(0.f)
    , m_fRideSpeed(0.f)
    , bAttachTree(false)
    , m_bPlayerInside(false)
    , m_bBounceStarted(false)
    , m_bFallingInside(false)
    , m_SpiralDir(1)
    , m_EntryDoorSize(Vec2(kCylinderDoorDefaultWidth, kCylinderDoorDefaultHeight))
    , m_EntryDoorOffset(Vec2(0.f, 0.f))
    , m_ExitDoorSize(Vec2(kCylinderDoorDefaultWidth, kCylinderDoorDefaultHeight))
    , m_ExitDoorOffset(Vec2(0.f, 0.f))
    , m_pRidingPlayer(nullptr)
{
    RegisterScriptParams();
}

CCylinderScript::CCylinderScript(const CCylinderScript& _Origin)
    : CScript(_Origin)
    , fTheta(0.f)
    , m_fRideSpeed(0.f)
    , bAttachTree(false)
    , m_bPlayerInside(false)
    , m_bBounceStarted(false)
    , m_bFallingInside(false)
    , m_SpiralDir(1)
    , m_EntryDoorSize(_Origin.m_EntryDoorSize)
    , m_EntryDoorOffset(_Origin.m_EntryDoorOffset)
    , m_ExitDoorSize(_Origin.m_ExitDoorSize)
    , m_ExitDoorOffset(_Origin.m_ExitDoorOffset)
    , m_pRidingPlayer(nullptr)
{
    RegisterScriptParams();
}

CCylinderScript::~CCylinderScript()
{
}

void CCylinderScript::RegisterScriptParams()
{
    AddScriptParam(SCRIPT_PARAM::VEC2, &m_EntryDoorSize, L"Cylinder Entry Size", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &m_EntryDoorOffset, L"Cylinder Entry Offset", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &m_ExitDoorSize, L"Cylinder Exit Size", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &m_ExitDoorOffset, L"Cylinder Exit Offset", false, 1.f);
}

void CCylinderScript::Begin()
{
    if (Collider2D() != nullptr)
    {
        Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CCylinderScript::BeginOverlap);
        Collider2D()->AddDynamicOverlap(this, (COLLISION_EVENT)&CCylinderScript::Overlap);
        Collider2D()->AddDynamicEndOverlap(this, (COLLISION_EVENT)&CCylinderScript::EndOverlap);
    }

    SetCylinderSpriteVisual(GetOwner(), false);
}

void CCylinderScript::Tick()
{
    Vec2 cylinderCenter = {};
    float halfWidth = 0.f;
    float halfHeight = 0.f;
    if (!GetCylinderWorldRect(cylinderCenter, halfWidth, halfHeight))
        return;

    const float debugZ = (GetOwner() != nullptr && GetOwner()->Transform() != nullptr)
        ? (GetOwner()->Transform()->GetWorldPos().z + kCylinderDebugDoorZBias)
        : kCylinderDebugDoorZBias;
    DrawCylinderDoorDebug(cylinderCenter, halfWidth, halfHeight,
                          m_EntryDoorSize,
                          m_EntryDoorOffset,
                          m_ExitDoorSize,
                          m_ExitDoorOffset,
                          debugZ);

    if (!m_bPlayerInside || m_pRidingPlayer == nullptr)
        return;

    if (m_pRidingPlayer->IsDead())
    {
        ResetRideState(true);
        return;
    }

    Ptr<CPlayerScript> pPlayer = m_pRidingPlayer->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
    {
        ResetRideState(true);
        return;
    }

    UpdateRide(pPlayer.Get(), cylinderCenter, halfWidth, halfHeight);
}

void CCylinderScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    Overlap(_OwnCollider, _OtherCollider);
}

void CCylinderScript::Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OwnCollider == nullptr || _OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
        return;

    Ptr<CPlayerScript> pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    Vec2 cylinderCenter = {};
    float halfWidth = 0.f;
    float halfHeight = 0.f;
    Vec2 playerCenter = {};
    float playerHalfWidth = 0.f;
    float playerHalfHeight = 0.f;
    if (!GetCylinderWorldRect(cylinderCenter, halfWidth, halfHeight) ||
        !GetOtherColliderWorldRect(_OtherCollider, playerCenter, playerHalfWidth, playerHalfHeight))
    {
        return;
    }

    if (!m_bPlayerInside)
    {
        const Rect2D playerRect = MakeRect(playerCenter, playerHalfWidth, playerHalfHeight);

        if (TryBeginRide(pPlayer.Get(), playerCenter, playerHalfWidth, playerHalfHeight,
                         cylinderCenter, halfWidth, halfHeight))
        {
            UpdateRide(pPlayer.Get(), cylinderCenter, halfWidth, halfHeight);
            return;
        }

        if (IsNearCylinderEntryDoor(playerRect, cylinderCenter, halfWidth, halfHeight, m_EntryDoorSize, m_EntryDoorOffset))
            return;

        PossibleAttatchTreeWall(_OtherCollider);
        return;
    }

    if (m_pRidingPlayer == nullptr)
        m_pRidingPlayer = _OtherCollider->GetOwner();

    if (m_pRidingPlayer != _OtherCollider->GetOwner())
        return;

    UpdateRide(pPlayer.Get(), cylinderCenter, halfWidth, halfHeight);
}

void CCylinderScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    UNREFERENCED_PARAMETER(_OwnCollider);
    UNREFERENCED_PARAMETER(_OtherCollider);
}

bool CCylinderScript::GetCylinderWorldRect(Vec2& _OutCenter, float& _OutHalfWidth, float& _OutHalfHeight)
{
    if (GetOwner() == nullptr || GetOwner()->Transform() == nullptr)
        return false;

    Vec3 worldPos = GetOwner()->Transform()->GetWorldPos();
    Vec3 worldScale = GetOwner()->Transform()->GetWorldScale();
    Vec2 colliderOffset = Vec2(0.f, 0.f);
    Vec2 colliderScale = Vec2(1.f, 1.f);

    if (Collider2D() != nullptr)
    {
        colliderOffset = Collider2D()->GetOffset();
        colliderScale = Collider2D()->GetScale();
    }

    _OutCenter = Vec2(worldPos.x + colliderOffset.x, worldPos.y + colliderOffset.y);
    _OutHalfWidth = max(kCylinderMinHalfExtent, fabsf(worldScale.x * colliderScale.x) * 0.5f);
    _OutHalfHeight = max(kCylinderMinHalfExtent, fabsf(worldScale.y * colliderScale.y) * 0.5f);
    return true;
}

bool CCylinderScript::GetOtherColliderWorldRect(CCollider2D* _OtherCollider, Vec2& _OutCenter,
                                                float& _OutHalfWidth, float& _OutHalfHeight) const
{
    if (_OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
        return false;

    Vec3 worldPos = _OtherCollider->GetOwner()->Transform()->GetWorldPos();
    Vec3 worldScale = _OtherCollider->GetOwner()->Transform()->GetWorldScale();
    Vec2 colliderOffset = _OtherCollider->GetOffset();
    Vec2 colliderScale = _OtherCollider->GetScale();

    _OutCenter = Vec2(worldPos.x + colliderOffset.x, worldPos.y + colliderOffset.y);
    _OutHalfWidth = fabsf(worldScale.x * colliderScale.x) * 0.5f;
    _OutHalfHeight = fabsf(worldScale.y * colliderScale.y) * 0.5f;
    return true;
}

bool CCylinderScript::TryBeginRide(CPlayerScript* _Player, const Vec2& _PlayerCenter,
                                   float _PlayerHalfWidth, float _PlayerHalfHeight,
                                   const Vec2& _CylinderCenter, float _HalfWidth, float _HalfHeight)
{
    if (_Player == nullptr || _Player->GetOwner() == nullptr)
        return false;

    const Vec2 velocity = _Player->GetVelocity();
    if (velocity.y < -kCylinderEntryMaxDownSpeed)
        return false;

    const Rect2D playerRect = MakeRect(_PlayerCenter, _PlayerHalfWidth, _PlayerHalfHeight);
    const Rect2D leftEntryDoor = InflateRect(
        MakeCylinderDoorRect(_CylinderCenter, _HalfWidth, _HalfHeight, m_EntryDoorSize, m_EntryDoorOffset, -1, true),
        kCylinderEntryDoorApproachPaddingX,
        kCylinderEntryDoorApproachPaddingY);
    const Rect2D rightEntryDoor = InflateRect(
        MakeCylinderDoorRect(_CylinderCenter, _HalfWidth, _HalfHeight, m_EntryDoorSize, m_EntryDoorOffset, 1, true),
        kCylinderEntryDoorApproachPaddingX,
        kCylinderEntryDoorApproachPaddingY);
    const bool isAtBottomLeftDoor = Overlaps(playerRect, leftEntryDoor);
    const bool isAtBottomRightDoor = Overlaps(playerRect, rightEntryDoor);

    if (!isAtBottomLeftDoor && !isAtBottomRightDoor)
        return false;

    m_bPlayerInside = true;
    bAttachTree = true;
    m_bBounceStarted = false;
    m_bFallingInside = false;
    m_pRidingPlayer = _Player->GetOwner();
    m_SpiralDir = isAtBottomLeftDoor ? 1 : -1;
    fTheta = 0.f;
    m_fRideSpeed = Minf(kCylinderBounceSpeedCap, fabsf(velocity.x));
    SetCylinderSpriteVisual(GetOwner(), true);

    _Player->SetIsGround(false);
    _Player->SetIsJump(false);

    Vec2 playerColliderOffset = Vec2(0.f, 0.f);
    if (_Player->GetOwner()->Collider2D() != nullptr)
        playerColliderOffset = _Player->GetOwner()->Collider2D()->GetOffset();

    Vec3 playerPos = _Player->GetOwner()->Transform()->GetRelativePos();
    const float insideBottomCenterY = _CylinderCenter.y - _HalfHeight + _PlayerHalfHeight + 4.f;
    playerPos.y = Maxf(playerPos.y, insideBottomCenterY - playerColliderOffset.y);
    _Player->GetOwner()->Transform()->SetRelativePos(playerPos);
    return true;
}

void CCylinderScript::UpdateRide(CPlayerScript* _Player, const Vec2& _CylinderCenter,
                                 float _HalfWidth, float _HalfHeight)
{
    if (_Player == nullptr || _Player->GetOwner() == nullptr)
        return;

    Vec3 playerPos3 = _Player->GetOwner()->Transform()->GetRelativePos();
    Vec2 localPos = Vec2(playerPos3.x - _CylinderCenter.x, playerPos3.y - _CylinderCenter.y);
    const bool leftHeld = KEY_PRESSED(KEY::LEFT);
    const bool rightHeld = KEY_PRESSED(KEY::RIGHT);
    const bool anyHorizontalHeld = leftHeld || rightHeld;
    Vec2 playerCenter = Vec2(playerPos3.x, playerPos3.y);
    float playerHalfWidth = 18.f;
    float playerHalfHeight = 24.f;
    Vec2 playerVelocity = _Player->GetVelocity();

    if (_Player->GetOwner()->Collider2D() != nullptr)
        GetOtherColliderWorldRect(_Player->GetOwner()->Collider2D().Get(), playerCenter, playerHalfWidth, playerHalfHeight);

    const float innerHalfWidth = max(kCylinderMinHalfExtent, _HalfWidth - kCylinderWallInset);
    const float openFloorLocalY = -_HalfHeight + playerHalfHeight + 4.f;
    const bool isAboveOpenFloor = localPos.y > openFloorLocalY + 1.f;

    if ((KEY_TAP(KEY::SPACE) || !anyHorizontalHeld) && (m_bBounceStarted || m_bFallingInside || isAboveOpenFloor))
    {
        m_bBounceStarted = false;
        m_bFallingInside = true;
        if (playerVelocity.y > 0.f)
            playerVelocity.y = 0.f;
    }

    const float currentHorizontalSpeed = fabsf(playerVelocity.x);
    if (currentHorizontalSpeed > 0.5f)
        m_fRideSpeed = Minf(kCylinderBounceSpeedCap, currentHorizontalSpeed);

    int inputDir = 0;
    if (rightHeld && !leftHeld)
        inputDir = 1;
    else if (leftHeld && !rightHeld)
        inputDir = -1;

    if (m_bFallingInside)
    {
        localPos.x += playerVelocity.x * DT;
        localPos.y += playerVelocity.y * DT;
        playerVelocity.y = Maxf(-kCylinderInternalFallMaxSpeed, playerVelocity.y - kCylinderInternalFallAccel * DT);

        if (localPos.x >= innerHalfWidth)
            localPos.x = innerHalfWidth;
        else if (localPos.x <= -innerHalfWidth)
            localPos.x = -innerHalfWidth;

        if (localPos.y <= openFloorLocalY)
        {
            localPos.y = openFloorLocalY;
            playerVelocity.y = 0.f;
            m_bFallingInside = false;
        }
    }
    else if (!m_bBounceStarted)
    {
        localPos.y = openFloorLocalY;
        localPos.x += playerVelocity.x * DT;

        if (inputDir > 0)
            _Player->SetFacing(1);
        else if (inputDir < 0)
            _Player->SetFacing(-1);

        if (localPos.x <= -innerHalfWidth)
        {
            localPos.x = -innerHalfWidth;
        }

        if (localPos.x >= innerHalfWidth)
        {
            localPos.x = innerHalfWidth;

            const float wallRideStartSpeed = Maxf(kCylinderFirstBounceMinSpeed,
                                                  _Player->GetMaxMoveSpeed() * kCylinderWallRideStartSpeedRatio);
            if (inputDir > 0 && m_fRideSpeed >= wallRideStartSpeed)
            {
                m_bBounceStarted = true;
                m_SpiralDir = -1;
            }
        }
    }
    else
    {
        localPos.x += (float)m_SpiralDir * kCylinderBounceSpeed * DT;

        if (localPos.x >= innerHalfWidth)
        {
            localPos.x = innerHalfWidth;
            m_SpiralDir = -1;
        }
        else if (localPos.x <= -innerHalfWidth)
        {
            localPos.x = -innerHalfWidth;
            m_SpiralDir = 1;
        }

        localPos.y += kCylinderClimbSpeed * DT;
    }

    const Vec2 nextPlayerCenter = Vec2(_CylinderCenter.x + localPos.x, _CylinderCenter.y + localPos.y);
    const Rect2D playerRect = MakeRect(nextPlayerCenter, playerHalfWidth, playerHalfHeight);
    const Rect2D leftExitDoor = MakeCylinderDoorRect(_CylinderCenter, _HalfWidth, _HalfHeight,
                                                     m_ExitDoorSize, m_ExitDoorOffset, -1, false);
    const Rect2D rightExitDoor = MakeCylinderDoorRect(_CylinderCenter, _HalfWidth, _HalfHeight,
                                                      m_ExitDoorSize, m_ExitDoorOffset, 1, false);
    const bool canUseLeftExitDoor = Overlaps(playerRect, leftExitDoor);
    const bool canUseRightExitDoor = Overlaps(playerRect, rightExitDoor);
    const float exitThresholdY = -_HalfHeight + (_HalfHeight * 2.f) * Clamp01(kCylinderExitHeightRatio);

    if (canUseLeftExitDoor || canUseRightExitDoor || localPos.y >= exitThresholdY)
    {
        const bool useLeftExit = canUseLeftExitDoor || (!canUseRightExitDoor && localPos.x < 0.f);
        const float exitDir = useLeftExit ? -1.f : 1.f;
        const Rect2D& exitDoor = useLeftExit ? leftExitDoor : rightExitDoor;
        playerPos3.x = exitDoor.Center.x;
        playerPos3.y = Maxf(playerPos3.y, exitDoor.Center.y);
        _Player->GetOwner()->Transform()->SetRelativePos(playerPos3);
        ReleasePlayer(_Player, Vec2(exitDir * kCylinderExitVelocity.x, kCylinderExitVelocity.y));
        return;
    }

    playerPos3.x = _CylinderCenter.x + localPos.x;
    playerPos3.y = _CylinderCenter.y + localPos.y;
    _Player->GetOwner()->Transform()->SetRelativePos(playerPos3);
    _Player->SetIsGround(false);
    _Player->SetIsJump(false);

    if (m_bFallingInside)
    {
        _Player->SetVelocity(playerVelocity);
    }
    else if (!m_bBounceStarted)
    {
        playerVelocity.y = 0.f;
        _Player->SetVelocity(playerVelocity);
    }
    else
    {
        _Player->SetVelocity(Vec2(0.f, 0.f));
    }

    fTheta += DT;
}

void CCylinderScript::ReleasePlayer(CPlayerScript* _Player, const Vec2& _Velocity)
{
    if (_Player != nullptr)
    {
        _Player->SetIsGround(false);
        _Player->SetIsJump(false);
        _Player->SetVelocity(_Velocity);
    }

    ResetRideState(true);
}

void CCylinderScript::ResetRideState(bool _RestoreOutsideSprite)
{
    m_bPlayerInside = false;
    m_bBounceStarted = false;
    m_bFallingInside = false;
    bAttachTree = false;
    m_fRideSpeed = 0.f;
    m_pRidingPlayer = nullptr;

    if (_RestoreOutsideSprite)
        SetCylinderSpriteVisual(GetOwner(), false);
}

void CCylinderScript::SaveToLevelFile(FILE* _File)
{
    fwrite(&kCylinderScriptDataMagic, sizeof(unsigned int), 1, _File);
    fwrite(&kCylinderScriptDataVersion, sizeof(unsigned int), 1, _File);
    fwrite(&m_EntryDoorSize, sizeof(Vec2), 1, _File);
    fwrite(&m_ExitDoorSize, sizeof(Vec2), 1, _File);
    fwrite(&m_EntryDoorOffset, sizeof(Vec2), 1, _File);
    fwrite(&m_ExitDoorOffset, sizeof(Vec2), 1, _File);
}

void CCylinderScript::LoadFromLevelFile(FILE* _File)
{
    const long startPos = ftell(_File);
    unsigned int magic = 0u;
    if (1 != fread(&magic, sizeof(unsigned int), 1, _File))
    {
        if (startPos >= 0)
            fseek(_File, startPos, SEEK_SET);
        return;
    }

    if (magic != kCylinderScriptDataMagic)
    {
        if (startPos >= 0)
            fseek(_File, startPos, SEEK_SET);
        return;
    }

    unsigned int version = 0u;
    fread(&version, sizeof(unsigned int), 1, _File);

    if (version >= 1u)
    {
        fread(&m_EntryDoorSize, sizeof(Vec2), 1, _File);
        fread(&m_ExitDoorSize, sizeof(Vec2), 1, _File);
    }

    if (version >= 2u)
    {
        fread(&m_EntryDoorOffset, sizeof(Vec2), 1, _File);
        fread(&m_ExitDoorOffset, sizeof(Vec2), 1, _File);
    }
}

void CCylinderScript::PossibleAttatchTreeWall(CCollider2D* _OtherCollider)
{
    if (Collider2D() == nullptr || _OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
        return;

    Ptr<CPlayerScript> pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    Vec2 cylinderCenter = {};
    float cylinderHalfWidth = 0.f;
    float cylinderHalfHeight = 0.f;
    Vec2 playerCenter = {};
    float playerHalfWidth = 0.f;
    float playerHalfHeight = 0.f;
    if (!GetCylinderWorldRect(cylinderCenter, cylinderHalfWidth, cylinderHalfHeight) ||
        !GetOtherColliderWorldRect(_OtherCollider, playerCenter, playerHalfWidth, playerHalfHeight))
    {
        return;
    }

    const float dx = playerCenter.x - cylinderCenter.x;
    const float dy = playerCenter.y - cylinderCenter.y;
    const float overlapX = (cylinderHalfWidth + playerHalfWidth) - fabsf(dx);
    const float overlapY = (cylinderHalfHeight + playerHalfHeight) - fabsf(dy);
    if (overlapX <= 0.f || overlapY <= 0.f)
        return;

    const Rect2D playerRect = MakeRect(playerCenter, playerHalfWidth, playerHalfHeight);
    if (IsNearCylinderEntryDoor(playerRect, cylinderCenter, cylinderHalfWidth, cylinderHalfHeight,
                                m_EntryDoorSize, m_EntryDoorOffset))
    {
        return;
    }

    Vec3 playerPos = pPlayer->Transform()->GetRelativePos();
    Vec2 playerVelocity = pPlayer->GetVelocity();

    if (overlapX < overlapY)
    {
        if (dx >= 0.f)
            playerPos.x += overlapX + kCylinderResolveEpsilon;
        else
            playerPos.x -= overlapX + kCylinderResolveEpsilon;

        playerVelocity.x = 0.f;
        pPlayer->StopBlockedAction();
    }
    else
    {
        if (dy >= 0.f)
        {
            playerPos.y += overlapY + kCylinderResolveEpsilon;
            pPlayer->ForceFlatGroundContact(0.12f);
            if (playerVelocity.y < 0.f)
                playerVelocity.y = 0.f;
        }
        else
        {
            playerPos.y -= overlapY + kCylinderResolveEpsilon;
            if (playerVelocity.y > 0.f)
                playerVelocity.y = 0.f;
        }
    }

    pPlayer->Transform()->SetRelativePos(playerPos);
    pPlayer->SetVelocity(playerVelocity);
}

Vec2 CCylinderScript::FallingTree()
{
    return kCylinderFallVelocity;
}
