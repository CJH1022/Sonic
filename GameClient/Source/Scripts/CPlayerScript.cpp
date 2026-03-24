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
#include "CBlockScript.h"

#include <cmath> // fabsf
#include "CBlockPushingScript.h"
#include "AssetMgr.h"

CPlayerScript::CPlayerScript()
    : CScript(SCRIPT_TYPE::PLAYERSCRIPT)
{
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

    FlipbookRender()->SetFlipbook(20, LOAD(AFlipbook, L"Flipbook\\Sonic_Pushing.flip"));
    m_LastTickPosX = Transform()->GetRelativePos().x;
    m_LastFrameDeltaX = 0.f;
}

CPlayerScript::~CPlayerScript()
{
}

CPlayerScript::PlayerInput CPlayerScript::ReadInput() const
{
    PlayerInput in;
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
    const float curPosX = Transform()->GetRelativePos().x;
    m_LastFrameDeltaX = curPosX - m_LastTickPosX;
    m_LastTickPosX = curPosX;

    const float dt = DT;
    const PlayerInput in = ReadInput();

    // 1) 상태 전이(결정)
    ResolveTransitions(in, dt);

    // 2) 물리/이동(실행)
    Simulate(in, dt);

    // 3) 회전 상태 에니메이션 갱신
    UpdateGroundRotation();

    // 4) 타이머 갱신 (속도/상태 기반)
    UpdateTimers(dt);

    // 5) 애니메이션(표현) - 한 곳에서 우선순위로만 결정
    UpdateAnimation(dt);

    // 디버그는 네 코드처럼 필요하면 여기서
    if (in.spacePressed)
    {
        DrawDebugRect(Transform()->GetWorldMat(), Vec4(1.f, 0.f, 0.f, 1.f), 2.f);
    }
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

    if ((m_Action == ActionState::Roll) && IsGround)
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

    // TODO: Attack/Roll 종료는 m_ActionTime 또는 애니 이벤트로 처리
    // if (m_Action == ActionState::Attack) ...
    if (m_Action == ActionState::Roll)
        return;

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
    if (!IsGround) return false;

    // Dash는 예외로 허용
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

void CPlayerScript::StartJump()
{
    IsJump = true;
    vVelocity += vNormal * 600.f;
    IsGround = false;
    m_Action = ActionState::None;
    m_BreakUngroundedTime = 0.f;
    m_IdleTime = 0.f;
}

void CPlayerScript::StartSkillDash()
{
    const float MaxSpeed = 600.f;

    m_Action = ActionState::SkillDash;
    m_ActionTime = 0.f;
    m_Pose = PoseState::None;

    vVelocity.x = MaxSpeed * (float)m_Facing;
}

void CPlayerScript::Rolling()
{
    m_Action = ActionState::Roll;
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

    if (!lockFacing)
    {
        float vt = vVelocity.Dot(vTangent);
    }

    // 실제 스케일 적용 (m_Facing 결정권은 유지)
    if (m_Facing == 1)
    {
        vScale.x = fabsf(vScale.x);
        GetOwner()->GetChild(0)->Transform()->SetRelativeRot(Vec3(0.f, 0.f, 0.f));
    }
    else
    {
        vScale.x = -fabsf(vScale.x);
        GetOwner()->GetChild(0)->Transform()->SetRelativeRot(Vec3(0.f, 0.f, XM_PI));
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
            Transform()->SetRelativeRot(Vec3(0.f, 0.f, fTargetRot));
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
            Transform()->SetRelativeRot(Vec3(0.f, 0.f, fTargetRot));
        }
    }
}

void CPlayerScript::UpdateAnimation(float dt)
{
    const float MaxSpeed = 300.f;
    const float absSpeed = IsGround ? fabsf(vVelocity.Dot(vTangent)) : fabsf(vVelocity.x);
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
        FlipbookRender()->Play(20, 6.f, -1);
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
    auto pBlock = _OtherCollider->GetOwner()->GetScript<CBlockScript>();
    auto pBlockPush = _OtherCollider->GetOwner()->GetScript<CBlockPushingScript>();
    if (pTile == nullptr && pBlock == nullptr && pBlockPush == nullptr)
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
    auto pBlock = _OtherCollider->GetOwner()->GetScript<CBlockScript>();
    if (pTile == nullptr && pBlock == nullptr && pBlockPush == nullptr)
        return;

    --m_TileOverlapCount;
    if (m_TileOverlapCount < 0)
        m_TileOverlapCount = 0;

    if (m_TileOverlapCount == 0)
    {
        IsGround = false;
    }
}
