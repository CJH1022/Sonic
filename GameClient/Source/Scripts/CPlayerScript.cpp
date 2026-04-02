#include "pch.h"
#include "CPlayerScript.h"

#include "KeyMgr.h"
#include "TimeMgr.h"
#include "RenderMgr.h"
#include "CTransform.h"
#include "LevelMgr.h"
#include "GameObject.h"
#include "CCollider2D.h"
#include "TaskMgr.h"
#include "CTileScript.h"
#include "CSurfaceCircleGuideScript.h"
#include "CSurfaceScript.h"
#include "CBlockScript.h"
#include "CBlockMovingScript.h"

#include <cmath> // fabsf
#include "CBlockPushingScript.h"
#include "AssetMgr.h"

namespace
{
    constexpr int kPushingFlipbookIndex = 20;
    constexpr int kKnockBackFlipbookIndex = 21;
    constexpr float kKnockBackBlinkInterval = 0.08f;
    constexpr float kKnockBackMinGroundRecoverTime = 0.25f;
    constexpr float kKnockBackMaxDuration = 0.45f;
    constexpr float kKnockBackGroundFriction = 8.f;
    constexpr float kKnockBackAirDrag = 0.75f;
    constexpr float kGroundRotationLerpSpeed = 14.f;
    constexpr float kSurfaceResolveFrameEpsilon = 0.0001f;
    constexpr float kSurfaceResolveScoreEpsilon = 0.01f;
    constexpr float kSurfaceLineSeamKeepDot = 0.90f;
    constexpr float kSurfaceLineSeamTransitionDepth = 0.03f;
    constexpr float kSurfaceLineSeamDownwardDelta = 0.025f;
    constexpr float kSurfaceLineSeamDirectionBias = 0.40f;
    constexpr float kSurfaceLineSeamTransitionScoreFavor = 0.30f;
    constexpr float kSurfaceLineSeamFlatToSlopePushLimit = 0.02f;
    constexpr float kSurfaceLineSeamAngleShiftDot = 0.90f;
    constexpr float kSurfaceLineSeamAngleShiftScorePenalty = 0.28f;
    constexpr float kSurfaceLineSeamAngleShiftPushLimit = 0.015f;
    constexpr float kSurfaceLineSeamEntryBlendFloor = 0.25f;
    constexpr float kSurfaceLineSeamEnterDownGradeBonus = 0.6f;
    constexpr float kSurfaceLineSeamMinY = 0.08f;
    constexpr float kSurfaceLineSeamMoveSpeedGate = 2200.f;
    constexpr float kSurfaceLineSeamHoldSpeedMin = 70.f;      // 낮은 속도에서 이전 접선 고착을 더 억제
    constexpr float kSurfaceLineSeamFlatToSlopeBlendFloor = 0.55f;
    constexpr float kSurfaceLineSeamAngleShiftBlendFloor = 0.60f;
    constexpr float kSurfaceLineSeamDownwardBlendFloor = 0.80f;
    constexpr float kSurfaceLineSeamSteepDownBlendFloor = 0.85f;
    constexpr float kSurfaceLineSeamRisingSlopeScoreFavor = 0.45f;
    constexpr float kSurfaceLineSeamRisingSlopeBlendFloor = 0.72f;
    constexpr float kSurfaceLineSeamRisingSlopePushLimit = 1.25f;
    constexpr float kSurfaceLineSeamRisingSlopeSnapMaxDistance = 10.f;
    constexpr float kSurfaceLineSeamRisingSlopeMoveMin = 5.f;

    constexpr float kSurfaceLineAttachSnapMaxDistance = 2.5f;  // line 표면은 현재/전이 라인일 때만 짧게 스냅한다
    constexpr float kSurfaceLineStableProjectionMaxDistance = 2.5f;
    constexpr float kSurfaceLinePenetrationPushMaxDistance = 1.5f;
    constexpr float kSurfaceContactPushBias = 0.015f;         // 미세 침투 복귀량을 더 낮춰 점프/튕김을 완화
    constexpr float kSurfaceContactMaxPush = 1.5f;
    constexpr float kSurfaceContactDownTransitionPushLimit = 0.08f;
    constexpr float kSurfaceStableProjectionMaxDistance = 6.f;
    constexpr float kSurfaceCircleProjectionMaxDistance = 18.f;
    constexpr float kSurfaceCircleNormalBlend = 0.82f;
    constexpr float kSurfaceCircleGroundHoldTime = 0.14f;
    constexpr float kSurfaceCircleStickMinY = 0.02f;
    constexpr float kSurfaceCircleStickSpeedMin = 40.f;
    constexpr float kSurfaceJumpDetachSpeedMin = 60.f;
    constexpr float kSurfaceInwardCircleAttachBlockTime = 1.20f;
constexpr float kSurfaceInwardCircleLoopReleaseAngle = XM_2PI - 0.35f;
constexpr float kSurfaceInwardCircleUpperHalfMin = 0.5f;
constexpr float kSurfaceInwardCircleMaxAngleStep = XM_PIDIV2;
constexpr float kSurfaceInwardCircleTopHalfSwitchEpsilon = 1.5f;
constexpr float kSurfaceInwardCircleHalfCheckerSwitchUpperHalfMin = 0.75f;
constexpr float kSurfaceInwardCircleLowSpeedDetachUpperHalfMin = 0.35f;
constexpr float kSurfaceInwardCircleLowSpeedDetachSpeedMin = 110.f;
constexpr float kSurfaceInwardCircleLowSpeedDetachMinAngle = 0.35f;
constexpr float kSurfaceInwardCircleLowSpeedDetachBlockTime = 0.22f;
constexpr float kSurfaceLineOverTopHalfInwardCircleDistanceMargin = 1.5f;
    constexpr float kAutoplaySampleInterval = 0.10f;
    constexpr float kAutoplayDefaultTotalDuration = 15.35f;
    constexpr float kAutoplaySurfaceTotalDuration = 9.20f;
    constexpr float kAutoplaySurfaceChordTotalDuration = 3.40f;
    constexpr float kAutoplaySurfaceJumpTotalDuration = 2.60f;
    constexpr float kAutoplaySurfaceTopJumpTotalDuration = 2.20f;
    constexpr float kAutoplaySurfaceSlowFallTotalDuration = 2.20f;
    constexpr float kAutoplaySurfaceLineUnderTotalDuration = 2.20f;

    float WrapAngleRad(float _Angle)
    {
        while (_Angle > XM_PI)
            _Angle -= XM_2PI;

        while (_Angle < -XM_PI)
            _Angle += XM_2PI;

        return _Angle;
    }

    float LerpAngleRad(float _Current, float _Target, float _Speed, float _Dt)
    {
        float delta = WrapAngleRad(_Target - _Current);
        float t = _Speed * _Dt;
        if (t > 1.f)
            t = 1.f;

        return WrapAngleRad(_Current + delta * t);
    }

    float Clamp01f(float _Value)
    {
        if (_Value < 0.f)
            return 0.f;
        if (_Value > 1.f)
            return 1.f;
        return _Value;
    }

    float NormalizePositiveAngleRad(float _Angle)
    {
        while (_Angle < 0.f)
            _Angle += XM_2PI;

        while (_Angle >= XM_2PI)
            _Angle -= XM_2PI;

        return _Angle;
    }

    float DeltaAngleCCW(float _From, float _To)
    {
        return NormalizePositiveAngleRad(_To - _From);
    }

    float DeltaAngleAlongOrientation(bool _CounterClockwise, float _From, float _To)
    {
        return _CounterClockwise ? DeltaAngleCCW(_From, _To) : DeltaAngleCCW(_To, _From);
    }

    float DegreesToRadians(float _AngleDeg)
    {
        return _AngleDeg * XM_PI / 180.f;
    }

    float ComputeLineSeamBlend(float _SeamT, bool _HasValue, bool _IsTransition,
                              const Vec2& _SeamStart, const Vec2& _SeamEnd, const Vec2& _ContactPoint,
                              float _MoveVelocityX)
    {
        if (!_HasValue || !_IsTransition)
            return 0.f;

        float blend = Clamp01f(_SeamT);
        if (_SeamEnd.x != _SeamStart.x || _SeamEnd.y != _SeamStart.y)
        {
            const float dx = _SeamEnd.x - _SeamStart.x;
            const float dy = _SeamEnd.y - _SeamStart.y;
            if (fabsf(dx) >= fabsf(dy))
            {
                if (fabsf(dx) > 0.0001f)
                    blend = Clamp01f((_ContactPoint.x - _SeamStart.x) / dx);
            }
            else if (fabsf(dy) > 0.0001f)
            {
                blend = Clamp01f((_ContactPoint.y - _SeamStart.y) / dy);
            }
        }

        if (_MoveVelocityX > 0.f)
            return blend;

        if (_MoveVelocityX < 0.f)
            return 1.f - blend;

        return 0.f;
    }

    float ComputeSeamDirectionBias(float _SeamT, bool _HasValue, bool _IsTransition, float _MoveVelocityX)
    {
        if (!_HasValue || !_IsTransition)
            return 0.f;

        if (_MoveVelocityX > 1.f)
            return Clamp01f(1.f - Clamp01f(_SeamT));

        if (_MoveVelocityX < -1.f)
            return Clamp01f(_SeamT);

        return 0.f;
    }

    float DotVec2(const Vec2& _A, const Vec2& _B)
    {
        return _A.x * _B.x + _A.y * _B.y;
    }

    float LengthVec2(const Vec2& _V)
    {
        return sqrtf(_V.x * _V.x + _V.y * _V.y);
    }

    Vec2 NormalizeSafeVec2(const Vec2& _V, const Vec2& _Fallback = Vec2(0.f, 1.f))
    {
        float len = LengthVec2(_V);
        if (len <= 0.0001f)
            return _Fallback;

        return Vec2(_V.x / len, _V.y / len);
    }

    bool IsAutoplayEnabled()
    {
        static const bool enabled = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay"));
        return enabled;
    }

    bool IsSurfaceAutoplayScenario()
    {
        static const bool enabled = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface"));
        return enabled;
    }

