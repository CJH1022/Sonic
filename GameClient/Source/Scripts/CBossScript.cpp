#include "pch.h"
#include "CBossScript.h"

#include "AssetMgr.h"
#include "CCollider2D.h"
#include "CBackScript.h"
#include "Engine.h"
#include "CFireScript.h"
#include "CFlipbookRender.h"
#include "CMeshRender.h"
#include "CTransform.h"
#include "GameObject.h"
#include <TimeMgr.h>
#include <cmath>
#include <algorithm>
#include "RenderMgr.h"

#include "Source/Scripts/CKnockbackScript.h"
namespace
{
    constexpr int kBossMoveFlipbookIndex = 0;
    constexpr int kBossStateCount = 8;
    const Vec3 kBossFireOffset = Vec3(-120.f, 10.f, 0.f);
    const Vec2 kBossFireBaseScale = Vec2(150.f, 110.f);
    const Vec2 kBossFireDir = Vec2(-1.f, 0.f);
    constexpr float kBossFireLength = 250.f;
    constexpr float kBossFireLifeTime = 0.75f;
    constexpr float kBossAttack1GapTime = 1.0f;
    constexpr int kBossAttack1RepeatCount = 2;
    constexpr float kBossAttack1SequenceDuration
        = (kBossFireLifeTime * kBossAttack1RepeatCount)
        + (kBossAttack1GapTime * (kBossAttack1RepeatCount - 1));
    constexpr float kBossHitFlashDuration = 0.12f;
    constexpr float kBossBodyBlinkDuration = 1.0f;
    constexpr float kBossBodyBlinkInterval = 0.05f;
    const Vec3 kBossHitFlashBaseScale = Vec3(95.f, 95.f, 1.f);
    const Vec3 kBossHitFlashScaleGrow = Vec3(35.f, 35.f, 0.f);
    const wchar_t* kBossHitFlashName = L"BossHitFlash";


    constexpr float kBossSpeed = 200.f;

    int ClampBossStateIndex(int _StateIndex)
    {
        if (_StateIndex < 0)
            return 0;

        if (_StateIndex >= kBossStateCount)
            return kBossStateCount - 1;

        return _StateIndex;
    }

    GameObject* FindBossFireObject(GameObject* _Owner)
    {
        if (_Owner == nullptr)
            return nullptr;

        const vector<Ptr<GameObject>>& children = _Owner->GetChild();
        for (const auto& child : children)
        {
            if (child == nullptr || child->IsDead())
                continue;

            Ptr<CFireScript> pFireScript = child->GetScript<CFireScript>();
            if (pFireScript != nullptr)
                return child.Get();
        }

        return nullptr;
    }

    GameObject* FindChildByName(GameObject* _Owner, const wchar_t* _Name)
    {
        if (_Owner == nullptr)
            return nullptr;

        const vector<Ptr<GameObject>>& children = _Owner->GetChild();
        for (const auto& child : children)
        {
            if (child != nullptr && !child->IsDead() && child->GetName() == _Name)
                return child.Get();
        }

        return nullptr;
    }

    GameObject* FindBossHitFlashObject(GameObject* _Owner)
    {
        return FindChildByName(_Owner, kBossHitFlashName);
    }

    CFireScript* CreateBossFireScript(GameObject* _Owner)
    {
        if (_Owner == nullptr)
            return nullptr;

        GameObject* pBossFireObject = FindBossFireObject(_Owner);
        if (pBossFireObject != nullptr)
        {
            Ptr<CFireScript> pFireScript = pBossFireObject->GetScript<CFireScript>();
            if (pFireScript != nullptr)
                return pFireScript.Get();
        }

        Ptr<GameObject> pBossFire = new GameObject;
        pBossFire->SetName(L"BossFire");
        pBossFire->AddComponent(new CTransform);

        Ptr<CFireScript> pNewFireScript = new CFireScript;
        pNewFireScript->SetLoop(1);
        pNewFireScript->SetMoveSpeed(0.f);
        pNewFireScript->SetLifeTime(kBossFireLifeTime);
        pNewFireScript->SetFlameLength(kBossFireLength);
        pNewFireScript->SetBaseScale(kBossFireBaseScale);
        pNewFireScript->SetMoveDir(kBossFireDir);
        pNewFireScript->SetActive(true);
        pBossFire->AddComponent(pNewFireScript.Get());

        pBossFire->Transform()->SetIndependentScale(true);
        pBossFire->Transform()->SetRelativePos(kBossFireOffset);
        _Owner->AddChild(pBossFire);

        return pNewFireScript.Get();
    }

