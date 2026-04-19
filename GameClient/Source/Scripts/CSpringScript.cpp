#include "pch.h"
#include "CSpringScript.h"
#include "TimeMgr.h"
#include "CFlipbookRender.h"
#include "CCollider2D.h"
#include "GameObject.h"
#include "CTransform.h"
#include "LevelMgr.h"
#include "CPlayerScript.h"
#include "func.h"

CSpringScript::CSpringScript()
    : CScript(SCRIPT_TYPE::SPRINGSCRIPT)
    , vVelocity(Vec2(0.f, 0.f))
{
}

CSpringScript::~CSpringScript()
{
}

void CSpringScript::Begin()
{
    Vec3 vRot = GetOwner()->Transform()->GetRelativeRot();
    vVelocity = Vec2(0.f, 0.f);

    if(vRot.z == 0.f) // (RIGHT)
        vVelocity.x = 1200.f;

    if(vRot.z == XM_PIDIV2) //(위쪽)
        vVelocity.y = 1200.f;

    if(vRot.z == XM_PI) // (왼쪽)
        vVelocity.x = -1200.f;

    if(vRot.z == -XM_PIDIV2) // (아래쪽)
        vVelocity.y = -1200.f;

	Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CSpringScript::BeginOverlap);
}


void CSpringScript::Tick()
{
}

void CSpringScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OtherCollider->GetOwner()->GetName() == L"Player")
    {
        Ptr<CPlayerScript> pScript = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
        if (pScript == nullptr) return; // 터짐 방지

        // 1. Transform 위치와 스케일 가져오기
        Vec3 myTransformPos = Transform()->GetRelativePos();
        Vec3 myTransformScale = Transform()->GetRelativeScale();
        Vec3 vRot = Transform()->GetRelativeRot();

        Vec3 otherTransformPos = _OtherCollider->GetOwner()->Transform()->GetRelativePos();
        Vec3 otherTransformScale = _OtherCollider->GetOwner()->Transform()->GetRelativeScale();

        // 2. Collider의 Offset과 Scale 가져오기
        Vec2 myColOffset = _OwnCollider->GetOffset();
        Vec2 myColScale = _OwnCollider->GetScale();

        Vec2 otherColOffset = _OtherCollider->GetOffset();
        Vec2 otherColScale = _OtherCollider->GetScale();

        // 3. ✨ [핵심] 실제 충돌체의 중심점(Center) 계산 (Transform 위치 + Collider Offset)
        Vec2 myCenter = Vec2(myTransformPos.x + myColOffset.x, myTransformPos.y + myColOffset.y);
        Vec2 otherCenter = Vec2(otherTransformPos.x + otherColOffset.x, otherTransformPos.y + otherColOffset.y);

        // 4. ✨ [핵심] 실제 충돌체의 크기(Size) 계산
        // (엔진에 따라 ColliderScale이 절대값이면 myColScale.y 만 사용하세요. 보통은 TransformScale에 곱해집니다.)
        float myFinalWidth = fabs(myTransformScale.x * myColScale.x);
        float myFinalHeight = fabs(myTransformScale.y * myColScale.y);
        float otherFinalWidth = fabs(otherTransformScale.x * otherColScale.x);
        float otherFinalHeight = fabs(otherTransformScale.y * otherColScale.y);

        // 5. '위에서 밟았는지' 판정 (Y축이 위로 갈수록 +인 기준)
        // 충돌 중이므로 거리를 재는 것이 아니라, "플레이어의 발끝(Bottom)"이 "스프링의 중심"보다 위에 있는지를 체크합니다.
        bool bTrigger = false;

        // 5. 방향에 따른 충돌 면 판정

        if(vRot.z == XM_PIDIV2) //(위쪽)
            bTrigger = (otherCenter.y - (otherFinalHeight / 2.f)) > myCenter.y;

        if (vRot.z == -XM_PIDIV2) // (아래쪽)
            bTrigger = (otherCenter.y + (otherFinalHeight / 2.f)) < myCenter.y;

        if (vRot.z == 0.f) // (오른쪽)
        {
            bTrigger = (otherCenter.x + (otherFinalWidth / 2.f)) > myCenter.x;
            pScript->SetFacing(1);
        }
            
        if (vRot.z == XM_PI) // (왼쪽)
        {
            bTrigger = (otherCenter.x - (otherFinalWidth / 2.f)) < myCenter.x;
            pScript->SetFacing(-1);
        }
        
        if (bTrigger)
        {
            PlayGameSFX(L"Sound\\스프링 닿았을때.wav", 0.8f, true);
            pScript->SetVelocity(vVelocity);

            // 스프링 방향에 따라 Facing 강제 설정
            int dir = (vVelocity.x >= 0.f) ? 1 : -1;

            // 플레이어의 공중 상태 및 스프링 점프 플래그 강제 활성화
            pScript->SetIsGround(false);
            pScript->SetSpringJumpState(ActionState::Spring, dir); // 이 함수를 호출하여 bIsSpringJump를 true로 만듦

            if (GetOwner()->FlipbookRender() != nullptr)
            {
                GetOwner()->FlipbookRender()->Play(0, 0, 20.f, 0);
            }
        }
    }
}
