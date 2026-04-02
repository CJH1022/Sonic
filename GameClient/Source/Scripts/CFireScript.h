#pragma once
#include "CScript.h"

class GameObject;

class CFireScript :
    public CScript
{
private:
    Vec2    m_MoveDir;
    Vec2    m_BaseScale;
    Vec2    m_KnockBackPower;
    float   m_MoveSpeed;
    float   m_FlameLength;
    float   m_LifeTime;
    float   m_FlameFPS;
    float   m_PulseSpeed;
    float   m_PulseAmount;
    float   m_AccTime;
    int     m_Loop;
    bool    m_Active;

public:
    virtual void Begin();
    virtual void Tick() override;

public:
    void SetMoveDir(const Vec2& _Dir) { m_MoveDir = _Dir; }
    void SetBaseScale(const Vec2& _Scale) { m_BaseScale = _Scale; }
    void SetKnockBackPower(const Vec2& _Power) { m_KnockBackPower = _Power; }
    void SetMoveSpeed(float _Speed) { m_MoveSpeed = _Speed; }
    void SetFlameLength(float _Length) { m_FlameLength = _Length; }
    void SetLifeTime(float _LifeTime) { m_LifeTime = _LifeTime; }
    void SetFlameFPS(float _FPS) { m_FlameFPS = _FPS; }
    void SetLoop(int _Loop) { m_Loop = _Loop; }
    void SetActive(bool _Active);
    bool IsActive() const { return m_Active; }

public:
    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

private:
    GameObject* FindSegment(const wchar_t* _Name);
    void EnsureSegments();
    void SetSegmentVisible(const wchar_t* _Name, bool _Visible);
    void SetAllSegmentsVisible(bool _Visible);
    void UpdateSegment(const wchar_t* _Name, const Vec2& _Dir, float _Distance, const Vec2& _Scale,
                       float _RotationZ, bool _Visible, int _FlipbookIndex, float _FPS);

public:
    CLONE(CFireScript);
    CFireScript();
    CFireScript(const CFireScript& _Origin);
    virtual ~CFireScript();
};