    void DestroyBossFireObject(GameObject* _Owner)
    {
        GameObject* pBossFireObject = FindBossFireObject(_Owner);
        if (pBossFireObject != nullptr)
            pBossFireObject->Destroy();
    }

    GameObject* CreateBossHitFlashObject(GameObject* _Owner)
    {
        if (_Owner == nullptr)
            return nullptr;

        GameObject* pExistingFlashObject = FindBossHitFlashObject(_Owner);
        if (pExistingFlashObject != nullptr)
            return pExistingFlashObject;

        Ptr<GameObject> pHitFlash = new GameObject;
        pHitFlash->SetName(kBossHitFlashName);
        pHitFlash->AddComponent(new CTransform);
        pHitFlash->AddComponent(new CMeshRender);
        pHitFlash->Transform()->SetIndependentScale(true);
        pHitFlash->Transform()->SetRelativeScale(kBossHitFlashBaseScale);

        pHitFlash->MeshRender()->SetMesh(FIND(AMesh, L"RectMesh"));
        pHitFlash->MeshRender()->SetMaterial(FIND(AMaterial, L"BossHitFlashMtrl"));

        _Owner->AddChild(pHitFlash);

        Ptr<AMaterial> pFlashMtrl = pHitFlash->MeshRender()->CreateDynamicMaterial();
        if (pFlashMtrl != nullptr)
            pFlashMtrl->SetScalar(VEC4_0, Vec4(1.f, 1.f, 1.f, 0.f));

        return pHitFlash.Get();
    }
}

CBossScript::CBossScript()
    : CScript(SCRIPT_TYPE::BOSSSCRIPT)
    , m_Life(3)
    , m_bCameraFix(false)
    , m_State(BOSS_STATE::IDLE)
    , m_MoveFlipFPS(60.f)
    , m_IdleTime(0.f)
    , m_AttackStateTime(0.f)
    , m_AttackBurstTime(0.f)
    , m_AttackBurstCount(0)
    , m_AttackBurstActive(false)
    , m_HitFlashTime(0.f)
    , m_BodyBlinkTime(0.f)
    , m_Dir(-1)
    , m_PatternPhase(0)
    , m_MoveStep(0)
{
    AddScriptParam(SCRIPT_PARAM::INT, &m_Life, L"Life", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_MoveFlipFPS, L"MoveFlipFPS", false, 1.f);
}

CBossScript::~CBossScript()
{
}