    bool IsSurfaceAutoplayLeftScenario()
    {
        static const bool enabled =
            (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_left")) ||
            (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_chord_left"));
        return enabled;
    }

    bool IsSurfaceAutoplayChordScenario()
    {
        static const bool enabled = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_chord"));
        return enabled;
    }

    bool IsSurfaceAutoplayJumpScenario()
    {
        static const bool enabled = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_jump"));
        return enabled;
    }

    bool IsSurfaceAutoplayTopJumpScenario()
    {
        static const bool enabled = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_top_jump"));
        return enabled;
    }

    bool IsSurfaceAutoplaySlowFallScenario()
    {
        static const bool enabled = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_slow_fall"));
        return enabled;
    }

    bool IsSurfaceAutoplayLineUnderScenario()
    {
        static const bool enabled = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_line_under"));
        return enabled;
    }

    float GetAutoplayTotalDuration()
    {
        if (IsSurfaceAutoplayTopJumpScenario())
            return kAutoplaySurfaceTopJumpTotalDuration;

        if (IsSurfaceAutoplaySlowFallScenario())
            return kAutoplaySurfaceSlowFallTotalDuration;

        if (IsSurfaceAutoplayLineUnderScenario())
            return kAutoplaySurfaceLineUnderTotalDuration;

        if (IsSurfaceAutoplayJumpScenario())
            return kAutoplaySurfaceJumpTotalDuration;

        if (IsSurfaceAutoplayChordScenario())
            return kAutoplaySurfaceChordTotalDuration;

        return IsSurfaceAutoplayScenario() ? kAutoplaySurfaceTotalDuration : kAutoplayDefaultTotalDuration;
    }

    const wchar_t* GetAutoplayPhaseName(float _Time)
    {
        if (IsSurfaceAutoplayScenario())
        {
            if (IsSurfaceAutoplayTopJumpScenario())
            {
                if (_Time < 0.14f)
                    return L"settle";

                if (_Time < 0.24f)
                    return L"top_jump";

                if (_Time < GetAutoplayTotalDuration())
                    return L"cooldown";

                return L"finish";
            }

            if (IsSurfaceAutoplaySlowFallScenario())
            {
                if (_Time < 0.35f)
                    return L"settle";

                if (_Time < 0.85f)
                    return L"slow_circle_fall";

                if (_Time < GetAutoplayTotalDuration())
                    return L"cooldown";

                return L"finish";
            }

            if (IsSurfaceAutoplayLineUnderScenario())
            {
                if (_Time < 0.35f)
                    return L"settle";

                if (_Time < 0.85f)
                    return L"line_under_fall";

                if (_Time < GetAutoplayTotalDuration())
                    return L"cooldown";

                return L"finish";
            }

            if (IsSurfaceAutoplayJumpScenario())
            {
                if (_Time < 0.35f)
                    return L"settle";

                if (_Time < 0.62f)
                    return IsSurfaceAutoplayLeftScenario() ? L"surface_run_left" : L"surface_run_right";

                if (_Time < 0.72f)
                    return L"jump";

                if (_Time < GetAutoplayTotalDuration())
                    return L"cooldown";

                return L"finish";
            }

            if (_Time < 0.35f)
                return L"settle";

            if (_Time < 8.30f)
                return IsSurfaceAutoplayLeftScenario() ? L"surface_run_left" : L"surface_run_right";

            if (_Time < GetAutoplayTotalDuration())
                return L"cooldown";

            return L"finish";
        }

        if (_Time < 0.35f)
            return L"settle";

        if (_Time < 7.35f)
            return L"run_right";

        if (_Time < 8.00f)
            return L"pause";

        if (_Time < 15.00f)
            return L"run_left";

        if (_Time < GetAutoplayTotalDuration())
            return L"cooldown";

        return L"finish";
    }

    wstring GetAutoplayLogPath()
    {
        wchar_t modulePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

        wstring logPath = modulePath;
        const size_t slashPos = logPath.find_last_of(L"\\/");
        if (slashPos != wstring::npos)
            logPath.erase(slashPos + 1);

        if (IsSurfaceAutoplayTopJumpScenario())
            logPath += L"autoplay_surface_top_jump_report.txt";
        else if (IsSurfaceAutoplaySlowFallScenario())
            logPath += L"autoplay_surface_slow_fall_report.txt";
        else if (IsSurfaceAutoplayLineUnderScenario())
            logPath += L"autoplay_surface_line_under_report.txt";
        else if (IsSurfaceAutoplayJumpScenario())
            logPath += L"autoplay_surface_jump_report.txt";
        else if (IsSurfaceAutoplayChordScenario() && IsSurfaceAutoplayLeftScenario())
            logPath += L"autoplay_surface_chord_left_report.txt";
        else if (IsSurfaceAutoplayChordScenario())
            logPath += L"autoplay_surface_chord_report.txt";
        else if (IsSurfaceAutoplayLeftScenario())
            logPath += L"autoplay_surface_left_report.txt";
        else if (IsSurfaceAutoplayScenario())
            logPath += L"autoplay_surface_suite_report.txt";
        else
            logPath += L"autoplay_surface_report.txt";
        return logPath;
    }

    void AppendSurfaceAutoplayDebug(const wchar_t* _Format, ...)
    {
        if (!IsSurfaceAutoplayScenario())
            return;

        wchar_t modulePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

        wstring logPath = modulePath;
        const size_t slashPos = logPath.find_last_of(L"\\/");
        if (slashPos != wstring::npos)
            logPath.erase(slashPos + 1);
        logPath += L"autoplay_surface_debug.txt";

        FILE* pTrace = nullptr;
        _wfopen_s(&pTrace, logPath.c_str(), L"a, ccs=UTF-8");
        if (nullptr == pTrace)
            return;

        va_list args;
        va_start(args, _Format);
        vfwprintf(pTrace, _Format, args);
        va_end(args);
        fwprintf(pTrace, L"\n");
        fclose(pTrace);
    }
}

CPlayerScript::CPlayerScript()
    : CScript(SCRIPT_TYPE::PLAYERSCRIPT)
{
    AddScriptParam(SCRIPT_PARAM::VEC2, &vAccel, L"Acceleration", false, 10.f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &vFraction, L"Friction", false, 0.1f);

    // 네 기존 초기값 스타일 유지
    m_Limit = 0.f;
    
    vAccel = Vec2(500.f, 1000.f);
    vVelocity = Vec2(30.f, 0.f);     // 원래 30이었으면 유지 (원치 않으면 0으로)
    vFraction = Vec2(5.f, 10.f);

    IsGround = false;
    bIsSpringJump = false;

    m_Pose = PoseState::None;
    m_Action = ActionState::None;

    m_IdleTime = 0.f;
    m_PoseTime = 0.f;
    m_ActionTime = 0.f;

    m_Facing = 1;
    m_TurnTargetFacing = 1;
}

void CPlayerScript::Begin()
{
    Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CPlayerScript::BeginOverlap);
    Collider2D()->AddDynamicOverlap(this, (COLLISION_EVENT)&CPlayerScript::Overlap);
    Collider2D()->AddDynamicEndOverlap(this, (COLLISION_EVENT)&CPlayerScript::EndOverlap);

    FlipbookRender()->SetFlipbook(kPushingFlipbookIndex, LOAD(AFlipbook, L"Flipbook\\Sonic_Pushing.flip"));
    FlipbookRender()->SetFlipbook(kKnockBackFlipbookIndex, LOAD(AFlipbook, L"Flipbook\\Sonic_Hurt.flip"));
    m_LastTickPosX = Transform()->GetRelativePos().x;
    m_LastFrameDeltaX = 0.f;
}

CPlayerScript::~CPlayerScript()
{
    if (nullptr != m_pAutoplayLog)
    {
        fclose(m_pAutoplayLog);
        m_pAutoplayLog = nullptr;
    }
}

CPlayerScript::PlayerInput CPlayerScript::ReadInput() const
{
    if (IsAutoplayEnabled())
        return m_AutoplayInput;

    PlayerInput in;
    // =======================================================
    // [수정됨] 넉백 상태일 때는 모든 키 입력을 무시하고 빈 입력 반환
    if (m_Action == ActionState::KnockBack)
    {
        return in; // 모든 멤버가 false인 상태로 반환됨
    }
    // =======================================================

    in.upHeld = KEY_PRESSED(KEY::UP);
    in.downHeld = KEY_PRESSED(KEY::DOWN);
    in.downReleased = KEY_RELEASED(KEY::DOWN);

    in.leftHeld = KEY_PRESSED(KEY::LEFT);
    in.rightHeld = KEY_PRESSED(KEY::RIGHT);
    in.leftTap = KEY_TAP(KEY::LEFT);
    in.rightTap = KEY_TAP(KEY::RIGHT);

    in.spacePressed = KEY_TAP(KEY::SPACE);
    in.spaceHeld = KEY_PRESSED(KEY::SPACE);
    in.spaceReleased = KEY_RELEASED(KEY::SPACE);

    // TODO: 공격/구르기 키는 네 프로젝트에 맞게 지정
    //in.attackPressed = KEY_TAP(KEY::Z);
    in.rollPressed = KEY_TAP(KEY::DOWN);

    return in;
}

void CPlayerScript::Tick()
{
    float dt = DT;
    if (IsAutoplayEnabled() && dt > (1.f / 60.f))
        dt = (1.f / 60.f);
    if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        AppendSurfaceAutoplayDebug(L"tick begin t=%.3f dt=%.6f", m_AutoplayTime, dt);
    InitAutoplay();
    UpdateAutoplay(dt);

    if (m_AttachBlockedTime > 0.f)
    {
        m_AttachBlockedTime -= dt;
        if (m_AttachBlockedTime <= 0.f)
        {
            m_AttachBlockedTime = 0.f;
            m_pAttachBlockedSurface = nullptr;
        }
    }

    if (m_InwardCircleAttachBlockTime > 0.f)
    {
        m_InwardCircleAttachBlockTime -= dt;
        if (m_InwardCircleAttachBlockTime <= 0.f)
        {
            m_InwardCircleAttachBlockTime = 0.f;
            m_InwardCircleAttachBlockCenter = Vec2(0.f, 0.f);
            m_InwardCircleAttachBlockRadius = 0.f;
        }
    }

    if (m_SurfaceGroundHoldTime > 0.f)
    {
        m_SurfaceGroundHoldTime -= dt;
        if (m_SurfaceGroundHoldTime < 0.f)
            m_SurfaceGroundHoldTime = 0.f;
    }

    if (m_KnockBackInvincibleTime > 0.f)
    {
        m_KnockBackInvincibleTime -= dt;
        if (m_KnockBackInvincibleTime < 0.f)
            m_KnockBackInvincibleTime = 0.f;
    }

    if (FlipbookRender() != nullptr)
    {
        bool bVisible = true;

        if (m_KnockBackInvincibleTime > 0.f)
        {
            const float blinkElapsed = kKnockBackInvincibleDuration - m_KnockBackInvincibleTime;
            const float blinkCycle = kKnockBackBlinkInterval * 2.f;
            const float blinkPhase = fmodf(blinkElapsed, blinkCycle);
            bVisible = (blinkPhase < kKnockBackBlinkInterval);
        }

        FlipbookRender()->SetVisible(bVisible);
    }

    ResolveBufferedSurfaceContacts();
    if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        AppendSurfaceAutoplayDebug(L"tick after_resolve1 t=%.3f ground=%d", m_AutoplayTime, IsGround ? 1 : 0);

    if (IsGround && m_TileOverlapCount == 0 && m_SurfaceGroundHoldTime <= 0.f && !m_bHasCurrentSurfaceContact)
        ResetGroundContact();

    const float curPosX = Transform()->GetRelativePos().x;
    m_LastFrameDeltaX = curPosX - m_LastTickPosX;
    m_LastTickPosX = curPosX;

    const PlayerInput in = ReadInput();

    // 1) 상태 전이(결정)
    ResolveTransitions(in, dt);

    // 2) 물리/이동(실행)
    Simulate(in, dt);
    if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        AppendSurfaceAutoplayDebug(L"tick after_simulate t=%.3f pos=(%.2f,%.2f) vel=(%.2f,%.2f)",
                                   m_AutoplayTime, Transform()->GetRelativePos().x, Transform()->GetRelativePos().y, vVelocity.x, vVelocity.y);

    // 이동 직후 현재 위치 기준으로 다시 surface를 잡아, 새 평지/경사 전환이
    // 다음 프레임까지 밀리지 않도록 한다.
    ResolveBufferedSurfaceContacts();
    if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        AppendSurfaceAutoplayDebug(L"tick after_resolve2 t=%.3f ground=%d", m_AutoplayTime, IsGround ? 1 : 0);

    // 3) 회전 상태 에니메이션 갱신
    UpdateGroundRotation();
    if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        AppendSurfaceAutoplayDebug(L"tick after_rotation t=%.3f rot=%.3f", m_AutoplayTime, Transform()->GetRelativeRot().z);

    // 4) 타이머 갱신 (속도/상태 기반)
    UpdateTimers(dt);

    // 5) 애니메이션(표현) - 한 곳에서 우선순위로만 결정
    UpdateAnimation(dt);
    if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        AppendSurfaceAutoplayDebug(L"tick end t=%.3f action=%d", m_AutoplayTime, (int)m_Action);

    // 디버그는 네 코드처럼 필요하면 여기서
    if (in.spacePressed)
    {
        DrawDebugRect(Transform()->GetWorldMat(), Vec4(1.f, 0.f, 0.f, 1.f), 2.f);
    }

    if (IsAutoplayEnabled() && m_bAutoplayInitialized && !m_bAutoplayFinished)
    {
        const Vec3 pos = Transform()->GetRelativePos();
        if (pos.x > m_AutoplayMaxX)
            m_AutoplayMaxX = pos.x;
        if (pos.y < m_AutoplayMinY)
            m_AutoplayMinY = pos.y;

        if (!IsGround)
        {
            m_AutoplayAirTime += dt;
            if (m_bAutoplayWasGround && m_AutoplayTime > 0.75f)
            {
                ++m_AutoplayGroundLossCount;
                LogAutoplayState(L"ground_lost");
            }
        }
        else
        {
            if (!m_bAutoplayWasGround && m_AutoplayAirTime > 0.f)
                LogAutoplayState(L"ground_regained");

            if (m_AutoplayAirTime > m_AutoplayLongestAirTime)
                m_AutoplayLongestAirTime = m_AutoplayAirTime;

            m_AutoplayAirTime = 0.f;
        }

        m_bAutoplayWasGround = IsGround;
        m_AutoplaySampleAccum += dt;
        if (m_AutoplaySampleAccum >= kAutoplaySampleInterval)
        {
            m_AutoplaySampleAccum = 0.f;
            LogAutoplayState(L"sample");
        }

        if (m_bAutoplayPendingJumpDetach && !IsGround)
        {
            ++m_AutoplayJumpDetachCount;
            m_bAutoplayPendingJumpDetach = false;
            LogAutoplayState(L"jump_detach");
        }

        if (m_bHasCurrentSurfaceContact && m_CurrentSurfaceContact.Surface != nullptr)
        {
            const wstring surfaceName = m_CurrentSurfaceContact.Surface->GetName();
            const float contactError = fabsf(m_CurrentSurfaceContact.SignedDistance);

            if (surfaceName.find(L"SurfaceLine") == 0)
            {
                if (contactError > m_AutoplayMaxLineContactError)
                    m_AutoplayMaxLineContactError = contactError;
            }
            else if (surfaceName.find(L"SurfaceCircle") == 0)
            {
                if (contactError > m_AutoplayMaxCircleContactError)
                    m_AutoplayMaxCircleContactError = contactError;

                if (IsGround)
                {
                    m_bAutoplayTouchedCircle = true;
                    m_AutoplayCircleGroundTime += dt;
                }
            }
        }

        if (m_AutoplayTime >= GetAutoplayTotalDuration())
            FinishAutoplay();
    }
}

void CPlayerScript::InitAutoplay()
{
    if (!IsAutoplayEnabled() || m_bAutoplayInitialized)
        return;

    m_bAutoplayInitialized = true;
    m_bAutoplayWasGround = IsGround;
    m_AutoplayMaxLineContactError = 0.f;
    m_AutoplayMaxCircleContactError = 0.f;
    m_AutoplayCircleGroundTime = 0.f;
    m_AutoplayInwardCircleReleaseCount = 0;
    m_AutoplayJumpStartCount = 0;
    m_AutoplayJumpDetachCount = 0;
    m_bAutoplayPendingJumpDetach = false;
    m_bAutoplayTouchedCircle = false;

    const Vec3 pos = Transform()->GetRelativePos();
    m_AutoplayMaxX = pos.x;
    m_AutoplayMinY = pos.y;

    const wstring logPath = GetAutoplayLogPath();
    _wfopen_s(&m_pAutoplayLog, logPath.c_str(), L"w, ccs=UTF-8");
    if (nullptr != m_pAutoplayLog)
    {
        fwprintf(m_pAutoplayLog, L"autoplay surface report\n");
        fwprintf(m_pAutoplayLog, L"log_path=%ls\n", logPath.c_str());
    }

    LogAutoplayState(L"begin");
}

void CPlayerScript::UpdateAutoplay(float dt)
{
    if (!IsAutoplayEnabled() || !m_bAutoplayInitialized || m_bAutoplayFinished)
        return;

    m_AutoplayTime += dt;
    m_AutoplayInput = {};

    if (IsSurfaceAutoplayScenario())
    {
        if (IsSurfaceAutoplayTopJumpScenario())
        {
            if (m_AutoplayTime < 0.14f)
                return;

            if (m_AutoplayTime < 0.24f)
            {
                m_AutoplayInput.spaceHeld = true;
                m_AutoplayInput.spacePressed = true;
                return;
            }

            return;
        }

        if (m_AutoplayTime < 0.35f)
            return;

        if (IsSurfaceAutoplaySlowFallScenario() || IsSurfaceAutoplayLineUnderScenario())
            return;

        if (IsSurfaceAutoplayJumpScenario())
        {
            if (m_AutoplayTime < 0.62f)
            {
                if (IsSurfaceAutoplayLeftScenario())
                    m_AutoplayInput.leftHeld = true;
                else
                    m_AutoplayInput.rightHeld = true;
                return;
            }

            if (m_AutoplayTime < 0.72f)
            {
                if (IsSurfaceAutoplayLeftScenario())
                    m_AutoplayInput.leftHeld = true;
                else
                    m_AutoplayInput.rightHeld = true;
                m_AutoplayInput.spaceHeld = true;
                m_AutoplayInput.spacePressed = true;
                return;
            }

            return;
        }

        if (m_AutoplayTime < 8.30f)
        {
            if (IsSurfaceAutoplayLeftScenario())
                m_AutoplayInput.leftHeld = true;
            else
                m_AutoplayInput.rightHeld = true;
            return;
        }

        return;
    }

    if (m_AutoplayTime < 0.35f)
        return;

    if (m_AutoplayTime < 7.35f)
    {
        m_AutoplayInput.rightHeld = true;
        return;
    }

    if (m_AutoplayTime < 8.00f)
        return;

    if (m_AutoplayTime < 15.00f)
    {
        m_AutoplayInput.leftHeld = true;
        return;
    }
}

void CPlayerScript::LogAutoplayState(const wchar_t* _Tag)
{
    if (nullptr == m_pAutoplayLog)
        return;

    const Vec3 pos = Transform()->GetRelativePos();
    const wchar_t* phase = GetAutoplayPhaseName(m_AutoplayTime);
    const wstring surfaceName = (m_bHasCurrentSurfaceContact && m_CurrentSurfaceContact.Surface)
        ? m_CurrentSurfaceContact.Surface->GetName()
        : L"-";
    const float contactError = m_bHasCurrentSurfaceContact ? m_CurrentSurfaceContact.SignedDistance : 0.f;

    fwprintf(m_pAutoplayLog,
             L"tag=%ls time=%.3f phase=%ls pos=(%.2f,%.2f) vel=(%.2f,%.2f) ground=%d normal=(%.3f,%.3f) tangent=(%.3f,%.3f) action=%d surface=%ls contact_error=%.3f\n",
             _Tag, m_AutoplayTime, phase, pos.x, pos.y, vVelocity.x, vVelocity.y, IsGround ? 1 : 0,
             vNormal.x, vNormal.y, vTangent.x, vTangent.y, static_cast<int>(m_Action), surfaceName.c_str(), contactError);
    fflush(m_pAutoplayLog);
}

void CPlayerScript::FinishAutoplay()
{
    if (!IsAutoplayEnabled() || m_bAutoplayFinished)
        return;

    m_bAutoplayFinished = true;

    if (m_AutoplayAirTime > m_AutoplayLongestAirTime)
        m_AutoplayLongestAirTime = m_AutoplayAirTime;

    LogAutoplayState(L"finish");

    if (nullptr != m_pAutoplayLog)
    {
        if (IsSurfaceAutoplayScenario())
        {
            fwprintf(m_pAutoplayLog,
                     L"summary max_x=%.2f min_y=%.2f ground_loss_count=%d longest_air=%.3f line_error_max=%.3f circle_error_max=%.3f circle_ground_time=%.3f touched_circle=%d circle_release_count=%d jump_start_count=%d jump_detach_count=%d\n",
                     m_AutoplayMaxX, m_AutoplayMinY, m_AutoplayGroundLossCount, m_AutoplayLongestAirTime,
                     m_AutoplayMaxLineContactError, m_AutoplayMaxCircleContactError,
                     m_AutoplayCircleGroundTime, m_bAutoplayTouchedCircle ? 1 : 0,
                     m_AutoplayInwardCircleReleaseCount, m_AutoplayJumpStartCount, m_AutoplayJumpDetachCount);
        }
        else
        {
            fwprintf(m_pAutoplayLog, L"summary max_x=%.2f min_y=%.2f ground_loss_count=%d longest_air=%.3f\n",
                     m_AutoplayMaxX, m_AutoplayMinY, m_AutoplayGroundLossCount, m_AutoplayLongestAirTime);
        }
        fclose(m_pAutoplayLog);
        m_pAutoplayLog = nullptr;
    }

    PostQuitMessage(0);
}

void CPlayerScript::ResolveTransitions(const PlayerInput& in, float dt)
{
    // 먼저 액션 업데이트(차징 중이면 release 감지 등)
    ResolveAction(in, dt);

    const bool wasPushing = (m_Action == ActionState::Pushing);
    const bool wantsLeft = in.leftHeld && !in.rightHeld;
    const bool wantsRight = in.rightHeld && !in.leftHeld;
    const int inputDir = wantsRight ? 1 : (wantsLeft ? -1 : 0);
    const bool canPush = IsGround && m_bPushContact && inputDir != 0 && m_PushContactDir == inputDir;

    if (canPush)
    {
        m_Action = ActionState::Pushing;
        m_bPushing = true;
        m_PushingDir = inputDir;
        m_Facing = inputDir;
        m_Pose = PoseState::None;
    }
    else if (m_Action == ActionState::Pushing)
    {
        m_Action = ActionState::None;
        m_bPushing = false;
        m_PushingDir = 0;
    }

    if (m_Action == ActionState::Break)
    {
        if (m_bBreakWallLocked &&
            ((m_iBreakDirection == 1 && wantsLeft) || (m_iBreakDirection == -1 && wantsRight)))
        {
            // 벽에서 정지된 브레이크 상태에서 반대 방향 입력이 오면
            // 즉시 방향을 바꿔서 벽 밀림을 빠져나갈 수 있도록 처리.
            m_bBreakWallLocked = false;
            m_iBreakDirection = -m_iBreakDirection;
            m_TurnTargetFacing = m_iBreakDirection;
            m_BreakUngroundedTime = 0.f;
        }

        if (IsGround)
        {
            m_BreakUngroundedTime = 0.f;
        }
        else
        {
            m_BreakUngroundedTime += dt;

            // 실제 점프/낙하가 확실하면 즉시 해제,
            // 경사 보정 중 지면 판정 끊김은 짧게 허용.
            const bool hardAirborne = IsJump || fabsf(vVelocity.y) > 120.f;
            if ((hardAirborne || m_BreakUngroundedTime > 0.08f) && !m_bBreakWallLocked)
            {
                m_Action = ActionState::None;
                m_BreakUngroundedTime = 0.f;
            }
        }
    }
    else
    {
        m_BreakUngroundedTime = 0.f;
    }

    if ((m_Action == ActionState::Roll) && IsGround && !m_bBackReaction)
    {
        const float blockedMove = fabsf(m_LastFrameDeltaX);
        const float currentSpeed = fabsf(vVelocity.Dot(vTangent));
        if (blockedMove < 0.5f && currentSpeed > 80.f)
        {
            StopBlockedAction();
            return;
        }
    }

    // 액션 중이면 자세는 강제 해제(원하는 규칙)
    if (m_Action != ActionState::None
        && m_Action != ActionState::SkillDash
        && m_Action != ActionState::Roll
        && m_Action != ActionState::Pushing)
    {
        m_Pose = PoseState::None;
        return;
    }

    // 액션이 없을 때만 자세(위/아래 보기) 가능
    ResolvePose(in, dt);

    // 점프 시작 조건
    if (CanStartJump(in))
    {
        StartJump();
        return;
    }

    if (canPush)
        return;

    // 스킬 차징 시작 조건:
    // - 지상
    // - 아래보기 자세
    // - 스페이스 누르고 있음
    if (IsGround && m_Pose == PoseState::LookDown && in.spaceHeld)
    {
        m_Action = ActionState::SkillCharge;
        m_ActionTime = 0.f;
        return;
    }

    // TODO: Attack/Roll 시작도 여기서 (액션 시작하면 Pose는 해제)
    // if (IsGround && in.attackPressed) { m_Action = ActionState::Attack; m_ActionTime = 0.f; m_Pose = PoseState::None; }
    const float moveSpeed = IsGround ? vVelocity.Dot(vTangent) : vVelocity.x;
    const float absXSpeed = fabsf(vVelocity.x);
    const float absMoveSpeed = fabsf(moveSpeed);
    const float speedForSign =
        (absMoveSpeed >= absXSpeed * 0.25f) ? moveSpeed : vVelocity.x;
    const float absSpeed = fabsf(speedForSign);
    float normalSpeed = 0.f;

    if (IsGround)
        normalSpeed = fabsf(vVelocity.Dot(vNormal));
    else
        normalSpeed = fabsf(vVelocity.y);

    float rollMinSpeed = 150.f;

    if (IsGround && in.rollPressed)
    {
        if (absSpeed >= rollMinSpeed && normalSpeed < 30.f)
        {
            Rolling();
            return;
        }
    }

    bool isOpposite = false;
    // 반대 방향 입력 감지
    if (IsGround)
    {
        isOpposite =
            (speedForSign > 0.f && wantsLeft) ||
            (speedForSign < 0.f && wantsRight);

        if (!isOpposite && fabsf(speedForSign) < 1.f)
            isOpposite = (wantsLeft && m_Facing == 1) || (wantsRight && m_Facing == -1);
    }
    else
    {
        isOpposite =
            (vVelocity.x > 0.f && wantsLeft) ||
            (vVelocity.x < 0.f && wantsRight);
    }

    // 브레이크 모션
    const float breakEntrySpeed = (absSpeed > absXSpeed) ? absSpeed : absXSpeed;

    if (IsGround &&
        m_Action == ActionState::None &&
        isOpposite &&
        breakEntrySpeed > 30.f)
    {
        m_Action = ActionState::Break;
        float breakStartSpeed = IsGround ? fabsf(vVelocity.Dot(vTangent)) : fabsf(vVelocity.x);
        if (breakStartSpeed < fabsf(vVelocity.x))
            breakStartSpeed = fabsf(vVelocity.x);

        m_fBreakSpeed = breakStartSpeed;
        m_iBreakDirection = (speedForSign >= 0.f) ? 1 : -1;
        m_bBreakWallLocked = false;
        m_BreakUngroundedTime = 0.f;
        m_TurnTargetFacing = wantsLeft ? -1 : 1;
        m_ActionTime = 0.f;
        return;
    }

    if (wasPushing && m_Action == ActionState::None && inputDir != 0 && absSpeed <= 250.f)
    {
        m_Facing = inputDir;
    }

    // 정지 상태 방향 전환
    if (IsGround && absSpeed < 5.f)
    {
        if (in.rightTap)      m_Facing = 1;
        else if (in.leftTap)  m_Facing = -1;
    }
}

void CPlayerScript::ResolvePose(const PlayerInput& in, float dt)
{
    // ✅ "보면서 달리기" 금지 규칙: 이동 입력이거나 속도가 크면 Pose 불가
    const bool wantsMove = in.leftHeld || in.rightHeld || in.leftTap || in.rightTap;
    const float absSpeed = IsGround ? fabsf(vVelocity.Dot(vTangent)) : fabsf(vVelocity.x);

    if (!IsGround || wantsMove || absSpeed > 15.f)
    {
        if (m_Pose != PoseState::None)
        {
            m_Pose = PoseState::None;
            m_PoseTime = 0.f;
        }
        return;
    }

    PoseState nextPose = PoseState::None;
    if (in.upHeld && in.downHeld)
    {
        nextPose = PoseState::None;
    }
    else if (in.upHeld) nextPose = PoseState::LookUp;
    else if (in.downHeld) nextPose = PoseState::LookDown;

    if (nextPose != m_Pose)
    {
        m_Pose = nextPose;
        m_PoseTime = 0.f; // 포즈 시작 연출용 타이머 리셋
    }
}

void CPlayerScript::ResolveAction(const PlayerInput& in, float dt)
{
    // 차징 중: 아래키 유지 + 스페이스 유지
    if (m_Action == ActionState::SkillCharge)
    {
        // 아래키를 떼면 대시 발동
        if (in.downReleased)
        {
            StartSkillDash();
            return;
        }

        // 스페이스를 떼면 대시 발동
        if (in.spaceReleased)
        {
            StartSkillDash();
            return;
        }

        // 스페이스를 그냥 안 누르게 되면 취소(원하면 유지로 바꿔도 됨)
        if (!in.spaceHeld)
        {
            StartSkillDash();
            m_Action = ActionState::SkillCharge;
            return;
        }

        return;
    }

    // SkillDash는 이동 시뮬레이션에서 속도 보고 종료 처리
    if (m_Action == ActionState::SkillDash)
        return;

    if (m_Action == ActionState::KnockBack)
    {
        const bool bRecoveredOnGround = IsGround && m_ActionTime >= kKnockBackMinGroundRecoverTime;
        const bool bTimedOut = m_ActionTime >= kKnockBackMaxDuration;

        if (bRecoveredOnGround || bTimedOut)
        {
            m_Action = ActionState::None;
            m_ActionTime = 0.f;
            bIsSpringJump = false;

            if (IsGround)
                IsJump = false;
        }
        return;
    }

    // TODO: Attack/Roll 종료는 m_ActionTime 또는 애니 이벤트로 처리
    // if (m_Action == ActionState::Attack) ...
    if (m_Action == ActionState::Roll)
    {
        if (!m_bBackReaction)
            return;

        const bool bRecoveredOnGround = IsGround;
        const bool bTimedOut = m_ActionTime >= kKnockBackMaxDuration;

        if (bRecoveredOnGround || bTimedOut)
        {
            m_Action = ActionState::None;
            m_ActionTime = 0.f;
            m_bBackReaction = false;
            bIsSpringJump = false;

            if (IsGround)
                IsJump = false;
        }
        return;
    }

    if (m_Action == ActionState::Pushing)
    {
        if (!IsGround)
        {
            m_bPushContact = false;
            m_PushContactDir = 0;
            m_bPushing = false;
            m_PushingDir = 0;
            m_Action = ActionState::None;
        }
        return;
    }
}

bool CPlayerScript::CanStartJump(const PlayerInput& in) const
{
    const bool surfaceCoyoteTime =
        (m_SurfaceGroundHoldTime > 0.f) &&
        ((m_bHasCurrentSurfaceContact && m_CurrentSurfaceContact.Attachable && !m_CurrentSurfaceContact.WallLike) ||
         (m_bHasPendingSurfaceContact && m_PendingSurfaceContact.Attachable && !m_PendingSurfaceContact.WallLike));
    if (!IsGround && !surfaceCoyoteTime) return false;

    // Dash는 예외로 허용
    if (m_bBackReaction)
        return false;

    if (m_Action != ActionState::None
        && m_Action != ActionState::SkillDash
        && m_Action != ActionState::Roll
        && m_Action != ActionState::Break
        && m_Action != ActionState::Pushing)
        return false;

    // ✅ 기존처럼: 아래보기(앉기) 상태에서는 스페이스가 점프가 아니라 스킬에 쓰이므로 점프 금지
    if (m_Pose == PoseState::LookDown) return false;

    return in.spacePressed;
}

bool CPlayerScript::IsSameInwardCircleLoop(const Vec2& _Center, float _Radius) const
{
    return (fabsf(m_InwardCircleLoopCenter.x - _Center.x) <= 0.5f)
        && (fabsf(m_InwardCircleLoopCenter.y - _Center.y) <= 0.5f)
        && (fabsf(m_InwardCircleLoopRadius - _Radius) <= 0.5f);
}

bool CPlayerScript::IsSameInwardCircleHalfChecker(const Vec2& _Center, float _Radius) const
{
    return (fabsf(m_InwardCircleHalfCheckerCenter.x - _Center.x) <= 0.5f)
        && (fabsf(m_InwardCircleHalfCheckerCenter.y - _Center.y) <= 0.5f)
        && (fabsf(m_InwardCircleHalfCheckerRadius - _Radius) <= 0.5f);
}

bool CPlayerScript::ShouldLineOwnTopRightInwardCircleHalf(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint) const
{
    if (m_bInwardCircleHalfCheckerTracked && IsSameInwardCircleHalfChecker(_Center, _Radius))
        return m_bInwardCircleLineOwnsTopRight;

    float travelX = 0.f;
    if (fabsf(vVelocity.x) > 1.f)
        travelX = vVelocity.x;
    else if (fabsf(m_LastFrameDeltaX) > 0.1f)
        travelX = m_LastFrameDeltaX;

    if (travelX > 0.f)
        return true;

    if (travelX < 0.f)
        return false;

    if (_ContactPoint.x < _Center.x - kSurfaceInwardCircleTopHalfSwitchEpsilon)
        return true;

    if (_ContactPoint.x > _Center.x + kSurfaceInwardCircleTopHalfSwitchEpsilon)
        return false;

    return (_ContactPoint.x >= _Center.x);
}

void CPlayerScript::UpdateInwardCircleHalfChecker(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint)
{
    if (!m_bInwardCircleHalfCheckerTracked || !IsSameInwardCircleHalfChecker(_Center, _Radius))
    {
        m_bInwardCircleHalfCheckerTracked = true;
        m_InwardCircleHalfCheckerCenter = _Center;
        m_InwardCircleHalfCheckerRadius = _Radius;
        m_bInwardCircleLineOwnsTopRight = ShouldLineOwnTopRightInwardCircleHalf(_Center, _Radius, _ContactPoint);
    }

    const float safeRadius = max(_Radius, 0.0001f);
    const float upperHalfRatio = (_Center.y - _ContactPoint.y) / safeRadius;
    if (upperHalfRatio < kSurfaceInwardCircleHalfCheckerSwitchUpperHalfMin)
        return;

    float travelX = 0.f;
    if (fabsf(vVelocity.x) > 1.f)
        travelX = vVelocity.x;
    else if (fabsf(m_LastFrameDeltaX) > 0.1f)
        travelX = m_LastFrameDeltaX;

    if (travelX > 0.f && _ContactPoint.x >= _Center.x + kSurfaceInwardCircleTopHalfSwitchEpsilon)
    {
        m_bInwardCircleLineOwnsTopRight = false;
    }
    else if (travelX < 0.f && _ContactPoint.x <= _Center.x - kSurfaceInwardCircleTopHalfSwitchEpsilon)
    {
        m_bInwardCircleLineOwnsTopRight = true;
    }
}

void CPlayerScript::PrimeInwardCircleHalfCheckerFromLineContext(const SurfaceContact& _Contact)
{
    if (_Contact.Circle || _Contact.WallLike || !_Contact.Attachable)
        return;

    const auto primeFromCircleSurface = [&](GameObject* _SurfaceObject)
    {
        if (_SurfaceObject == nullptr)
            return false;

        auto pSurface = _SurfaceObject->GetScript<CSurfaceScript>();
        if (pSurface == nullptr ||
            pSurface->GetGeometry() != CSurfaceScript::SURFACE_GEOMETRY::CIRCLE ||
            !pSurface->GetFillInside())
        {
            return false;
        }

        const CSurfaceScript::ARC_CORNER corner = pSurface->GetArcCorner();
        if (corner != CSurfaceScript::ARC_CORNER::TOP_LEFT &&
            corner != CSurfaceScript::ARC_CORNER::TOP_RIGHT)
        {
            return false;
        }

        Vec2 center = {};
        float radius = 0.f;
        Vec2 boxMin = {};
        Vec2 boxMax = {};
        pSurface->GetArcWorldData(center, radius, boxMin, boxMax);
        UpdateInwardCircleHalfChecker(center, radius, _Contact.ContactPoint);
        return true;
    };

    if (m_bHasCurrentSurfaceContact &&
        m_CurrentSurfaceContact.Circle &&
        m_CurrentSurfaceContact.InwardCircle &&
        primeFromCircleSurface(m_CurrentSurfaceContact.Surface))
    {
        return;
    }

    if (m_bHasPendingSurfaceContact &&
        m_PendingSurfaceContact.Circle &&
        m_PendingSurfaceContact.InwardCircle &&
        primeFromCircleSurface(m_PendingSurfaceContact.Surface))
    {
        return;
    }

    for (GameObject* pSurfaceObject : m_vecActiveSurfaceObjects)
    {
        if (primeFromCircleSurface(pSurfaceObject))
            return;
    }
}

void CPlayerScript::NoteInwardCircleLoopProgress(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint)
{
    const Vec2 radial = _ContactPoint - _Center;
    if (fabsf(radial.x) <= 0.0001f && fabsf(radial.y) <= 0.0001f)
        return;

    const float angle = atan2f(radial.y, radial.x);
    if (!m_bInwardCircleLoopTracked || !IsSameInwardCircleLoop(_Center, _Radius))
    {
        m_bInwardCircleLoopTracked = true;
        m_bInwardCirclePassedLowerHalf = false;
        m_InwardCircleLoopCenter = _Center;
        m_InwardCircleLoopRadius = _Radius;
        m_InwardCircleLoopAccumulatedAngle = 0.f;
        m_InwardCircleLoopLastAngle = angle;
        return;
    }

    const float delta = WrapAngleRad(angle - m_InwardCircleLoopLastAngle);
    m_InwardCircleLoopLastAngle = angle;

    const float stepAngle = min(fabsf(delta), kSurfaceInwardCircleMaxAngleStep);
    if (stepAngle > 0.0001f)
        m_InwardCircleLoopAccumulatedAngle += stepAngle;

    if (radial.y >= 0.f)
        m_bInwardCirclePassedLowerHalf = true;
}

bool CPlayerScript::ShouldReleaseInwardCircle(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint) const
{
    if (!m_bInwardCircleLoopTracked || !IsSameInwardCircleLoop(_Center, _Radius))
        return false;

    const float safeRadius = max(_Radius, 0.0001f);
    const float upperHalfRatio = (_Center.y - _ContactPoint.y) / safeRadius;
    const bool reachedUpperHalf = (upperHalfRatio >= kSurfaceInwardCircleUpperHalfMin);
    const bool completedLoop = (m_InwardCircleLoopAccumulatedAngle >= kSurfaceInwardCircleLoopReleaseAngle);
    return m_bInwardCirclePassedLowerHalf && completedLoop && reachedUpperHalf;
}

void CPlayerScript::BlockInwardCircleAttach(const Vec2& _Center, float _Radius, float _Time)
{
    m_InwardCircleAttachBlockCenter = _Center;
    m_InwardCircleAttachBlockRadius = _Radius;
    m_InwardCircleAttachBlockTime = _Time;

    if (m_bHasCurrentSurfaceContact && m_CurrentSurfaceContact.Surface != nullptr)
        BlockSurfaceAttach(m_CurrentSurfaceContact.Surface, _Time);

    m_bHasPendingSurfaceContact = false;
    m_PendingSurfaceContact = SurfaceContact{};
    m_SurfaceResolveFrame = E_Time;
    m_SurfaceResolveScore = 999999.f;
    m_pResolvedSurface = nullptr;
    m_SurfaceGroundHoldTime = 0.f;
    ResetGroundContact();
    ResetInwardCircleLoopState();

    if (IsSurfaceAutoplayScenario() && !IsJump)
    {
        ++m_AutoplayInwardCircleReleaseCount;
        LogAutoplayState(L"circle_release");
    }
}

bool CPlayerScript::IsInwardCircleAttachBlocked(const Vec2& _Center, float _Radius) const
{
    if (m_InwardCircleAttachBlockTime <= 0.f)
        return false;

    return (fabsf(m_InwardCircleAttachBlockCenter.x - _Center.x) <= 0.5f)
        && (fabsf(m_InwardCircleAttachBlockCenter.y - _Center.y) <= 0.5f)
        && (fabsf(m_InwardCircleAttachBlockRadius - _Radius) <= 0.5f);
}

bool CPlayerScript::HasInwardCircleLineContext() const
{
    const auto isAttachableLineSurfaceObject = [](GameObject* _SurfaceObject)
    {
        if (_SurfaceObject == nullptr)
            return false;

        auto pSurface = _SurfaceObject->GetScript<CSurfaceScript>();
        if (pSurface == nullptr)
            return false;

        return pSurface->IsAttachable()
            && (pSurface->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::LINE)
            && (pSurface->GetRole() != CSurfaceScript::SURFACE_ROLE::WALL);
    };

    if (m_bHasCurrentSurfaceContact &&
        !m_CurrentSurfaceContact.Circle &&
        !m_CurrentSurfaceContact.WallLike &&
        m_CurrentSurfaceContact.Attachable)
    {
        return true;
    }

    if (m_bHasPendingSurfaceContact &&
        !m_PendingSurfaceContact.Circle &&
        !m_PendingSurfaceContact.WallLike &&
        m_PendingSurfaceContact.Attachable)
    {
        return true;
    }

    for (GameObject* pSurfaceObject : m_vecActiveSurfaceObjects)
    {
        if (isAttachableLineSurfaceObject(pSurfaceObject))
            return true;
    }

    return false;
}

bool CPlayerScript::TryEvaluateGuidedTopHalfInwardCircleContact(const SurfaceContact& _Contact,
                                                                bool& _OutOnActiveArc,
                                                                bool& _OutBeforeHalf,
                                                                bool& _OutBeforeHalfUsesTopRight) const
{
    _OutOnActiveArc = false;
    _OutBeforeHalf = false;
    _OutBeforeHalfUsesTopRight = false;

    if (!_Contact.Circle || !_Contact.InwardCircle || _Contact.Surface == nullptr)
        return false;

    auto pSurface = _Contact.Surface->GetScript<CSurfaceScript>();
    auto pGuide = _Contact.Surface->GetScript<CSurfaceCircleGuideScript>();
    if (pSurface == nullptr ||
        pGuide == nullptr ||
        !pGuide->IsEnabled() ||
        pSurface->GetGeometry() != CSurfaceScript::SURFACE_GEOMETRY::CIRCLE)
    {
        return false;
    }

    const CSurfaceScript::ARC_CORNER corner = pSurface->GetArcCorner();
    if (corner != CSurfaceScript::ARC_CORNER::TOP_LEFT &&
        corner != CSurfaceScript::ARC_CORNER::TOP_RIGHT)
    {
        return false;
    }

    Vec2 center = {};
    float radius = 0.f;
    Vec2 boxMin = {};
    Vec2 boxMax = {};
    pSurface->GetArcWorldData(center, radius, boxMin, boxMax);
    if (radius <= 0.0001f)
        return false;

    if (_Contact.ContactPoint.y >= center.y - 0.5f)
        return false;

    const float entryAngle = DegreesToRadians(pGuide->GetEntryAngleDeg());
    const float halfAngle = DegreesToRadians(pGuide->GetHalfCheckAngleDeg());
    const float exitAngle = DegreesToRadians(pGuide->GetExitAngleDeg());
    const float contactAngle = atan2f(_Contact.ContactPoint.y - center.y, _Contact.ContactPoint.x - center.x);

    bool counterClockwise = false;
    if (DeltaAngleCCW(entryAngle, halfAngle) <= DeltaAngleCCW(entryAngle, exitAngle) + 0.0001f)
        counterClockwise = true;
    else if (DeltaAngleCCW(exitAngle, halfAngle) <= DeltaAngleCCW(exitAngle, entryAngle) + 0.0001f)
        counterClockwise = false;
    else
        return false;

    const float arcToExit = DeltaAngleAlongOrientation(counterClockwise, entryAngle, exitAngle);
    const float arcToHalf = DeltaAngleAlongOrientation(counterClockwise, entryAngle, halfAngle);
    const float arcToContact = DeltaAngleAlongOrientation(counterClockwise, entryAngle, contactAngle);

    _OutOnActiveArc = (arcToContact <= arcToExit + 0.0001f);
    _OutBeforeHalf = _OutOnActiveArc && (arcToContact <= arcToHalf + 0.0001f);
    _OutBeforeHalfUsesTopRight = (cosf(entryAngle) >= 0.f);
    return true;
}

bool CPlayerScript::IsGuideLinkedCorrectionLine(const SurfaceContact& _LineContact, const SurfaceContact& _CircleContact) const
{
    if (_LineContact.Surface == nullptr || _CircleContact.Surface == nullptr)
        return false;

    if (_LineContact.Circle || !_CircleContact.Circle)
        return false;

    auto pGuide = _CircleContact.Surface->GetScript<CSurfaceCircleGuideScript>();
    if (pGuide == nullptr || !pGuide->IsEnabled() || pGuide->GetLinkedCorrectionLineName().empty())
        return false;

    return (_LineContact.Surface->GetName() == pGuide->GetLinkedCorrectionLineName());
}

void CPlayerScript::StartJump()
{
    IsJump = true;
    vVelocity += vNormal * 600.f;
    IsGround = false;
    m_SurfaceGroundHoldTime = 0.f;

    if (IsAutoplayEnabled())
    {
        ++m_AutoplayJumpStartCount;
        m_bAutoplayPendingJumpDetach = true;
        LogAutoplayState(L"jump_start");
    }

    GameObject* pJumpSurface = m_bHasCurrentSurfaceContact ? m_CurrentSurfaceContact.Surface : nullptr;
    if (pJumpSurface != nullptr)
        BlockSurfaceAttach(pJumpSurface, 0.12f);

    if (pJumpSurface != nullptr)
    {
        auto pSurface = pJumpSurface->GetScript<CSurfaceScript>();
        if (pSurface != nullptr && pSurface->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::CIRCLE)
        {
            Vec2 center = {};
            float radius = 0.f;
            Vec2 boxMin = {};
            Vec2 boxMax = {};
            pSurface->GetArcWorldData(center, radius, boxMin, boxMax);

            const Vec3 playerPos3 = Transform()->GetRelativePos();
            const Vec2 playerPos = Vec2(playerPos3.x, playerPos3.y);
            if (DotVec2(vNormal, playerPos - center) < 0.f)
                BlockInwardCircleAttach(center, radius, 0.12f);
        }
    }

    m_bHasPendingSurfaceContact = false;
    m_bHasCurrentSurfaceContact = false;
    m_CurrentSurfaceContact = SurfaceContact{};
    ResetInwardCircleLoopState();
    ResetInwardCircleHalfCheckerState();
    m_Action = ActionState::None;
    m_bBackReaction = false;
    m_BreakUngroundedTime = 0.f;
    m_IdleTime = 0.f;
}

void CPlayerScript::StartSkillDash()
{
    const float MaxSpeed = 600.f;

    m_Action = ActionState::SkillDash;
    m_bBackReaction = false;
    m_ActionTime = 0.f;
    m_Pose = PoseState::None;

    vVelocity.x = MaxSpeed * (float)m_Facing;
}

void CPlayerScript::Rolling()
{
    m_Action = ActionState::Roll;
    m_bBackReaction = false;
    m_ActionTime = 0.f;
    m_Pose = PoseState::None;

    if (IsGround)
    {
        float vt = vVelocity.Dot(vTangent);
        float curSpeed = fabsf(vt);

        if (curSpeed < 150.f)
            curSpeed = 150.f;

        vt = (m_Facing == 1) ? curSpeed : -curSpeed;
        vVelocity = vTangent * vt;
    }
    else
    {
        float curSpeed = fabsf(vVelocity.x);
        if (curSpeed < 150.f)
            curSpeed = 150.f;

        vVelocity.x = curSpeed * (float)m_Facing;
    }
}

bool CPlayerScript::CanMoveInput() const
{
    // ✅ 액션 중엔 이동 입력 잠금 (원하는 규칙에 따라 조절)
    if (m_Action == ActionState::SkillCharge) return false;
    if (m_Action == ActionState::SkillDash)   return false;
    if (m_Action == ActionState::Attack)      return false;
    if (m_Action == ActionState::KnockBack)   return false;
    if (m_Action == ActionState::Roll)        return false;
    if (m_Action == ActionState::Pushing)     return false;

    // ✅ 위/아래 보기 중에도 이동 금지
    if (m_Pose != PoseState::None) return false;

    return true;
}

void CPlayerScript::Simulate(const PlayerInput& in, float dt)
{
    Vec3 vPos = Transform()->GetRelativePos();
    Vec3 vScale = Transform()->GetRelativeScale();
    Vec3 vRotation = Transform()->GetRelativeRot();

    ApplyFacing(in, vScale);
    SimulateHorizontal(in, dt, vPos);
    SimulateVertical(dt, vPos);

    Transform()->SetRelativePos(vPos);
    Transform()->SetRelativeScale(vScale);
    Transform()->SetRelativeRot(vRotation);
}

void CPlayerScript::ApplyFacing(const PlayerInput& in, Vec3& vScale)
{
    bool lockFacing = (m_Action == ActionState::Break || m_Action == ActionState::Spring);
    GameObject* pFacingChild = nullptr;
    if (!GetOwner()->GetChild().empty() && GetOwner()->GetChild(0) != nullptr)
        pFacingChild = GetOwner()->GetChild(0).Get();

    if (!lockFacing)
    {
        float vt = vVelocity.Dot(vTangent);
    }

    // 실제 스케일 적용 (m_Facing 결정권은 유지)
    if (m_Facing == 1)
    {
        vScale.x = fabsf(vScale.x);
        if (pFacingChild != nullptr)
            pFacingChild->Transform()->SetRelativeRot(Vec3(0.f, 0.f, 0.f));
    }
    else
    {
        vScale.x = -fabsf(vScale.x);
        if (pFacingChild != nullptr)
            pFacingChild->Transform()->SetRelativeRot(Vec3(0.f, 0.f, XM_PI));
    }
}

void CPlayerScript::SimulateHorizontal(const PlayerInput& in, float dt, Vec3& vPos)
{
    const float MaxSpeed = 1000.f;
    const float PushSpeed = 30.f;

    if (m_Action == ActionState::Break)
    {
        float brakePower = 500.f;
        float breakSpeed = m_fBreakSpeed;
        if (m_bBreakWallLocked)
        {
            m_fBreakSpeed = 0.f;
            vVelocity.x = 0.f;
            vVelocity.y = 0.f;
            if (IsGround)
                vVelocity = vTangent * 0.f;
            else
                vVelocity.x = 0.f;
        }
        else
        {
            breakSpeed -= brakePower * dt;
            if (breakSpeed < 0.f)
                breakSpeed = 0.f;
            m_fBreakSpeed = breakSpeed;
        }

        if (m_fBreakSpeed < 10.f && !m_bBreakWallLocked)
        {
            m_iBreakDirection = 1;
            m_Action = ActionState::None;
            m_bBreakWallLocked = false;
            if (m_TurnTargetFacing != 0)
                m_Facing = m_TurnTargetFacing;
            m_fBreakSpeed = 0.f;
            return;
        }

        if (m_bBreakWallLocked)
            return;

		// 브레이크 중 공중이면 접선 방향 이동을 끊고 수직 낙하만 하게 한다.
		if (!IsGround)
		{
			vVelocity.x = 0.f;
			return;
		}

		vVelocity = vTangent * (m_fBreakSpeed * (float)m_iBreakDirection);

        vPos.x += vVelocity.x * dt;
        return;
    }
    // 1) 스킬 대시 이동
    else if (m_Action == ActionState::SkillDash)
    {
        const float skillFriction = 0.3f; // 네 기존 vFraction.x=0.5 효과
        float brakePower = 100.f;  // 반대키 감속 힘

        vVelocity.x -= vVelocity.x * skillFriction * dt;

        if (fabsf(vVelocity.x) < 100.f)
        {
            vVelocity.x = 0.f;
            m_Action = ActionState::None;
        }

        // 🔥 반대 방향 입력 시 추가 감속
        if ((vVelocity.x > 0.f && in.leftHeld) ||
            (vVelocity.x < 0.f && in.rightHeld))
        {
            if (vVelocity.x > 0.f)
                vVelocity.x -= brakePower * dt;
            else
                vVelocity.x += brakePower * dt;

        }

    }
    else if (m_Action == ActionState::Roll)
    {
        if (m_bBackReaction)
        {
            const float friction = IsGround ? kKnockBackGroundFriction : kKnockBackAirDrag;
            vVelocity.x -= vVelocity.x * friction * dt;

            if (IsGround && fabsf(vVelocity.x) < 25.f)
                vVelocity.x = 0.f;
        }
        else
        {
            float rollFriction = 0.3f; // 작게 줘야 자연스러움
            float brakePower = 200.f;  // 반대키 감속 힘
            vVelocity.x -= vVelocity.x * rollFriction * DT;

            if (IsGround)
            {
                float vt = vVelocity.Dot(vTangent);
                if (fabsf(vt) < 200.f)
                {
                    vVelocity.x = 0.f;
                    m_Action = ActionState::None;
                }
            }

            // 🔥 반대 방향 입력 시 추가 감속
            if ((vVelocity.x > 0.f && in.leftHeld) ||
                (vVelocity.x < 0.f && in.rightHeld))
            {
                if (vVelocity.x > 0.f)
                    vVelocity.x -= brakePower * dt;
                else
                    vVelocity.x += brakePower * dt;
            }
        }
    }
    else if (m_Action == ActionState::Spring)
    {
        // 공중 저항이나 마찰력이 필요하다면 적용 (SkillDash 로직 참고)
        float springFriction = 0.1f;
        vVelocity.x -= vVelocity.x * springFriction * dt;   

        // 지면에 닿거나 속도가 매우 줄어들면 상태 해제
        if (IsGround || fabsf(vVelocity.x) < 50.f)
        {
            m_Action = ActionState::None;
            bIsSpringJump = false;
        }
    }
    else if (m_Action == ActionState::Pushing)
    {
        float targetSpeed = 0.f;

        if (m_PushingDir == 1 && in.rightHeld && !in.leftHeld)
            targetSpeed = PushSpeed;
        else if (m_PushingDir == -1 && in.leftHeld && !in.rightHeld)
            targetSpeed = -PushSpeed;

        vVelocity.x = Lerp(vVelocity.x, targetSpeed, 12.f * dt);

        if (fabsf(vVelocity.x) < 5.f && targetSpeed == 0.f)
            vVelocity.x = 0.f;
    }
    else if (m_Action == ActionState::KnockBack)
    {
        const float friction = IsGround ? kKnockBackGroundFriction : kKnockBackAirDrag;
        vVelocity.x -= vVelocity.x * friction * dt;

        if (IsGround && fabsf(vVelocity.x) < 25.f)
            vVelocity.x = 0.f;
    }
    else
    {
        // 2) 일반 이동/마찰
        if (CanMoveInput())
        {
            const bool wantsRight = in.rightHeld && !in.leftHeld;
            const bool wantsLeft = in.leftHeld && !in.rightHeld;

            vFraction.x = 1.f;
            if (IsGround)
            {
                float vt = vVelocity.Dot(vTangent);

                if (wantsRight)
                {
                    m_Facing = 1;
                    vt += vAccel.x * dt;
                }
                else if (wantsLeft)
                {
                    m_Facing = -1;
                    vt -= vAccel.x * dt;              
                }
                else
                {
                    vt -= vt * vFraction.x * dt;      
                    if (fabsf(vt) < 30.f)
                        vt = 0.f;
                }

                if (vt > MaxSpeed)  vt = MaxSpeed;
                if (vt < -MaxSpeed) vt = -MaxSpeed;

                vVelocity = vTangent * vt;
            }
            else
            {
                // 공중 제어 (약하게)
                float airAccel = vAccel.x * 0.5f;
                float airFraction = 1.f;

                if (wantsRight)
                    vVelocity.x += airAccel * dt;
                else if (wantsLeft)
                    vVelocity.x -= airAccel * dt;

                // 공중에서는 마찰 거의 없음
                else
                {
                    vVelocity.x -= vVelocity.x * airFraction * dt;
                    if (fabsf(vVelocity.x) < 10.f) vVelocity.x = 0.f;
                }
            }
        }
        else
        {
            // 이동 잠금 중이면 마찰로 멈추기
            vVelocity.x -= vVelocity.x * vFraction.x * dt;
            if (fabsf(vVelocity.x) < 10.f) vVelocity.x = 0.f;
        }
    }

    // 속도 제한
    if (vVelocity.x > MaxSpeed)  vVelocity.x = MaxSpeed;
    if (vVelocity.x < -MaxSpeed) vVelocity.x = -MaxSpeed;

    vPos.x += vVelocity.x * dt;
}

void CPlayerScript::SimulateVertical(float dt, Vec3& vPos)
{
    if (m_Action == ActionState::Break && m_bBreakWallLocked)
    {
        vVelocity.x = 0.f;
        vVelocity.y = 0.f;
        IsJump = false;
        bIsSpringJump = false;
        return;
    }

    if (IsGround)
    {
        // 접선 속도의 y 성분만 반영
        vPos.y += vVelocity.y * dt;
        IsJump = false;
        bIsSpringJump = false;
        return;
    }

    vVelocity.y -= vAccel.y * dt;
    vPos.y += vVelocity.y * dt;

    if (vVelocity.y > 600.f)
    {
        bIsSpringJump = true;
    }
    else if (vVelocity.y < -600.f)
    {
        bIsSpringJump = false;
    }
}

void CPlayerScript::UpdateTimers(float dt)
{
    // IdleLong 조건: 지상 + 액션 없음 + 포즈 없음 + 속도 매우 낮음
    const float absSpeed = IsGround ? fabsf(vVelocity.Dot(vTangent)) : fabsf(vVelocity.x);
    const bool idleCandidate =
        IsGround &&
        (m_Action == ActionState::None) &&
        (m_Pose == PoseState::None) &&
        (absSpeed < 15.f);

    if (idleCandidate) m_IdleTime += dt;
    else m_IdleTime = 0.f;

    if (m_Pose != PoseState::None) m_PoseTime += dt;
    else m_PoseTime = 0.f;

    if (m_Action != ActionState::None) m_ActionTime += dt;
    else m_ActionTime = 0.f;
}

void CPlayerScript::UpdateGroundRotation()
{
    float curRotZ = Transform()->GetRelativeRot().z;

    if (IsGround)
    {
        if (IsJump)
        {
            Transform()->SetRelativeRot(Vec3(0.f, 0.f, 0.f));
        }
        else
        {
            float fAngle = atan2f(vNormal.y, vNormal.x);
            float fTargetRot = fAngle + XM_PIDIV2 + XM_PI;
            float fSmoothedRot = LerpAngleRad(curRotZ, fTargetRot, kGroundRotationLerpSpeed, DT);
            Transform()->SetRelativeRot(Vec3(0.f, 0.f, fSmoothedRot));
        }
    }
    else
    {
        if (IsJump)
        {
            Transform()->SetRelativeRot(Vec3(0.f, 0.f, 0.f));
        }
        else
        {
            float fAngle = atan2f(vNormal.y, vNormal.x);
            float fTargetRot = fAngle + XM_PIDIV2 + XM_PI;
            float fSmoothedRot = LerpAngleRad(curRotZ, fTargetRot, kGroundRotationLerpSpeed, DT);
            Transform()->SetRelativeRot(Vec3(0.f, 0.f, fSmoothedRot));
        }
    }
}

void CPlayerScript::UpdateAnimation(float dt)
{
    const float MaxSpeed = 300.f;
    const float absSpeed = IsGround ? fabsf(vVelocity.Dot(vTangent)) : fabsf(vVelocity.x);
    if (m_Action == ActionState::KnockBack)
    {
        Ptr<AFlipbook> pKnockBackFlipbook = FlipbookRender()->GetFlipbook(kKnockBackFlipbookIndex);
        if (nullptr != pKnockBackFlipbook && 0 < pKnockBackFlipbook->GetSpriteCount())
        {
            const float knockBackAnimFPS = (float)pKnockBackFlipbook->GetSpriteCount() / kKnockBackMaxDuration;
            if (FlipbookRender()->GetCurFlipbookIdx() != kKnockBackFlipbookIndex)
                FlipbookRender()->Play(kKnockBackFlipbookIndex, knockBackAnimFPS, 0);
        }
        return;
    }

    // 1) Action 우선
    if (m_Action == ActionState::SkillDash)
    {
        FlipbookRender()->Play(4, 20.f, -1);
        return;
    }
    if (m_Action == ActionState::SkillCharge)
    {
        FlipbookRender()->Play(7, 60.f, -1);
        return;
    }
    // TODO: Attack/Roll 애니
    // if (m_Action == ActionState::Attack) { ... return; }
    if (m_Action == ActionState::Roll)
    {
        FlipbookRender()->Play(4, 20.f, -1);
        return;
    }
    if (m_Action == ActionState::Break)
    {
        float breakSpeed = m_fBreakSpeed;
        if (breakSpeed <= 0.f)
            breakSpeed = IsGround
            ? fmaxf(fabsf(vVelocity.Dot(vTangent)), fabsf(vVelocity.x))
            : fabsf(vVelocity.x);

        if (breakSpeed < 100.f)
            FlipbookRender()->Play(9, 10.f, -1);
        else
            FlipbookRender()->Play(8, 12.f, -1);
        return;
    }
    if (m_Action == ActionState::Pushing)
    {
        FlipbookRender()->Play(kPushingFlipbookIndex, 6.f, -1);
        return;
    }

    // 2) 공중(점프/낙하)
    if (!IsGround)
    {
        //Transform()->SetRelativeRot(Vec3(0.f, 0.f, 0.f));
        //float fAngle = atan2f(vNormal.y, vNormal.x);
        //float fTargetRot = fAngle + XM_PIDIV2 + XM_PI;
        //Transform()->SetRelativeRot(Vec3(0.f, 0.f, fTargetRot));

        if (bIsSpringJump)
        {
            FlipbookRender()->Play(10, 10.f, 1);
            return;
        }
        else
        {
            FlipbookRender()->Play(4, 15.f, -1);
            return;
        }
    }
    // 3) Pose(위/아래 보기)
    if (m_Pose == PoseState::LookUp)
    {
        // 네 기존 "시작 프레임 1회" 느낌 유지
        if (m_PoseTime < 0.1f)
            FlipbookRender()->Play(5, 0, 20.f, 0);
        //else
        //    FlipbookRender()->Play(5, 1, 10.f, -1);
        return;
    }
    if (m_Pose == PoseState::LookDown)
    {
        if (m_PoseTime < 0.1f)
            FlipbookRender()->Play(6, 0, 20.f, 0);
        //else
        //    FlipbookRender()->Play(6, 1, 10.f, -1); 
        return;
    }

    // 4) 이동/대기
    if (absSpeed < 15.f)
    {
        if (m_IdleTime > 5.f)
            FlipbookRender()->Play(2, 5.f, -1);   // IdleLong
        else
            FlipbookRender()->Play(1, 1.f, -1);   // Idle
        return;
    }

    if (absSpeed >= MaxSpeed - 1.f)
    {
        FlipbookRender()->Play(3, 10.f, -1);      // Run
    }
    else
        FlipbookRender()->Play(0, 8.f, -1);       // Walk

}

void CPlayerScript::ResetGroundContact()
{
    IsGround = false;
    vNormal = Vec2(0.f, 1.f);
    vTangent = Vec2(1.f, 0.f);
    m_bHasCurrentSurfaceContact = false;
    m_CurrentSurfaceContact = SurfaceContact{};
    ResetInwardCircleHalfCheckerState();
}

void CPlayerScript::ForceFlatGroundContact(float _HoldTime)
{
    if (m_bHasCurrentSurfaceContact && m_CurrentSurfaceContact.Surface != nullptr)
        BlockSurfaceAttach(m_CurrentSurfaceContact.Surface, _HoldTime);

    m_bHasPendingSurfaceContact = false;
    m_PendingSurfaceContact = SurfaceContact{};
    m_bHasCurrentSurfaceContact = false;
    m_CurrentSurfaceContact = SurfaceContact{};
    m_SurfaceResolveFrame = E_Time;
    m_SurfaceResolveScore = 999999.f;
    m_pResolvedSurface = nullptr;

    IsGround = true;
    IsJump = false;
    bIsSpringJump = false;
    vNormal = Vec2(0.f, 1.f);
    vTangent = Vec2(1.f, 0.f);
    RefreshSurfaceGroundHold(_HoldTime);
    ResetInwardCircleLoopState();
    ResetInwardCircleHalfCheckerState();
}

void CPlayerScript::RegisterActiveSurface(GameObject* _SurfaceObject)
{
    if (_SurfaceObject == nullptr)
        return;

    for (GameObject* pSurface : m_vecActiveSurfaceObjects)
    {
        if (pSurface == _SurfaceObject)
            return;
    }

    m_vecActiveSurfaceObjects.push_back(_SurfaceObject);
}

void CPlayerScript::UnregisterActiveSurface(GameObject* _SurfaceObject)
{
    if (_SurfaceObject == nullptr)
        return;

    auto iter = m_vecActiveSurfaceObjects.begin();
    while (iter != m_vecActiveSurfaceObjects.end())
    {
        if (*iter == _SurfaceObject)
            iter = m_vecActiveSurfaceObjects.erase(iter);
        else
            ++iter;
    }
}

float CPlayerScript::ComputeSurfaceCandidateScore(const SurfaceContact& _Contact, GameObject* _CurrentBestSurface) const
{
    const bool wasGround = GetIsGround();
    float candidateScore = _Contact.Score;
    const float seamDirBias = ComputeSeamDirectionBias(_Contact.SeamBlendT, _Contact.SeamBlendHasValue, _Contact.TransitionSurface, vVelocity.x);
    if (!_Contact.WallLike && !_Contact.Circle && seamDirBias > 0.f)
    {
        const float moveSpeed = fabsf(vVelocity.x);
        const float speedRate = Clamp01f(moveSpeed / kSurfaceLineSeamMoveSpeedGate);
        const float biasScale = 0.5f + 0.5f * speedRate;
        candidateScore -= kSurfaceLineSeamDirectionBias * seamDirBias * biasScale;
    }

    if (_Contact.TransitionSurface)
    {
        const bool canPreferTransition = _Contact.SeamBlendHasValue &&
            (_Contact.Score <= kSurfaceLineSeamTransitionDepth * 2.f);
        if (canPreferTransition && _CurrentBestSurface != _Contact.Surface)
            candidateScore -= kSurfaceLineSeamTransitionScoreFavor;

        if (wasGround && _Contact.SeamBlendHasValue)
        {
            const Vec2 previousNormal = NormalizeSafeVec2(vNormal);
            const Vec2 nextNormal = NormalizeSafeVec2(_Contact.Normal, previousNormal);
            const float previousToNextDot = DotVec2(previousNormal, _Contact.Normal);
            const Vec2 nextTangent = Vec2(nextNormal.y, -nextNormal.x);
            const float nextTangentSpeed = DotVec2(vVelocity, nextTangent);
            const bool risingSlopeEntry =
                (previousNormal.y > nextNormal.y + 0.01f) &&
                (nextTangentSpeed * nextTangent.y > kSurfaceLineSeamRisingSlopeMoveMin);

            if (previousToNextDot < kSurfaceLineSeamAngleShiftDot)
                candidateScore -= kSurfaceLineSeamAngleShiftScorePenalty;

            if (risingSlopeEntry)
                candidateScore -= kSurfaceLineSeamRisingSlopeScoreFavor;
        }
    }

    return candidateScore;
}

bool CPlayerScript::IsTopHalfInwardCircleContact(const SurfaceContact& _Contact)
{
    if (!_Contact.Circle || !_Contact.InwardCircle || _Contact.Surface == nullptr)
        return false;

    auto pSurface = _Contact.Surface->GetScript<CSurfaceScript>();
    if (pSurface == nullptr || pSurface->GetGeometry() != CSurfaceScript::SURFACE_GEOMETRY::CIRCLE)
        return false;

    const CSurfaceScript::ARC_CORNER corner = pSurface->GetArcCorner();
    if (corner != CSurfaceScript::ARC_CORNER::TOP_LEFT &&
        corner != CSurfaceScript::ARC_CORNER::TOP_RIGHT)
    {
        return false;
    }

    Vec2 center = {};
    float radius = 0.f;
    Vec2 boxMin = {};
    Vec2 boxMax = {};
    pSurface->GetArcWorldData(center, radius, boxMin, boxMax);
    return (_Contact.ContactPoint.y < center.y - 0.5f);
}

bool CPlayerScript::ShouldIgnoreTopHalfInwardCircleContact(const SurfaceContact& _Contact)
{
    if (!IsTopHalfInwardCircleContact(_Contact) || _Contact.Surface == nullptr)
        return false;

    if (!HasInwardCircleLineContext())
        return false;

    auto pSurface = _Contact.Surface->GetScript<CSurfaceScript>();
    if (pSurface == nullptr)
        return false;

    bool guideOnActiveArc = false;
    bool guideBeforeHalf = false;
    bool guideBeforeHalfUsesTopRight = false;
    if (TryEvaluateGuidedTopHalfInwardCircleContact(_Contact, guideOnActiveArc, guideBeforeHalf, guideBeforeHalfUsesTopRight))
    {
        if (!guideOnActiveArc)
            return true;

        // Guided correction-line rules:
        // 1) before Entry  : line keeps ownership
        // 2) Entry -> Half : circle forcibly owns the overlap
        // 3) after Half    : line owns the overlap again
        return !guideBeforeHalf;
    }

    Vec2 center = {};
    float radius = 0.f;
    Vec2 boxMin = {};
    Vec2 boxMax = {};
    pSurface->GetArcWorldData(center, radius, boxMin, boxMax);

    const auto isChordLikeLineSurface = [&](GameObject* _SurfaceObject)
    {
        if (_SurfaceObject == nullptr)
            return false;

        auto pLineSurface = _SurfaceObject->GetScript<CSurfaceScript>();
        if (pLineSurface == nullptr ||
            pLineSurface->GetGeometry() != CSurfaceScript::SURFACE_GEOMETRY::LINE ||
            !pLineSurface->IsAttachable())
        {
            return false;
        }

        Vec2 lineStart = {};
        Vec2 lineEnd = {};
        pLineSurface->GetWorldEndpoints(lineStart, lineEnd);

        const float safeRadius = max(radius, 0.0001f);
        const float endpointTolerance = max(8.f, safeRadius * 0.08f);
        const float insideMargin = max(4.f, safeRadius * 0.03f);
        const float startRadiusError = fabsf(LengthVec2(lineStart - center) - safeRadius);
        const float endRadiusError = fabsf(LengthVec2(lineEnd - center) - safeRadius);
        const Vec2 lineMid = (lineStart + lineEnd) * 0.5f;
        const float midRadius = LengthVec2(lineMid - center);

        return (startRadiusError <= endpointTolerance) &&
            (endRadiusError <= endpointTolerance) &&
            (midRadius < safeRadius - insideMargin);
    };

    if (m_bHasCurrentSurfaceContact && isChordLikeLineSurface(m_CurrentSurfaceContact.Surface))
        return false;

    if (m_bHasPendingSurfaceContact && isChordLikeLineSurface(m_PendingSurfaceContact.Surface))
        return false;

    for (GameObject* pSurfaceObject : m_vecActiveSurfaceObjects)
    {
        if (isChordLikeLineSurface(pSurfaceObject))
            return false;
    }

    UpdateInwardCircleHalfChecker(center, radius, _Contact.ContactPoint);
    const bool lineOwnsTopRight = ShouldLineOwnTopRightInwardCircleHalf(center, radius, _Contact.ContactPoint);
    const CSurfaceScript::ARC_CORNER corner = pSurface->GetArcCorner();
    if (lineOwnsTopRight)
        return (corner == CSurfaceScript::ARC_CORNER::TOP_RIGHT);

    return (corner == CSurfaceScript::ARC_CORNER::TOP_LEFT);
}

bool CPlayerScript::IsChordLineForInwardCircle(const SurfaceContact& _LineContact, const SurfaceContact& _CircleContact) const
{
    if (_LineContact.Surface == nullptr || _CircleContact.Surface == nullptr)
        return false;

    if (IsGuideLinkedCorrectionLine(_LineContact, _CircleContact))
        return true;

    auto pLineSurface = _LineContact.Surface->GetScript<CSurfaceScript>();
    auto pCircleSurface = _CircleContact.Surface->GetScript<CSurfaceScript>();
    if (pLineSurface == nullptr || pCircleSurface == nullptr)
        return false;

    if (pLineSurface->GetGeometry() != CSurfaceScript::SURFACE_GEOMETRY::LINE ||
        pCircleSurface->GetGeometry() != CSurfaceScript::SURFACE_GEOMETRY::CIRCLE)
    {
        return false;
    }

    Vec2 lineStart = {};
    Vec2 lineEnd = {};
    pLineSurface->GetWorldEndpoints(lineStart, lineEnd);

    Vec2 center = {};
    float radius = 0.f;
    Vec2 boxMin = {};
    Vec2 boxMax = {};
    pCircleSurface->GetArcWorldData(center, radius, boxMin, boxMax);

    const float safeRadius = max(radius, 0.0001f);
    const float endpointTolerance = max(8.f, safeRadius * 0.08f);
    const float insideMargin = max(4.f, safeRadius * 0.03f);
    const float startRadiusError = fabsf(LengthVec2(lineStart - center) - safeRadius);
    const float endRadiusError = fabsf(LengthVec2(lineEnd - center) - safeRadius);
    const Vec2 lineMid = (lineStart + lineEnd) * 0.5f;
    const float midRadius = LengthVec2(lineMid - center);

    return (startRadiusError <= endpointTolerance) &&
        (endRadiusError <= endpointTolerance) &&
        (midRadius < safeRadius - insideMargin);
}

bool CPlayerScript::ShouldPreferLineOverTopHalfInwardCircle(const SurfaceContact& _LineContact, const SurfaceContact& _CircleContact)
{
    if (_LineContact.Circle || _LineContact.WallLike || !_LineContact.Attachable)
        return false;

    if (!IsTopHalfInwardCircleContact(_CircleContact))
        return false;

    if (!ShouldIgnoreTopHalfInwardCircleContact(_CircleContact))
        return false;

    const bool guideLinkedCorrectionLine = IsGuideLinkedCorrectionLine(_LineContact, _CircleContact);
    const Vec2 lineNormal = NormalizeSafeVec2(_LineContact.Normal);
    const Vec2 circleNormal = NormalizeSafeVec2(_CircleContact.Normal, lineNormal);
    if (!guideLinkedCorrectionLine && DotVec2(lineNormal, circleNormal) <= 0.1f)
        return false;

    if (guideLinkedCorrectionLine)
        return true;

    if (!IsChordLineForInwardCircle(_LineContact, _CircleContact))
        return true;

    return (fabsf(_LineContact.SignedDistance) <= kSurfaceLineAttachSnapMaxDistance);
}

bool CPlayerScript::ShouldPreferTopHalfInwardCircleOverLine(const SurfaceContact& _CircleContact, const SurfaceContact& _LineContact)
{
    if (!IsTopHalfInwardCircleContact(_CircleContact))
        return false;

    if (ShouldIgnoreTopHalfInwardCircleContact(_CircleContact))
        return false;

    if (_LineContact.Circle || _LineContact.WallLike || !_LineContact.Attachable)
        return false;

    const bool guideLinkedCorrectionLine = IsGuideLinkedCorrectionLine(_LineContact, _CircleContact);
    const Vec2 lineNormal = NormalizeSafeVec2(_LineContact.Normal);
    const Vec2 circleNormal = NormalizeSafeVec2(_CircleContact.Normal, lineNormal);
    if (!guideLinkedCorrectionLine && DotVec2(lineNormal, circleNormal) <= 0.1f)
        return false;

    if (!IsChordLineForInwardCircle(_LineContact, _CircleContact))
        return false;

    if (guideLinkedCorrectionLine)
        return true;

    return true;
}

bool CPlayerScript::TrySelectSurfaceContact(SurfaceContact _Contact, SurfaceContact& _BestContact,
                                            float& _BestScore, GameObject*& _BestSurface, bool& _HasBest)
{
    _Contact.Score = ComputeSurfaceCandidateScore(_Contact, _BestSurface);

    if (!_Contact.Circle && !_Contact.WallLike && _Contact.Attachable)
        PrimeInwardCircleHalfCheckerFromLineContext(_Contact);

    if (ShouldIgnoreTopHalfInwardCircleContact(_Contact))
        return false;

    bool accept = false;
    if (!_HasBest)
    {
        accept = true;
    }
    else if (ShouldIgnoreTopHalfInwardCircleContact(_BestContact))
    {
        accept = true;
    }
    else if (ShouldPreferLineOverTopHalfInwardCircle(_Contact, _BestContact))
    {
        accept = true;
    }
    else if (ShouldPreferLineOverTopHalfInwardCircle(_BestContact, _Contact))
    {
        accept = false;
    }
    else if (ShouldPreferTopHalfInwardCircleOverLine(_Contact, _BestContact))
    {
        accept = true;
    }
    else if (ShouldPreferTopHalfInwardCircleOverLine(_BestContact, _Contact))
    {
        accept = false;
    }
    else if (_BestSurface == _Contact.Surface)
    {
        accept = (_Contact.Score <= _BestScore + kSurfaceResolveScoreEpsilon);
    }
    else if (_Contact.Score + kSurfaceResolveScoreEpsilon < _BestScore)
    {
        accept = true;
    }

    if (!accept)
        return false;

    _BestContact = _Contact;
    _BestScore = _Contact.Score;
    _BestSurface = _Contact.Surface;
    _HasBest = true;
    return true;
}

bool CPlayerScript::ProbeSurfaceObject(GameObject* _SurfaceObject, bool _WasGround,
                                       SurfaceContact& _BestContact, float& _BestScore,
                                       GameObject*& _BestSurface, bool& _HasBest)
{
    if (_SurfaceObject == nullptr || _SurfaceObject->IsDead())
        return false;

    auto pSurface = _SurfaceObject->GetScript<CSurfaceScript>();
    if (pSurface == nullptr)
        return false;

    CSurfaceScript::CONTACT_PROBE probe = {};
    if (!pSurface->ProbePlayerContact(Collider2D(), _WasGround, probe))
        return false;

    SurfaceContact contact = {};
    contact.Surface = _SurfaceObject;
    contact.Normal = probe.Normal;
    contact.SignedDistance = probe.SignedDistance;
    contact.Score = probe.CandidateScore;
    contact.TransitionSurface = probe.TransitionSurface;
    contact.Attachable = probe.Attachable;
    contact.WallLike = probe.WallLike;
    contact.Circle = probe.Circle;
    contact.InwardCircle = probe.InwardCircle;
    contact.SeamBlendT = probe.SeamBlendT;
    contact.SeamBlendHasValue = probe.SeamBlendHasValue;
    contact.SeamStart = probe.SeamStart;
    contact.SeamEnd = probe.SeamEnd;
    contact.ContactPoint = probe.ContactPoint;
    return TrySelectSurfaceContact(contact, _BestContact, _BestScore, _BestSurface, _HasBest);
}

void CPlayerScript::ProbeSceneSurfaceContacts(bool _WasGround, SurfaceContact& _BestContact, float& _BestScore,
                                              GameObject*& _BestSurface, bool& _HasBest)
{
    Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();
    if (pCurLevel == nullptr)
        return;

    auto ProbeSurfaceTree = [&](auto&& _Self, GameObject* _Object) -> void
    {
        if (_Object == nullptr || _Object->IsDead())
            return;

        if (_Object->GetScript<CSurfaceScript>() != nullptr)
            ProbeSurfaceObject(_Object, _WasGround, _BestContact, _BestScore, _BestSurface, _HasBest);

        const vector<Ptr<GameObject>>& vecChild = _Object->GetChild();
        for (size_t childIdx = 0; childIdx < vecChild.size(); ++childIdx)
            _Self(_Self, vecChild[childIdx].Get());
    };

    for (UINT layerIdx = 0; layerIdx < MAX_LAYER; ++layerIdx)
    {
        const vector<Ptr<GameObject>>& vecObjects = pCurLevel->GetLayer(layerIdx)->GetParentObjects();
        for (size_t i = 0; i < vecObjects.size(); ++i)
        {
            ProbeSurfaceTree(ProbeSurfaceTree, vecObjects[i].Get());
        }
    }
}

void CPlayerScript::ResolveBufferedSurfaceContacts()
{
    const bool wasGround = GetIsGround() || m_SurfaceGroundHoldTime > 0.f;
    if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        AppendSurfaceAutoplayDebug(L"resolve begin t=%.3f wasGround=%d active=%zu pending=%d current=%d",
                                   m_AutoplayTime, wasGround ? 1 : 0, m_vecActiveSurfaceObjects.size(),
                                   m_bHasPendingSurfaceContact ? 1 : 0, m_bHasCurrentSurfaceContact ? 1 : 0);
    SurfaceContact bestContact = {};
    float bestScore = 999999.f;
    GameObject* bestSurface = nullptr;
    bool hasBest = false;

    if (m_bHasPendingSurfaceContact)
        ProbeSurfaceObject(m_PendingSurfaceContact.Surface, wasGround, bestContact, bestScore, bestSurface, hasBest);

    if (m_bHasCurrentSurfaceContact)
        ProbeSurfaceObject(m_CurrentSurfaceContact.Surface, wasGround, bestContact, bestScore, bestSurface, hasBest);

    for (GameObject* pSurfaceObject : m_vecActiveSurfaceObjects)
        ProbeSurfaceObject(pSurfaceObject, wasGround, bestContact, bestScore, bestSurface, hasBest);

    ProbeSceneSurfaceContacts(wasGround, bestContact, bestScore, bestSurface, hasBest);

    m_bHasPendingSurfaceContact = false;

    if (hasBest)
    {
        if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        {
            const wchar_t* surfaceName = (bestContact.Surface != nullptr) ? bestContact.Surface->GetName().c_str() : L"-";
            AppendSurfaceAutoplayDebug(L"resolve apply t=%.3f surface=%ls dist=%.3f normal=(%.3f,%.3f)",
                                       m_AutoplayTime, surfaceName, bestContact.SignedDistance, bestContact.Normal.x, bestContact.Normal.y);
        }
        if (ApplySurfaceContact(bestContact))
        {
            m_bHasCurrentSurfaceContact = true;
            m_CurrentSurfaceContact = bestContact;
        }
    }
    else if (m_SurfaceGroundHoldTime <= 0.f && m_TileOverlapCount == 0)
    {
        if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
            AppendSurfaceAutoplayDebug(L"resolve reset t=%.3f", m_AutoplayTime);
        ResetGroundContact();
    }
}

void CPlayerScript::SubmitSurfaceContact(GameObject* _Surface, const Vec2& _Normal, float _SignedDistance,
                                         bool _TransitionSurface, bool _Attachable, bool _WallLike, bool _Circle, bool _InwardCircle, float _Score,
                                         float _SeamBlendT, bool _SeamBlendHasValue, const Vec2& _SeamStart,
                                         const Vec2& _SeamEnd, const Vec2& _ContactPoint)
{
    if (_Surface == nullptr)
        return;

    if (fabsf(m_SurfaceResolveFrame - E_Time) > kSurfaceResolveFrameEpsilon)
    {
        m_SurfaceResolveFrame = E_Time;
        m_SurfaceResolveScore = 999999.f;
        m_pResolvedSurface = nullptr;
        m_bHasPendingSurfaceContact = false;
    }

    SurfaceContact contact = {};
    contact.Surface = _Surface;
    contact.Normal = _Normal;
    contact.SignedDistance = _SignedDistance;
    contact.Score = _Score;
    contact.TransitionSurface = _TransitionSurface;
    contact.Attachable = _Attachable;
    contact.WallLike = _WallLike;
    contact.Circle = _Circle;
    contact.InwardCircle = _InwardCircle;
    contact.SeamBlendT = _SeamBlendT;
    contact.SeamBlendHasValue = _SeamBlendHasValue;
    contact.SeamStart = _SeamStart;
    contact.SeamEnd = _SeamEnd;
    contact.ContactPoint = _ContactPoint;

    TrySelectSurfaceContact(contact, m_PendingSurfaceContact, m_SurfaceResolveScore, m_pResolvedSurface, m_bHasPendingSurfaceContact);
}

bool CPlayerScript::ApplySurfaceContact(const SurfaceContact& _Contact)
{
    Vec2 normal = NormalizeSafeVec2(_Contact.Normal);
    const bool wasGround = GetIsGround();
    const PlayerInput surfaceInput = ReadInput();
    const bool jumpPressed = surfaceInput.spacePressed || surfaceInput.spaceHeld || IsJump;
    const bool isAttachableLineContact = _Contact.Attachable && !_Contact.WallLike && !_Contact.Circle;
    const bool isCurrentAttachableLine =
        m_bHasCurrentSurfaceContact &&
        m_CurrentSurfaceContact.Surface != nullptr &&
        m_CurrentSurfaceContact.Attachable &&
        !m_CurrentSurfaceContact.WallLike &&
        !m_CurrentSurfaceContact.Circle;
    const bool isSameCurrentLineSurface =
        isAttachableLineContact &&
        isCurrentAttachableLine &&
        (m_CurrentSurfaceContact.Surface == _Contact.Surface);
    const bool allowLineCorrection =
        isAttachableLineContact &&
        (isSameCurrentLineSurface || _Contact.TransitionSurface);

    bool inwardCircleContact = false;
    Vec2 inwardCircleCenter = Vec2(0.f, 0.f);
    float inwardCircleRadius = 0.f;
    float inwardCircleUpperHalfRatio = 0.f;
    float inwardCircleLowerHalfRatio = 0.f;

    const bool continuingCurrentInwardCircle =
        m_bHasCurrentSurfaceContact &&
        m_CurrentSurfaceContact.Surface != nullptr &&
        m_CurrentSurfaceContact.Circle &&
        m_CurrentSurfaceContact.InwardCircle &&
        (m_CurrentSurfaceContact.Surface == _Contact.Surface);

    if (!_Contact.Circle || !_Contact.InwardCircle)
    {
        ResetInwardCircleLoopState();
    }
    else if (_Contact.Surface != nullptr)
    {
        auto pSurface = _Contact.Surface->GetScript<CSurfaceScript>();
        if (pSurface != nullptr)
        {
            Vec2 center = {};
            float radius = 0.f;
            Vec2 boxMin = {};
            Vec2 boxMax = {};
            pSurface->GetArcWorldData(center, radius, boxMin, boxMax);

            inwardCircleContact = true;
            inwardCircleCenter = center;
            inwardCircleRadius = radius;
            const float safeRadius = max(radius, 0.0001f);
            inwardCircleUpperHalfRatio = (center.y - _Contact.ContactPoint.y) / safeRadius;
            inwardCircleLowerHalfRatio = (_Contact.ContactPoint.y - center.y) / safeRadius;

            if (IsInwardCircleAttachBlocked(center, radius))
                return false;

            UpdateInwardCircleHalfChecker(center, radius, _Contact.ContactPoint);
            NoteInwardCircleLoopProgress(center, radius, _Contact.ContactPoint);
            if (!jumpPressed &&
                continuingCurrentInwardCircle &&
                ShouldReleaseInwardCircle(center, radius, _Contact.ContactPoint))
            {
                BlockInwardCircleAttach(center, radius, kSurfaceInwardCircleAttachBlockTime);
                return false;
            }
        }
    }

    Vec2 oldNormal = vNormal;
    if (fabsf(oldNormal.x) < 0.0001f && fabsf(oldNormal.y) < 0.0001f)
        oldNormal = normal;
    else
        oldNormal = NormalizeSafeVec2(oldNormal, normal);

    float normalDot = DotVec2(oldNormal, normal);
    if (normalDot > 1.f) normalDot = 1.f;
    else if (normalDot < -1.f) normalDot = -1.f;

    float normalBlend = 1.f;
    if (_Contact.Circle && wasGround)
        normalBlend = kSurfaceCircleNormalBlend;

    if (wasGround && !_Contact.TransitionSurface)
        normalBlend = 1.f;

    if (_Contact.TransitionSurface)
        normalBlend = 1.f;

    const bool bLineSeamCandidate =
        _Contact.TransitionSurface &&
        !_Contact.WallLike &&
        !_Contact.Circle;

    Vec2 candidateTangent = Vec2(normal.y, -normal.x);
    const float candidateTangentSpeed = DotVec2(vVelocity, candidateTangent);
    const bool bSlowSeamSpeed = fabsf(DotVec2(vVelocity, candidateTangent)) < kSurfaceLineSeamHoldSpeedMin;
    const bool bLowSpeedInwardCircleDetachEligible =
        inwardCircleContact &&
        continuingCurrentInwardCircle &&
        m_bInwardCircleLoopTracked &&
        IsSameInwardCircleLoop(inwardCircleCenter, inwardCircleRadius) &&
        (m_InwardCircleLoopAccumulatedAngle >= kSurfaceInwardCircleLowSpeedDetachMinAngle);
    const bool bLowSpeedInwardCircleDetach =
        bLowSpeedInwardCircleDetachEligible &&
        !jumpPressed &&
        (inwardCircleUpperHalfRatio >= kSurfaceInwardCircleLowSpeedDetachUpperHalfMin) &&
        (fabsf(candidateTangentSpeed) < kSurfaceInwardCircleLowSpeedDetachSpeedMin);
    const bool bRisingSlopeTransition =
        bLineSeamCandidate &&
        wasGround &&
        (oldNormal.y > normal.y + 0.01f) &&
        (candidateTangentSpeed * candidateTangent.y > kSurfaceLineSeamRisingSlopeMoveMin) &&
        (fabsf(_Contact.SignedDistance) <= kSurfaceLineSeamTransitionDepth * 2.f);
    float lineTransitionBlend = ComputeLineSeamBlend(
        _Contact.SeamBlendT,
        _Contact.SeamBlendHasValue,
        _Contact.TransitionSurface,
        _Contact.SeamStart,
        _Contact.SeamEnd,
        _Contact.ContactPoint,
        vVelocity.x);
    lineTransitionBlend = Clamp01f(lineTransitionBlend * 2.0f);
    if (bSlowSeamSpeed)
        lineTransitionBlend *= 0.4f;

    if (fabsf(vVelocity.x) > 20.f)
        lineTransitionBlend = powf(lineTransitionBlend, 0.85f);

    const float descendDeltaY = oldNormal.y - normal.y;
    const bool bFlatToSlopeTransition = bLineSeamCandidate &&
        wasGround &&
        (oldNormal.y > 0.88f) &&
        (normal.y < 0.93f) &&
        (fabsf(_Contact.SignedDistance) <= kSurfaceLineSeamTransitionDepth);

    const bool bSeamAngleShift = bLineSeamCandidate &&
        wasGround &&
        (normalDot < kSurfaceLineSeamAngleShiftDot) &&
        (fabsf(_Contact.SignedDistance) <= kSurfaceLineSeamTransitionDepth);

    const bool bDownwardSlopeTransition = bLineSeamCandidate &&
        wasGround &&
        (descendDeltaY > kSurfaceLineSeamDownwardDelta) &&
        fabsf(_Contact.SignedDistance) <= kSurfaceLineSeamTransitionDepth;

    const bool bSteepDownSlopeTransition = bDownwardSlopeTransition && fabsf(vVelocity.x) > 30.f;

    if (bFlatToSlopeTransition)
    {
        lineTransitionBlend = max(lineTransitionBlend, kSurfaceLineSeamFlatToSlopeBlendFloor);
    }
    else if (bSeamAngleShift)
    {
        lineTransitionBlend = max(lineTransitionBlend, kSurfaceLineSeamAngleShiftBlendFloor);
    }
    else if (bDownwardSlopeTransition)
    {
        lineTransitionBlend = max(lineTransitionBlend, kSurfaceLineSeamDownwardBlendFloor);
    }
    else if (fabsf(vVelocity.x) > 20.f && descendDeltaY > 0.04f)
    {
        lineTransitionBlend = max(lineTransitionBlend, kSurfaceLineSeamEnterDownGradeBonus * Clamp01f(descendDeltaY * 2.f));
        if (fabsf(vVelocity.x) > 400.f)
            lineTransitionBlend = max(lineTransitionBlend, 1.f);
    }
    else if (bLineSeamCandidate && _Contact.TransitionSurface && fabsf(vVelocity.x) > 20.f)
    {
        lineTransitionBlend = max(lineTransitionBlend, kSurfaceLineSeamEntryBlendFloor);
    }

    if (bSteepDownSlopeTransition)
    {
        lineTransitionBlend = max(lineTransitionBlend, kSurfaceLineSeamSteepDownBlendFloor);
    }

    if (bRisingSlopeTransition)
        lineTransitionBlend = max(lineTransitionBlend, kSurfaceLineSeamRisingSlopeBlendFloor);

    if (bSlowSeamSpeed && !bSteepDownSlopeTransition && !bRisingSlopeTransition)
        lineTransitionBlend *= 0.4f;

    const bool bLineSeamStick =
        !jumpPressed &&
        bLineSeamCandidate &&
        (wasGround || m_SurfaceGroundHoldTime > 0.f) &&
        (GetAction() == ActionState::None) &&
        fabsf(_Contact.SignedDistance) <= kSurfaceLineSeamTransitionDepth &&
        (normalDot >= kSurfaceLineSeamKeepDot);

    const bool bHoldLineSeam = bLineSeamStick && !bSlowSeamSpeed &&
        !bDownwardSlopeTransition &&
        !bFlatToSlopeTransition &&
        !bSeamAngleShift;

    Vec2 curNormal = NormalizeSafeVec2(
        oldNormal * (1.f - normalBlend) + normal * normalBlend,
        normal);

    if (bLineSeamCandidate && _Contact.TransitionSurface)
    {
        const float blend = Clamp01f(lineTransitionBlend);
        curNormal = NormalizeSafeVec2(
            oldNormal * (1.f - blend) + normal * blend,
            (blend >= 0.5f) ? normal : oldNormal);
    }

    if (bLowSpeedInwardCircleDetach)
    {
        BlockInwardCircleAttach(inwardCircleCenter, inwardCircleRadius, kSurfaceInwardCircleLowSpeedDetachBlockTime);
        return false;
    }

    const float jumpSeparatingSpeed = DotVec2(vVelocity, curNormal);
    if (IsJump && !_Contact.WallLike && jumpSeparatingSpeed > kSurfaceJumpDetachSpeedMin)
    {
        SetIsGround(false);
        SetNormal(curNormal);
        SetGroundTangent(Vec2(curNormal.y, -curNormal.x));
        return true;
    }

    SetNormal(curNormal);

    const bool bProjectToStableSurface =
        ((_Contact.Circle &&
          _Contact.Attachable &&
          !_Contact.WallLike &&
          (!_Contact.InwardCircle || !bLowSpeedInwardCircleDetach) &&
          (!_Contact.TransitionSurface || _Contact.Circle) &&
          (wasGround || m_SurfaceGroundHoldTime > 0.f || _Contact.Circle)) ||
         (allowLineCorrection &&
          !_Contact.TransitionSurface &&
          fabsf(_Contact.SignedDistance) <= kSurfaceLineStableProjectionMaxDistance));

    bool bAppliedStableProjection = false;
    if (bProjectToStableSurface)
    {
        const float maxProjectionDistance =
            _Contact.Circle ? kSurfaceCircleProjectionMaxDistance : kSurfaceLineStableProjectionMaxDistance;
        const float correction = max(-maxProjectionDistance, min(maxProjectionDistance, -_Contact.SignedDistance));

        if (fabsf(correction) > 0.0001f)
        {
            Vec3 pos = Transform()->GetRelativePos();
            pos.x += curNormal.x * correction;
            pos.y += curNormal.y * correction;
            Transform()->SetRelativePos(pos);
            bAppliedStableProjection = true;
        }
    }

    const bool bAttachToLineSurface =
        allowLineCorrection &&
        _Contact.SeamBlendHasValue &&
        (wasGround || m_SurfaceGroundHoldTime > 0.f);

    if (!bAppliedStableProjection && bAttachToLineSurface && _Contact.SignedDistance > 0.f)
    {
        float snapDistance = _Contact.SignedDistance;
        const float maxSnapDistance =
            bRisingSlopeTransition ? kSurfaceLineSeamRisingSlopeSnapMaxDistance : kSurfaceLineAttachSnapMaxDistance;
        if (snapDistance > maxSnapDistance)
            snapDistance = maxSnapDistance;

        Vec3 pos = Transform()->GetRelativePos();
        pos.x -= curNormal.x * snapDistance;
        pos.y -= curNormal.y * snapDistance;
        Transform()->SetRelativePos(pos);
    }
    else if (!bAppliedStableProjection &&
             _Contact.SignedDistance < 0.f &&
             (!_Contact.Circle ? allowLineCorrection && fabsf(_Contact.SignedDistance) <= kSurfaceLinePenetrationPushMaxDistance : true))
    {
        float worldPush = -_Contact.SignedDistance + kSurfaceContactPushBias;
        float maxPush = kSurfaceContactMaxPush;
        if (bRisingSlopeTransition)
        {
            maxPush = min(maxPush, kSurfaceLineSeamRisingSlopePushLimit);
        }
        else
        {
            if (bFlatToSlopeTransition)
                maxPush = min(maxPush, kSurfaceLineSeamFlatToSlopePushLimit);
            if (bSeamAngleShift)
                maxPush = min(maxPush, kSurfaceLineSeamAngleShiftPushLimit);
            if (bSteepDownSlopeTransition)
                maxPush = min(maxPush, kSurfaceContactDownTransitionPushLimit);
        }
        if (worldPush > maxPush)
            worldPush = maxPush;

        Vec3 pos = Transform()->GetRelativePos();
        pos.x += curNormal.x * worldPush;
        pos.y += curNormal.y * worldPush;
        Transform()->SetRelativePos(pos);
    }

    Vec2 tangent = Vec2(curNormal.y, -curNormal.x);
    float rawVn = DotVec2(vVelocity, curNormal);
    const float incomingVn = rawVn;
    float vt = DotVec2(vVelocity, tangent);

    const bool wallLikeSurface = _Contact.WallLike || !_Contact.Attachable;
    const bool blockedByWallLikeSurface =
        wallLikeSurface &&
        IsBreakOrRollAction() &&
        (fabsf(curNormal.y) < 0.35f) &&
        (fabsf(incomingVn) > 20.f);

    if (rawVn < 0.f)
    {
        vVelocity.x -= curNormal.x * rawVn;
        vVelocity.y -= curNormal.y * rawVn;
        rawVn = 0.f;
    }

    const float stickMinSpeed = 500.f;
    const float normalEnterY = 0.65f;
    const float normalKeepY = 0.45f;
    const float vnEnterTolerance = 5.f;
    const float vnKeepTolerance = 80.f;
    const float normalThreshold = wasGround ? normalKeepY : normalEnterY;
    const float vnTolerance = wasGround ? vnKeepTolerance : vnEnterTolerance;

    bool speedStick = (fabsf(vt) > stickMinSpeed) && (curNormal.y > 0.2f);
    bool wallAttach = _Contact.Attachable && (fabsf(curNormal.x) > 0.85f);
    bool canStick =
        (rawVn <= vnTolerance) &&
        (curNormal.y > normalThreshold || speedStick || wallAttach);

    if (bLineSeamCandidate && _Contact.TransitionSurface && curNormal.y < normalThreshold && !wallAttach)
    {
        const bool seamCanStick =
            (fabsf(curNormal.y) >= kSurfaceLineSeamMinY) &&
            (normalDot >= kSurfaceLineSeamKeepDot) &&
            (fabsf(vt) <= kSurfaceLineSeamMoveSpeedGate);

        if (bFlatToSlopeTransition || bDownwardSlopeTransition || bSeamAngleShift)
            canStick = true;
        else
            canStick = bHoldLineSeam && seamCanStick;
    }

    if (_Contact.Circle && _Contact.Attachable && !jumpPressed && !blockedByWallLikeSurface)
    {
        const bool bInwardCircleLowerHalfContact =
            inwardCircleContact &&
            (inwardCircleLowerHalfRatio >= 0.12f) &&
            (incomingVn <= vnKeepTolerance);
        const bool circleCanStick =
            (fabsf(_Contact.SignedDistance) <= kSurfaceCircleProjectionMaxDistance) &&
            (bInwardCircleLowerHalfContact ||
             ((!_Contact.InwardCircle || inwardCircleUpperHalfRatio < kSurfaceInwardCircleLowSpeedDetachUpperHalfMin) &&
              ((curNormal.y >= kSurfaceCircleStickMinY) ||
               wasGround ||
               m_SurfaceGroundHoldTime > 0.f)) ||
             fabsf(vt) >= kSurfaceCircleStickSpeedMin);

        if (circleCanStick)
            canStick = true;
    }

    if (blockedByWallLikeSurface)
    {
        if (GetAction() == ActionState::Break)
            RequestBreakWallStop();
        else
            StopBlockedAction();

        if (GetAction() == ActionState::Break)
        {
            Vec2 blockedVelocity = GetVelocity();
            blockedVelocity.x = 0.f;
            SetVelocity(blockedVelocity);
            return true;
        }
    }

    if (canStick)
    {
        RefreshSurfaceGroundHold(_Contact.Circle ? kSurfaceCircleGroundHoldTime : 0.08f);
        SetIsGround(true);
        SetNormal(curNormal);
        SetGroundTangent(tangent);

        if (curNormal.y > 0.f)
        {
            float friction = 100.f;

            if (vt > 0.f)
            {
                vt -= friction * DT;
                if (vt < 0.f) vt = 0.f;
            }
            else if (vt < 0.f)
            {
                vt += friction * DT;
                if (vt > 0.f) vt = 0.f;
            }
        }

        vVelocity = tangent * vt;
    }
    else
    {
        const bool definiteAirborne =
            (rawVn > vnKeepTolerance) ||
            (!wallAttach && curNormal.y < 0.2f && fabsf(vt) < stickMinSpeed);

        if (definiteAirborne)
            SetIsGround(false);

        SetNormal(curNormal);
        SetGroundTangent(tangent);

        if (curNormal.y <= 0.65f && fabsf(vt) < stickMinSpeed)
        {
            if (vVelocity.y > 0.f)
                vVelocity.y = 0.f;
        }
    }

    return true;
}

void CPlayerScript::Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    Ptr<CBlockPushingScript> pBlock = _OtherCollider->GetOwner()->GetScript<CBlockPushingScript>();
    if (pBlock == nullptr)
        return;

