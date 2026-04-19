#include "pch.h"
#include "CDeadPieceScript.h"

#include "AFlipbook.h"
#include "ASprite.h"
#include "CFlipbookRender.h"
#include "CMeshRender.h"
#include "CSpriteRender.h"
#include "CTransform.h"
#include "GameObject.h"
#include "TimeMgr.h"
#include "func.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kDeadPieceMinSize = 0.0001f;

    struct DeadPieceQuadrant
    {
        float xSign;
        float ySign;
        Vec2 uvMinFactor;
    };

    const DeadPieceQuadrant g_DeadPieceQuadrants[4] =
    {
        { -1.f,  1.f, Vec2(0.f, 0.f) },
        {  1.f,  1.f, Vec2(0.5f, 0.f) },
        { -1.f, -1.f, Vec2(0.f, 0.5f) },
        {  1.f, -1.f, Vec2(0.5f, 0.5f) },
    };

    int ResolveDeadPieceLayer(GameObject* _Source, int _RequestedLayerIdx)
    {
        if (_RequestedLayerIdx >= 0)
            return _RequestedLayerIdx;

        if (_Source != nullptr)
            return _Source->GetLayerIdx();

        return 0;
    }

    Vec3 ResolveSourcePos(GameObject* _Source)
    {
        if (_Source == nullptr || _Source->Transform() == nullptr)
            return Vec3(0.f, 0.f, 0.f);

        if (_Source->GetParent() == nullptr)
            return _Source->Transform()->GetRelativePos();

        return _Source->Transform()->GetWorldPos();
    }

    Vec3 ResolveSourceScale(GameObject* _Source)
    {
        if (_Source == nullptr || _Source->Transform() == nullptr)
            return Vec3(1.f, 1.f, 1.f);

        if (_Source->GetParent() == nullptr)
            return _Source->Transform()->GetRelativeScale();

        return _Source->Transform()->GetWorldScale();
    }

    Vec2 ResolveSpriteBackgroundUV(Ptr<ASprite> _Sprite)
    {
        if (_Sprite == nullptr)
            return Vec2(0.f, 0.f);

        Vec2 background = _Sprite->GetBackgroundUV();
        if (background.x <= 0.f || background.y <= 0.f)
            background = _Sprite->GetSliceUV();

        return background;
    }

    Vec2 ResolveSpriteOffsetUV(Ptr<ASprite> _Sprite)
    {
        if (_Sprite == nullptr)
            return Vec2(0.f, 0.f);

        Vec2 background = _Sprite->GetBackgroundUV();
        if (background.x <= 0.f || background.y <= 0.f)
            return Vec2(0.f, 0.f);

        return _Sprite->GetOffsetUV();
    }

    Ptr<ASprite> ResolveCurrentSprite(GameObject* _Source)
    {
        if (_Source == nullptr)
            return nullptr;

        if (_Source->SpriteRender() != nullptr)
            return _Source->SpriteRender()->GetSprite();

        if (_Source->FlipbookRender() == nullptr)
            return nullptr;

        Ptr<AFlipbook> pFlipbook = _Source->FlipbookRender()->GetCurFlipbook();
        if (pFlipbook == nullptr || 0 == pFlipbook->GetSpriteCount())
            return nullptr;

        int spriteIdx = _Source->FlipbookRender()->GetCurSpriteIdx();
        if (spriteIdx < 0)
            spriteIdx = 0;
        if ((UINT)spriteIdx >= pFlipbook->GetSpriteCount())
            spriteIdx = (int)pFlipbook->GetSpriteCount() - 1;

        return pFlipbook->GetSprite(spriteIdx);
    }

    Ptr<ASprite> CreateQuadrantSprite(Ptr<ASprite> _SourceSprite, int _QuadrantIdx)
    {
        if (_SourceSprite == nullptr || _SourceSprite->GetAtlas() == nullptr)
            return nullptr;

        const Vec2 sourceSlice = _SourceSprite->GetSliceUV();
        const Vec2 storedBackground = _SourceSprite->GetBackgroundUV();
        const bool bUseTrimmedSprite = (storedBackground.x > kDeadPieceMinSize && storedBackground.y > kDeadPieceMinSize);

        if (sourceSlice.x <= kDeadPieceMinSize || sourceSlice.y <= kDeadPieceMinSize)
        {
            return nullptr;
        }

        const DeadPieceQuadrant& quadrant = g_DeadPieceQuadrants[_QuadrantIdx];
        Ptr<ASprite> pPieceSprite = new ASprite;
        pPieceSprite->SetAtlas(_SourceSprite->GetAtlas());

        if (!bUseTrimmedSprite)
        {
            const Vec2 pieceSlice(sourceSlice.x * 0.5f, sourceSlice.y * 0.5f);
            pPieceSprite->SetLeftTopUV(_SourceSprite->GetLeftTopUV()
                + Vec2(sourceSlice.x * quadrant.uvMinFactor.x,
                       sourceSlice.y * quadrant.uvMinFactor.y));
            pPieceSprite->SetSliceUV(pieceSlice);
            return pPieceSprite;
        }

        const Vec2 background = ResolveSpriteBackgroundUV(_SourceSprite);
        const Vec2 sourceOffset = ResolveSpriteOffsetUV(_SourceSprite);

        if (background.x <= kDeadPieceMinSize || background.y <= kDeadPieceMinSize)
            return nullptr;

        const Vec2 pieceBackground(background.x * 0.5f, background.y * 0.5f);
        const Vec2 quadMin(background.x * quadrant.uvMinFactor.x, background.y * quadrant.uvMinFactor.y);
        const Vec2 quadMax = quadMin + pieceBackground;

        const Vec2 visibleMin((std::max)(sourceOffset.x, quadMin.x), (std::max)(sourceOffset.y, quadMin.y));
        const Vec2 visibleMax((std::min)(sourceOffset.x + sourceSlice.x, quadMax.x),
                              (std::min)(sourceOffset.y + sourceSlice.y, quadMax.y));

        if (visibleMax.x - visibleMin.x <= kDeadPieceMinSize
            || visibleMax.y - visibleMin.y <= kDeadPieceMinSize)
        {
            return nullptr;
        }

        pPieceSprite->SetLeftTopUV(_SourceSprite->GetLeftTopUV() + (visibleMin - sourceOffset));
        pPieceSprite->SetSliceUV(visibleMax - visibleMin);
        pPieceSprite->SetBackgroundUV(pieceBackground);
        pPieceSprite->SetOffsetUV(visibleMin - quadMin);
        return pPieceSprite;
    }

    void ApplyDeadPieceMotion(CDeadPieceScript* _Script, int _QuadrantIdx, const DeadPieceSpawnDesc& _Desc)
    {
        if (_Script == nullptr)
            return;

        const DeadPieceQuadrant& quadrant = g_DeadPieceQuadrants[_QuadrantIdx];
        const float upwardSpeed = (quadrant.ySign > 0.f) ? _Desc.TopUpwardSpeed : _Desc.BottomUpwardSpeed;
        const float horizontalSpeed = quadrant.xSign * _Desc.HorizontalSpeed;
        const float spinDir = (quadrant.xSign * quadrant.ySign < 0.f) ? -1.f : 1.f;

        _Script->SetVelocity(Vec2(horizontalSpeed, upwardSpeed));
        _Script->SetGravity(_Desc.Gravity);
        _Script->SetLifeTime(_Desc.LifeTime);
        _Script->SetAngularVelocity(_Desc.SpinSpeed * spinDir);
    }

    bool SpawnSpritePieces(GameObject* _Source, Ptr<ASprite> _Sprite, int _LayerIdx, const DeadPieceSpawnDesc& _Desc)
    {
        if (_Source == nullptr || _Source->Transform() == nullptr || _Sprite == nullptr)
            return false;

        const Vec3 sourcePos = ResolveSourcePos(_Source);
        const Vec3 sourceScale = ResolveSourceScale(_Source);
        const Vec3 sourceRot = _Source->Transform()->GetRelativeRot();
        const Vec3 pieceScale(sourceScale.x * 0.5f,
                              sourceScale.y * 0.5f,
                              (fabsf(sourceScale.z) <= kDeadPieceMinSize) ? 1.f : sourceScale.z);

        bool spawned = false;

        for (int i = 0; i < 4; ++i)
        {
            Ptr<ASprite> pPieceSprite = CreateQuadrantSprite(_Sprite, i);
            if (pPieceSprite == nullptr)
                continue;

            const DeadPieceQuadrant& quadrant = g_DeadPieceQuadrants[i];

            GameObject* pPiece = new GameObject;
            pPiece->SetName(_Source->GetName() + L"_DeadPiece");
            pPiece->AddComponent(new CTransform);
            pPiece->AddComponent(new CSpriteRender);

            Ptr<CDeadPieceScript> pPieceScript = new CDeadPieceScript;
            pPiece->AddComponent(pPieceScript.Get());

            pPiece->Transform()->SetRelativePos(Vec3(
                sourcePos.x + (sourceScale.x * 0.25f * quadrant.xSign),
                sourcePos.y + (sourceScale.y * 0.25f * quadrant.ySign),
                sourcePos.z));
            pPiece->Transform()->SetRelativeScale(pieceScale);
            pPiece->Transform()->SetRelativeRot(sourceRot);
            pPiece->SpriteRender()->SetSprite(pPieceSprite);

            ApplyDeadPieceMotion(pPieceScript.Get(), i, _Desc);
            CreateObject(pPiece, _LayerIdx);
            spawned = true;
        }

        return spawned;
    }

    bool SpawnMeshPieces(GameObject* _Source, int _LayerIdx, const DeadPieceSpawnDesc& _Desc)
    {
        if (_Source == nullptr || _Source->Transform() == nullptr || _Source->MeshRender() == nullptr)
            return false;

        Ptr<AMaterial> pMaterial = _Source->MeshRender()->GetMaterial();
        if (pMaterial != nullptr)
        {
            Ptr<ATexture> pTexture = pMaterial->GetTexture(TEX_0);
            if (pTexture != nullptr)
            {
                Ptr<ASprite> pTextureSprite = new ASprite;
                pTextureSprite->SetAtlas(pTexture);
                pTextureSprite->SetLeftTopUV(Vec2(0.f, 0.f));
                pTextureSprite->SetSliceUV(Vec2(1.f, 1.f));
                return SpawnSpritePieces(_Source, pTextureSprite, _LayerIdx, _Desc);
            }
        }

        Ptr<AMesh> pMesh = _Source->MeshRender()->GetMesh();
        if (pMesh == nullptr || pMaterial == nullptr)
            return false;

        const Vec3 sourcePos = ResolveSourcePos(_Source);
        const Vec3 sourceScale = ResolveSourceScale(_Source);
        const Vec3 sourceRot = _Source->Transform()->GetRelativeRot();
        const Vec3 pieceScale(sourceScale.x * 0.5f,
                              sourceScale.y * 0.5f,
                              (fabsf(sourceScale.z) <= kDeadPieceMinSize) ? 1.f : sourceScale.z);

        for (int i = 0; i < 4; ++i)
        {
            const DeadPieceQuadrant& quadrant = g_DeadPieceQuadrants[i];

            GameObject* pPiece = new GameObject;
            pPiece->SetName(_Source->GetName() + L"_DeadPiece");
            pPiece->AddComponent(new CTransform);
            pPiece->AddComponent(new CMeshRender);

            Ptr<CDeadPieceScript> pPieceScript = new CDeadPieceScript;
            pPiece->AddComponent(pPieceScript.Get());

            pPiece->Transform()->SetRelativePos(Vec3(
                sourcePos.x + (sourceScale.x * 0.25f * quadrant.xSign),
                sourcePos.y + (sourceScale.y * 0.25f * quadrant.ySign),
                sourcePos.z));
            pPiece->Transform()->SetRelativeScale(pieceScale);
            pPiece->Transform()->SetRelativeRot(sourceRot);

            pPiece->MeshRender()->SetMesh(pMesh);
            pPiece->MeshRender()->SetMaterial(pMaterial);

            ApplyDeadPieceMotion(pPieceScript.Get(), i, _Desc);
            CreateObject(pPiece, _LayerIdx);
        }

        return true;
    }
}