void CBossScript::Begin()
{
    if (GetOwner() == nullptr)
        return;

    GetOwner()->Transform()->SetRelativeScale(Vec3(250.f, 250.f, 1.f));

    if (FindChildByName(GetOwner(), L"BOSS_ATTACKED") == nullptr)
    {
        Ptr<GameObject> pAttacked = new GameObject;
        pAttacked->SetName(L"BOSS_ATTACKED");
        pAttacked->AddComponent(new CTransform);
        pAttacked->AddComponent(new CCollider2D);
        pAttacked->Transform()->SetIndependentScale(true);

        auto* pBack = new CBackScript;
        pBack->SetBackPower(Vec2(400.f, 220.f));
        pAttacked->AddComponent(pBack);

        pAttacked->Transform()->SetRelativePos(Vec3(0.f, 20.f, 0.f));
        pAttacked->Transform()->SetRelativeScale(Vec3(250.f, 200.f, 1.f));

        GetOwner()->AddChild(pAttacked);
    }

    if (FindChildByName(GetOwner(), L"BOSS_Hit") == nullptr)
    {
        Ptr<GameObject> pHit = new GameObject;
        pHit->SetName(L"BOSS_Hit");
        pHit->AddComponent(new CTransform);
        pHit->AddComponent(new CCollider2D);
        pHit->Transform()->SetIndependentScale(true);

        auto* pKnock = new CKnockbackScript;
        pKnock->SetKnockBackPower(Vec2(350.f, 220.f));
        pHit->AddComponent(pKnock);

        pHit->Transform()->SetRelativePos(Vec3(0.f, -100.f, 0.f));
        pHit->Transform()->SetRelativeScale(Vec3(250.f, 40.f, 1.f));

        GetOwner()->AddChild(pHit);
    }

    if (GetOwner()->Collider2D() != nullptr)
    {
        // Keep the prefab root collider out of play; child colliders own the actual hit logic.
        GetOwner()->Collider2D()->SetOffset(Vec2(0.f, -100000.f));
        GetOwner()->Collider2D()->SetScale(Vec2(0.01f, 0.01f));
    }

    if (FlipbookRender() != nullptr)
    {
        Ptr<AFlipbook> pMoveFlipbook = LOAD(AFlipbook, L"Flipbook\\Boss_Move.flip");
        if (pMoveFlipbook != nullptr)
        {
            FlipbookRender()->SetFlipbook(kBossMoveFlipbookIndex, pMoveFlipbook);
            FlipbookRender()->Play(kBossMoveFlipbookIndex, m_MoveFlipFPS, -1);
        }

        Ptr<AMaterial> pBossMtrl = FlipbookRender()->CreateDynamicMaterial();
        if (pBossMtrl != nullptr)
            pBossMtrl->SetScalar(VEC4_0, Vec4(1.f, 1.f, 1.f, 0.f));
    }


    CreateBossHitFlashObject(GetOwner());
    if (Collider2D() != nullptr)
        Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CBossScript::BeginOverlap);

    ResetAttackSequence();
    if (m_State == BOSS_STATE::ATTACK1)
        StartAttackBurst();

    UpdateFacing();
}

void CBossScript::Tick()
{
    if (m_Life < 1) m_State = BOSS_STATE::DEAD;

    // 상태에 따라 업데이트 함수 분배
    switch (m_State)
    {
    case BOSS_STATE::IDLE:
        TickIdle();
        break;
    case BOSS_STATE::MOVE:
        TickMove();
        if (FlipbookRender() != nullptr
            && FlipbookRender()->GetFlipbook(kBossMoveFlipbookIndex) != nullptr)
            FlipbookRender()->Play(kBossMoveFlipbookIndex, m_MoveFlipFPS, -1);
        break;
    case BOSS_STATE::ATTACK1:
        TickAttack1();
        break;
    case BOSS_STATE::DEAD:
        GetOwner()->Destroy();
        break;
    }

    UpdateFacing();
    TickHitFlash();
    TickBodyBlink();
}

void CBossScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
}

void CBossScript::ApplyDamage(int _Damage, Vec3 _HitWorldPos)
{
    if (_Damage <= 0)
        return;

    if (GetOwner() == nullptr || GetOwner()->IsDead())
        return;

    if (m_State == BOSS_STATE::DEAD)
        return;

    m_Life -= _Damage;
    if (m_Life < 0)
        m_Life = 0;

    m_BodyBlinkTime = kBossBodyBlinkDuration;
    TriggerHitFlash(_HitWorldPos);
}

void CBossScript::SetState(BOSS_STATE _State)
{
    if (m_State == _State)
        return;

    m_State = _State;
    m_IdleTime = 0.f;
    ResetAttackSequence();

    if (m_State == BOSS_STATE::ATTACK1)
        StartAttackBurst();

}

void CBossScript::SetStateByIndex(int _StateIndex)
{
    SetState((BOSS_STATE)ClampBossStateIndex(_StateIndex));
}

