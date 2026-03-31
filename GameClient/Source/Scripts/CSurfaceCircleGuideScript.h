#pragma once

#include "CScript.h"

#include <string>

class CSurfaceCircleGuideScript
    : public CScript
{
private:
    bool    m_Enabled;
    float   m_EntryAngleDeg;
    float   m_HalfCheckAngleDeg;
    float   m_ExitAngleDeg;
    Vec2    m_CorrectionLineStartLocal;
    Vec2    m_CorrectionLineEndLocal;
    wstring m_LinkedCorrectionLineName;

public:
    virtual void Begin() override;
    virtual void Tick() override;

    bool IsEnabled() const { return m_Enabled; }
    void SetEnabled(bool _Enabled) { m_Enabled = _Enabled; }

    float GetEntryAngleDeg() const { return m_EntryAngleDeg; }
    float GetHalfCheckAngleDeg() const { return m_HalfCheckAngleDeg; }
    float GetExitAngleDeg() const { return m_ExitAngleDeg; }
    void SetEntryAngleDeg(float _AngleDeg);
    void SetHalfCheckAngleDeg(float _AngleDeg);
    void SetExitAngleDeg(float _AngleDeg);

    const Vec2& GetCorrectionLineStartLocal() const { return m_CorrectionLineStartLocal; }
    const Vec2& GetCorrectionLineEndLocal() const { return m_CorrectionLineEndLocal; }
    void SetCorrectionLineStartLocal(const Vec2& _LocalPoint) { m_CorrectionLineStartLocal = _LocalPoint; }
    void SetCorrectionLineEndLocal(const Vec2& _LocalPoint) { m_CorrectionLineEndLocal = _LocalPoint; }

    const wstring& GetLinkedCorrectionLineName() const { return m_LinkedCorrectionLineName; }
    void SetLinkedCorrectionLineName(const wstring& _Name) { m_LinkedCorrectionLineName = _Name; }

    void ResetToDefaultGuide(float _Radius);
    void CopyFrom(const CSurfaceCircleGuideScript& _Other);

    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

    CLONE(CSurfaceCircleGuideScript);

public:
    CSurfaceCircleGuideScript();
    CSurfaceCircleGuideScript(const CSurfaceCircleGuideScript& _Origin);
    virtual ~CSurfaceCircleGuideScript();
};