CDeadPieceScript::CDeadPieceScript()
    : CScript(SCRIPT_TYPE::DEADPIECESCRIPT)
    , m_Velocity(Vec2(0.f, 0.f))
    , m_Gravity(900.f)
    , m_LifeTime(0.9f)
    , m_ElapsedTime(0.f)
    , m_AngularVelocity(0.f)
{
}

CDeadPieceScript::~CDeadPieceScript()
{
}

void CDeadPieceScript::Begin()
{
    m_ElapsedTime = 0.f;
}

void CDeadPieceScript::Tick()
{
    m_ElapsedTime += DT;
    if (m_ElapsedTime >= m_LifeTime)
    {
        Destroy();
        return;
    }

    m_Velocity.y -= m_Gravity * DT;

    Vec3 pos = Transform()->GetRelativePos();
    pos.x += m_Velocity.x * DT;
    pos.y += m_Velocity.y * DT;
    Transform()->SetRelativePos(pos);

    if (fabsf(m_AngularVelocity) > 0.0001f)
    {
        Vec3 rot = Transform()->GetRelativeRot();
        rot.z += m_AngularVelocity * DT;
        Transform()->SetRelativeRot(rot);
    }
}

bool CDeadPieceScript::SpawnSplitPieces(GameObject* _Source, const DeadPieceSpawnDesc& _Desc)
{
    if (_Source == nullptr || _Source->Transform() == nullptr)
        return false;

    const int layerIdx = ResolveDeadPieceLayer(_Source, _Desc.LayerIdx);

    Ptr<ASprite> pSourceSprite = ResolveCurrentSprite(_Source);
    if (pSourceSprite != nullptr)
        return SpawnSpritePieces(_Source, pSourceSprite, layerIdx, _Desc);

    if (_Source->MeshRender() != nullptr)
        return SpawnMeshPieces(_Source, layerIdx, _Desc);

    return false;
}