bool CBossScript::TryGetMovementBounds(float _Margin,
                                       float& _OutLeftTargetX,
                                       float& _OutRightTargetX,
                                       float& _OutUpTargetY,
                                       float& _OutDownTargetY) const
{
    Ptr<CCamera> pCamera = RenderMgr::GetInst()->GetPOVCamera();
    if (pCamera == nullptr)
        pCamera = RenderMgr::GetInst()->GetEditorCamera();

    if (pCamera == nullptr || pCamera->Transform() == nullptr)
        return false;

    const Vec3 cameraWorldPos = pCamera->Transform()->GetWorldPos();
    const Vec2 resol = Engine::GetInst()->GetResolution();

    _OutLeftTargetX = cameraWorldPos.x - (resol.x * 0.5f) + _Margin;
    _OutRightTargetX = cameraWorldPos.x + (resol.x * 0.5f) - _Margin;

    // 🚨 [수정됨] 엔진 좌표계에 맞춰 위쪽이 (+), 아래쪽이 (-)가 됩니다!
    _OutUpTargetY = cameraWorldPos.y + (resol.y * 0.5f) - _Margin;
    _OutDownTargetY = cameraWorldPos.y - (resol.y * 0.5f) + _Margin;
    return true;
}

void CBossScript::UpdateFacing()
{
    GameObject* pOwner = GetOwner();
    if (pOwner == nullptr || pOwner->Transform() == nullptr)
        return;

    Vec3 ownerScale = pOwner->Transform()->GetRelativeScale();
    float scaleX = fabsf(ownerScale.x);
    if (scaleX <= 0.0001f)
        scaleX = 250.f;

    ownerScale.x = (m_Dir < 0) ? scaleX : -scaleX;
    pOwner->Transform()->SetRelativeScale(ownerScale);

    GameObject* pBossFireObject = FindBossFireObject(pOwner);
    if (pBossFireObject == nullptr || pBossFireObject->Transform() == nullptr)
        return;

    const float fireDirSign = (m_Dir < 0) ? -1.f : 1.f;
    Vec3 fireOffset = kBossFireOffset;
    fireOffset.x = fabsf(kBossFireOffset.x) * fireDirSign;
    pBossFireObject->Transform()->SetRelativePos(fireOffset);

    Ptr<CFireScript> pFireScript = pBossFireObject->GetScript<CFireScript>();
    if (pFireScript != nullptr)
        pFireScript->SetMoveDir(Vec2(fireDirSign, 0.f));
}

void CBossScript::MoveUp()
{
    SetState(BOSS_STATE::MOVE);

    const float margin = 150.f;
    float leftTargetX = 0.f;
    float rightTargetX = 0.f;
    float upTargetY = 0.f;
    float downTargetY = 0.f;
    if (!TryGetMovementBounds(margin, leftTargetX, rightTargetX, upTargetY, downTargetY))
        return;
   
    Vec3 vMyPos = Transform()->GetRelativePos();
    vMyPos.y += kBossSpeed * DT;

    if (vMyPos.y >= upTargetY)
    {
        vMyPos.y = upTargetY;
        if (m_Dir == -1)
        {
            MoveLeft();
            return;
        }
        else
        {
            MoveRight();
            return;
        }
    }
    Transform()->SetRelativePos(vMyPos);
}

void CBossScript::MoveDown()
{
    SetState(BOSS_STATE::MOVE);

    const float margin = 150.f;
    float leftTargetX = 0.f;
    float rightTargetX = 0.f;
    float upTargetY = 0.f;
    float downTargetY = 0.f;
    if (!TryGetMovementBounds(margin, leftTargetX, rightTargetX, upTargetY, downTargetY))
        return;

    Vec3 vMyPos = Transform()->GetRelativePos();
    vMyPos.y -= kBossSpeed * DT;

    if (vMyPos.y <= downTargetY)
    {
        vMyPos.y = downTargetY;
        SetState(BOSS_STATE::IDLE);
        return;
    }
    Transform()->SetRelativePos(vMyPos);
}

void CBossScript::MoveLeft()
{
    SetState(BOSS_STATE::MOVE);

    const float margin = 150.f;
    float leftTargetX = 0.f;
    float rightTargetX = 0.f;
    float upTargetY = 0.f;
    float downTargetY = 0.f;
    if (!TryGetMovementBounds(margin, leftTargetX, rightTargetX, upTargetY, downTargetY))
        return;

    Vec3 vMyPos = Transform()->GetRelativePos();
    vMyPos.x -= kBossSpeed * DT;

    if (vMyPos.x < leftTargetX)
    {
        m_Dir = 1;
        vMyPos.x = leftTargetX;
        MoveDown();
        return;
    }

    m_IdleTime += DT;

    float frequency = 10.0f; 
    float amplitude = 30.0f; 


    vMyPos.y += sinf(m_IdleTime * frequency) * amplitude * DT;
    Transform()->SetRelativePos(vMyPos);
}

