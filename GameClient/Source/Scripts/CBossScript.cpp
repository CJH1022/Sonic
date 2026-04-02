#include "pch.h"
#include "CBossScript.h"

#include "AssetMgr.h"
#include "CCollider2D.h"
#include "CBackScript.h"
#include "CFireScript.h"
#include "CFlipbookRender.h"
#include "CMeshRender.h"
#include "CTransform.h"
#include "GameObject.h"
#include <TimeMgr.h>
#include <cmath>
#include <algorithm>
#include "Source/Scripts/CKnockbackScript.h"
namespace
{
    constexpr int kBossMoveFlipbookIndex = 0;
    constexpr int kBossStateCount = 7;
    const Vec3 kBossFireOffset = Vec3(-120.f, 10.f, 0.f);
    const Vec2 kBossFireBaseScale = Vec2(150.f, 110.f);
    const Vec2 kBossFireDir = Vec2(-1.f, 0.f);
    constexpr float kBossFireLength = 250.f;
    constexpr float kBossFireLifeTime = 0.75f;
    constexpr float kBossAttack1GapTime = 0.45f;
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

    if (Collider2D() != nullptr)
        Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CBossScript::BeginOverlap);

    ResetAttackSequence();
    if (m_State == BOSS_STATE::ATTACK1)
        StartAttackBurst();
}

void CBossScript::Tick()
{
    if (m_Life < 1)
        m_State = BOSS_STATE::DEAD;

    Vec3 vMyPos = Transform()->GetRelativePos();

    if (m_State == BOSS_STATE::ATTACK1)
        TickAttack1();
    else if (m_AttackBurstActive || FindBossFireObject(GetOwner()) != nullptr)
        StopAttackBurst();

    TickHitFlash();
    TickBodyBlink();

    if (FlipbookRender() == nullptr)
        return;

    switch (m_State)
    {
    case BOSS_STATE::IDLE:
    {
        // 1. 상태가 유지되는 동안 시간을 계속 누적합니다.
        m_IdleTime += DT;

        // 2. 움직임의 속도와 폭을 설정합니다.
        float frequency = 10.0f; // 위아래로 움직이는 빠르기 (주기)
        float amplitude = 20.0f; // 한 프레임당 이동할 수 있는 최대 속도 (폭)

        // 3. sinf()를 통해 -1 ~ 1 사이를 부드럽게 오가는 값을 얻어 Y축에 더해줍니다.
        vMyPos.y += sinf(m_IdleTime * frequency) * amplitude * DT;
        Transform()->SetRelativePos(vMyPos);
        [[fallthrough]];
    }

    case BOSS_STATE::ATTACK1:
    case BOSS_STATE::MOVE:
        if (FlipbookRender()->GetFlipbook(kBossMoveFlipbookIndex) != nullptr)
            FlipbookRender()->Play(kBossMoveFlipbookIndex, m_MoveFlipFPS, -1);
        break;

    case BOSS_STATE::DEAD:
        GetOwner()->Destroy();
        // 죽음 애니메이션, Destroy()

        break;
    default:
        break;
    }
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
}

void CBossScript::StopAttackBurst()
{
    DestroyBossFireObject(GetOwner());
    m_AttackBurstActive = false;
    m_AttackBurstTime = 0.f;
}

void CBossScript::TickAttack1()
{
    m_AttackStateTime += DT;

    if (m_AttackStateTime >= (kBossAttack1SequenceDuration + 0.1f))
    {
        SetState(BOSS_STATE::IDLE);
        return;
    }

    if (m_AttackBurstActive)
    {
        m_AttackBurstTime += DT;

        if (m_AttackBurstTime < kBossFireLifeTime)
            return;

        DestroyBossFireObject(GetOwner());
        m_AttackBurstActive = false;
        m_AttackBurstTime = 0.f;
        ++m_AttackBurstCount;

        if (m_AttackBurstCount >= kBossAttack1RepeatCount)
        {
            SetState(BOSS_STATE::IDLE);
        }

        return;
    }

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
    if (GetOwner() == nullptr)
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
    if (pHitFlashObject != nullptr)
        pHitFlashObject->Destroy();
}