    const int pushDir = pBlock->GetIsPushing();
    if (pushDir == 1 || pushDir == -1)
    {
        m_bPushContact = true;
        m_PushContactDir = pushDir;
        return;
    }

    m_bPushContact = false;
    m_PushContactDir = 0;
}

void CPlayerScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OtherCollider == nullptr)
        return;

    auto pTile = _OtherCollider->GetOwner()->GetScript<CTileScript>();
    auto pSurface = _OtherCollider->GetOwner()->GetScript<CSurfaceScript>();
    auto pBlock = _OtherCollider->GetOwner()->GetScript<CBlockScript>();
    auto pBlockMove = _OtherCollider->GetOwner()->GetScript<CBlockMovingScript>();
    auto pBlockPush = _OtherCollider->GetOwner()->GetScript<CBlockPushingScript>();

    if (pSurface != nullptr)
        RegisterActiveSurface(_OtherCollider->GetOwner());

    const bool bCountsAsGroundOverlap =
        (pTile != nullptr) ||
        (pBlock != nullptr) ||
        (pBlockMove != nullptr) ||
        (pBlockPush != nullptr) ||
        (pSurface != nullptr && pSurface->IsAttachable());

    if (!bCountsAsGroundOverlap)
        return;

    ++m_TileOverlapCount;
}

void CPlayerScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OtherCollider == nullptr)
        return;

    auto pBlockPush = _OtherCollider->GetOwner()->GetScript<CBlockPushingScript>();
    if (pBlockPush != nullptr)
    {
        m_bPushContact = false;
        m_PushContactDir = 0;
        m_bPushing = false;
        m_PushingDir = 0;
        if (m_Action == ActionState::Pushing)
            m_Action = ActionState::None;
    }

    auto pTile = _OtherCollider->GetOwner()->GetScript<CTileScript>();
    auto pSurface = _OtherCollider->GetOwner()->GetScript<CSurfaceScript>();
    auto pBlock = _OtherCollider->GetOwner()->GetScript<CBlockScript>();
    auto pBlockMove = _OtherCollider->GetOwner()->GetScript<CBlockMovingScript>();

    if (pSurface != nullptr)
        UnregisterActiveSurface(_OtherCollider->GetOwner());

    const bool bCountsAsGroundOverlap =
        (pTile != nullptr) ||
        (pBlock != nullptr) ||
        (pBlockMove != nullptr) ||
        (pBlockPush != nullptr) ||
        (pSurface != nullptr && pSurface->IsAttachable());

    if (!bCountsAsGroundOverlap)
        return;

    --m_TileOverlapCount;
    if (m_TileOverlapCount < 0)
        m_TileOverlapCount = 0;
}

void CPlayerScript::SaveToLevelFile(FILE* _File)
{
    //fwrite(&m_Dir, sizeof(Vec3), 1, _File);
}

void CPlayerScript::LoadFromLevelFile(FILE* _File)
{
    //fread(&m_Dir, sizeof(Vec3), 1, _File);
}