void CBossScript::MoveRight()
{
    SetState(BOSS_STATE::MOVE);

    const float margin = 150.f;
    float leftTargetX = 0.f;
    float rightTargetX = 0.f;
    float upTargetY = 0.f;
    float downTargetY = 0.f;
    if (!TryGetMovementBounds(margin, leftTargetX, rightTargetX, upTargetY, downTargetY))
        return;

    Vec3 vMyPos = Transform()->GetRelativePos();
    vMyPos.x += kBossSpeed * DT;

    if (vMyPos.x > rightTargetX)
    {
        m_Dir = -1;
        vMyPos.x = rightTargetX;
        MoveDown();
        return;
    }


    m_IdleTime += DT;

    float frequency = 10.0f; 
    float amplitude = 30.0f; 

    vMyPos.y += sinf(m_IdleTime * frequency) * amplitude * DT;
    Transform()->SetRelativePos(vMyPos);
}

void CBossScript::SaveToLevelFile(FILE* _File)
{
    int stateIndex = (int)m_State;
    fwrite(&m_Life, sizeof(int), 1, _File);
    fwrite(&m_bCameraFix, sizeof(bool), 1, _File);
    fwrite(&stateIndex, sizeof(int), 1, _File);
    fwrite(&m_MoveFlipFPS, sizeof(float), 1, _File);
}

void CBossScript::LoadFromLevelFile(FILE* _File)
{
    int stateIndex = 0;
    fread(&m_Life, sizeof(int), 1, _File);
    fread(&m_bCameraFix, sizeof(bool), 1, _File);
    fread(&stateIndex, sizeof(int), 1, _File);
    fread(&m_MoveFlipFPS, sizeof(float), 1, _File);
    SetStateByIndex(stateIndex);
}

void CBossScript::ResetAttackSequence()
{
    DestroyBossFireObject(GetOwner());
    m_AttackStateTime = 0.f;
    m_AttackBurstTime = 0.f;
    m_AttackBurstCount = 0;
    m_AttackBurstActive = false;
}

void CBossScript::StartAttackBurst()
{
    if (m_State != BOSS_STATE::ATTACK1 || m_AttackBurstActive)
        return;

    m_AttackBurstTime = 0.f;
    m_AttackBurstActive = true;
    CreateBossFireScript(GetOwner());
    UpdateFacing();
}

void CBossScript::StopAttackBurst()
{
    DestroyBossFireObject(GetOwner());
    m_AttackBurstActive = false;
    m_AttackBurstTime = 0.f;
}

void CBossScript::TickIdle()
{
    m_IdleTime += DT;

    // 1. 제자리에서 위아래로 둥둥 떠다니기
    float frequency = 10.0f;
    float amplitude = 20.0f;
    Vec3 pos = Transform()->GetRelativePos();
    pos.y += sinf(m_IdleTime * frequency) * amplitude * DT;
    Transform()->SetRelativePos(pos);

    // 2. 2초 동안 대기했다면, 다음 패턴으로 전이!
    if (m_IdleTime > 2.0f)
    {
        if (m_PatternPhase == 0) // 패턴 0: 왼쪽으로 이동
        {
            m_Dir = -1;
            m_MoveStep = 0;
            m_PatternPhase = 1; // 다음엔 패턴 1을 실행해라
            SetState(BOSS_STATE::MOVE);
        }
        else if (m_PatternPhase == 1) // 패턴 1: 왼쪽에서 공격
        {
            m_PatternPhase = 2; // 다음엔 패턴 2를 실행해라
            SetState(BOSS_STATE::ATTACK1);
        }
        else if (m_PatternPhase == 2) // 패턴 2: 오른쪽으로 이동
        {
            m_Dir = 1;
            m_MoveStep = 0;
            m_PatternPhase = 3; // 다음엔 패턴 3을 실행해라
            SetState(BOSS_STATE::MOVE);
        }
        else if (m_PatternPhase == 3) // 패턴 3: 오른쪽에서 공격
        {
            m_PatternPhase = 0; // 다음엔 처음(패턴 0)으로 돌아가라
            SetState(BOSS_STATE::ATTACK1);
        }
    }
}
void CBossScript::TickMove()
{
    Vec3 pos = Transform()->GetRelativePos();
    const float margin = 150.f;
    float leftTargetX = 0.f, rightTargetX = 0.f, upTargetY = 0.f, downTargetY = 0.f;
    if (!TryGetMovementBounds(margin, leftTargetX, rightTargetX, upTargetY, downTargetY))
        return;

    // [Step 0: 위로 이동]
    if (m_MoveStep == 0)
    {
        pos.y += kBossSpeed * DT;
        if (pos.y >= upTargetY)
        {
            pos.y = upTargetY;
            m_MoveStep = 1;
        }
    }
    // [Step 1: 좌우로 이동]
    else if (m_MoveStep == 1)
    {
        if (m_Dir == -1) // 왼쪽으로
        {
            pos.x -= kBossSpeed * DT;
            if (pos.x <= leftTargetX) { pos.x = leftTargetX; m_MoveStep = 2; m_Dir = 1; }
        }
        else if (m_Dir == 1) // 오른쪽으로
        {
            pos.x += kBossSpeed * DT;
            if (pos.x >= rightTargetX) { pos.x = rightTargetX; m_MoveStep = 2; m_Dir = -1; }
        }
    }
    // [Step 2: 아래로 이동]
    else if (m_MoveStep == 2)
    {
        pos.y -= kBossSpeed * DT;
        if (pos.y <= downTargetY)
        {
            pos.y = downTargetY;

            // 🚨 누가 다음에 실행될지 고민하지 않고, 무조건 IDLE로 던집니다.
            SetState(BOSS_STATE::IDLE);
        }
    }

    Transform()->SetRelativePos(pos);
}
void CBossScript::TickAttack1()
{
    m_AttackStateTime += DT;

    // 1. 전체 공격 지속 시간이 끝났을 때
    if (m_AttackStateTime >= (kBossAttack1SequenceDuration + 0.1f))
    {
        SetState(BOSS_STATE::IDLE); // 공격 끝 -> 대기!
        return;
    }

    if (m_AttackBurstActive)
    {
        m_AttackBurstTime += DT;

        if (m_AttackBurstTime < kBossFireLifeTime)
            return;

        // 불꽃 발사 종료
        DestroyBossFireObject(GetOwner());
        m_AttackBurstActive = false;
        m_AttackBurstTime = 0.f;
        ++m_AttackBurstCount;

        // 2. 정해진 발사 횟수를 모두 채웠을 때
        if (m_AttackBurstCount >= kBossAttack1RepeatCount)
        {
            SetState(BOSS_STATE::IDLE); // 공격 끝 -> 대기!
        }
        return;
    }

    // 3. 다음 불꽃 발사 타이밍
    const float nextBurstStartTime = (float)m_AttackBurstCount * (kBossFireLifeTime + kBossAttack1GapTime);
    if (m_AttackBurstCount < kBossAttack1RepeatCount && m_AttackStateTime >= nextBurstStartTime)
    {
        StartAttackBurst();
    }
}

void CBossScript::TickBodyBlink()
{
    if (FlipbookRender() == nullptr)
        return;

    FlipbookRender()->SetVisible(true);

    Ptr<AMaterial> pBossMtrl = FlipbookRender()->GetMaterial();
    if (pBossMtrl == nullptr)
        return;

    if (m_BodyBlinkTime <= 0.f)
    {
        pBossMtrl->SetScalar(VEC4_0, Vec4(1.f, 1.f, 1.f, 0.f));
        return;
    }

    m_BodyBlinkTime -= DT;
    if (m_BodyBlinkTime <= 0.f)
    {
        m_BodyBlinkTime = 0.f;
        pBossMtrl->SetScalar(VEC4_0, Vec4(1.f, 1.f, 1.f, 0.f));
        return;
    }

    const float elapsed = kBossBodyBlinkDuration - m_BodyBlinkTime;
    const float blinkCycle = kBossBodyBlinkInterval * 2.f;
    const float blinkPhase = fmodf(elapsed, blinkCycle);
    const float flashAmount = (blinkPhase < kBossBodyBlinkInterval) ? 0.9f : 0.f;

    pBossMtrl->SetScalar(VEC4_0, Vec4(1.f, 1.f, 1.f, flashAmount));
}

void CBossScript::TriggerHitFlash(Vec3 _HitWorldPos)
{
    if (GetOwner() == nullptr || GetOwner()->Transform() == nullptr)
        return;

    GameObject* pHitFlashObject = CreateBossHitFlashObject(GetOwner());
    if (pHitFlashObject == nullptr || pHitFlashObject->Transform() == nullptr || pHitFlashObject->MeshRender() == nullptr)
        return;

    const Vec3 bossWorldPos = GetOwner()->Transform()->GetWorldPos();
    const Vec3 bossWorldScale = GetOwner()->Transform()->GetWorldScale();

    Vec3 localFlashPos = _HitWorldPos - bossWorldPos;
    localFlashPos.x = std::clamp(localFlashPos.x, -bossWorldScale.x * 0.45f, bossWorldScale.x * 0.45f);
    localFlashPos.y = std::clamp(localFlashPos.y, -bossWorldScale.y * 0.45f, bossWorldScale.y * 0.45f);
    localFlashPos.z = -0.1f;

    pHitFlashObject->Transform()->SetRelativePos(localFlashPos);
    pHitFlashObject->Transform()->SetRelativeScale(kBossHitFlashBaseScale);

    Ptr<AMaterial> pFlashMtrl = pHitFlashObject->MeshRender()->CreateDynamicMaterial();
    if (pFlashMtrl != nullptr)
        pFlashMtrl->SetScalar(VEC4_0, Vec4(1.f, 1.f, 1.f, 0.9f));

    m_HitFlashTime = kBossHitFlashDuration;
}

void CBossScript::TickHitFlash()
{
    if (GetOwner() == nullptr || GetOwner()->IsDead())
    {
        m_HitFlashTime = 0.f;
        return;
    }

    GameObject* pHitFlashObject = FindBossHitFlashObject(GetOwner());
    if (pHitFlashObject == nullptr)
    {
        m_HitFlashTime = 0.f;
        return;
    }

    m_HitFlashTime -= DT;
    if (m_HitFlashTime <= 0.f)
    {
        m_HitFlashTime = 0.f;
        DestroyHitFlash();
        return;
    }

    if (pHitFlashObject->Transform() == nullptr || pHitFlashObject->MeshRender() == nullptr)
        return;

    const float progress = 1.f - (m_HitFlashTime / kBossHitFlashDuration);
    const float alpha = 1.f - progress;

    pHitFlashObject->Transform()->SetRelativeScale(kBossHitFlashBaseScale + (kBossHitFlashScaleGrow * progress));

    Ptr<AMaterial> pFlashMtrl = pHitFlashObject->MeshRender()->GetMaterial();
    if (pFlashMtrl != nullptr)
        pFlashMtrl->SetScalar(VEC4_0, Vec4(1.f, 1.f, 1.f, alpha));
}

void CBossScript::DestroyHitFlash()
{
    GameObject* pHitFlashObject = FindBossHitFlashObject(GetOwner());
    if (pHitFlashObject == nullptr)
        return;

    if (pHitFlashObject->Transform() != nullptr)
    {
        pHitFlashObject->Transform()->SetRelativePos(Vec3(0.f, 0.f, -0.1f));
        pHitFlashObject->Transform()->SetRelativeScale(kBossHitFlashBaseScale);
    }

    if (pHitFlashObject->MeshRender() != nullptr)
    {
        Ptr<AMaterial> pFlashMtrl = pHitFlashObject->MeshRender()->GetMaterial();
        if (pFlashMtrl != nullptr)
            pFlashMtrl->SetScalar(VEC4_0, Vec4(1.f, 1.f, 1.f, 0.f));
    }
}

