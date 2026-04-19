#pragma once
#include "CRenderComponent.h"
#include "AParticleTickCS.h"
#include "StructuredBuffer.h"

enum class PARTICLE_MODULE
{
    SPAWN,
    SPAWN_BURST,
    ADD_VELOCITY,
    SCALE,
    DRAG,
    NOISE_FORCE,
    RENDER,

    END,
};

// Particle Module
struct tParticleModule
{
    // Spawn Modlue
    float	SpawnRate;
    Vec4	StartColor;

    Vec3	MinScale;
    Vec3	MaxScale;
    float	MinLife;
    float	MaxLife;
    int		SpawnShape;			// 0 : Box, 1 : Sphere
    Vec3	SpawnShapeScale;
    int		SpaceType;			// 0 : Local, 1 : World

    UINT	BlockSpawnShape;		// 0 : Box,  1: Sphere
    Vec3	BlockSpawnShapeScale;	// SpawnShapeScale.x == Radius

    // Spawn Burst
    UINT	SpawnBurstCount;		
    UINT	SpawnBurstRepeat;
    float	SpawnBurstRepeatTime;


    // Add Velocity
    UINT	AddVelocityType;		// 0 : Random, 1 : FromCenter, 2 : ToCenter, 4 : Fixed 
    Vec3	AddVelocityFixedDir;
    float	AddMinSpeed;
    float	AddMaxSpeed;

    // Scale Module
    float	StartScale;
    float	EndScale;

    // Drag Module
    float	DestNormalizedAge;
    float	LimitSpeed;

    // Noise Force Module
    float	NoiseForceTerm;		// Noise Force 변경 간격
    float	NoiseForceScale;	// Noise Force 크기

    // Render Module
    Vec4	EndColor;			
    int		FadeOut;			// 0 : Off, 1 : Normalized Age
    float   StartRatio;			// FadeOut 시작 시점 Normalized Age 기준
    UINT	VelocityAlignment;  // 속도에 따른 정렬기능( 0 : Off, 1 : On )
    UINT	CrossMesh;			// 십자 메쉬 사용 ( 0 : Off, 1 : ON )

    // 파티클 소유주(GameObject) 의 위치
    Vec3	ObjectWorldPos;

    // Module On / Off
    int		Module[(UINT)PARTICLE_MODULE::END];
};


// =============
// Particle 정보
// =============
struct tParticle
{
    Vec4	LocalPos;		// 파티클의 위치
    Vec4	WorldPos;		// 파티클의 위치
    Vec4	WorldInitScale; // 파티클 초기 크기
    Vec4	WorldScale;		// 파티클 현재 크기
    Vec4	Color;			// 파티클 색상

    Vec4	Force;			// 파티클에 적용되고 있는 힘
    Vec4	Velocity;		// 현재 파티클의 속도
    float	Mass;			// 파티클 질량

    float   NoiseForceAccTime;  // Noise Force 가 새로 재적용되기까지 시칸 체크용도
    Vec3	NoiseForceDir;      // Noise Forec 가 적용중인 방향

    float	Age;			// 파티클 현재 수명
    float	Life;			// 파티클 최대 수명
    float	NormalizedAge;  // NormalizedAge == Age / Life

    int		Active;			// 파티클 활성화 상태    
};

// 생성할 파티클 개수
struct tSpawnCount
{
    int		SpawnCount;
    Vec3	vPadding;
};


class CStructuredBuffer;

class CParticleRender :
    public CRenderComponent
{
private:
    Ptr<StructuredBuffer>   m_ParticleBuffer;
    Ptr<StructuredBuffer>   m_SpawnBuffer;
    Ptr<StructuredBuffer>   m_ModuleBuffer;

    Ptr<AParticleTickCS>    m_TickCS;
    UINT                    m_MaxParticle;      // 파티클 최대 개수 제한

    tParticleModule         m_Module;           // 파티클 시뮬레이션 모듈설정
    bool                    m_ModuleChanged;    // 모듈에 값이 변경됐는지 체크
    float                   m_AccTime;          // 누적시간 체크용

    Ptr<ATexture>          m_ParticleTex;      // 파티클 입자가 사용할 텍스쳐


public:
    void SetParticleTex(Ptr<ATexture> _Tex) { m_ParticleTex = _Tex; }

    // Spawn
    void SetSpawnRate(float _Rate) { m_Module.SpawnRate = _Rate; m_ModuleChanged = true; }
    float GetSpawnRate() { return m_Module.SpawnRate; m_ModuleChanged = true; }
    void SetStartColor(Vec4 _Color) { m_Module.StartColor = _Color; m_ModuleChanged = true; }

    void SetMinScale(Vec3 _Scale) { m_Module.MinScale = _Scale; m_ModuleChanged = true; }
    void SetMaxScale(Vec3 _Scale) { m_Module.MaxScale = _Scale; m_ModuleChanged = true; }
    void SetMinLife(float _MinLife) { m_Module.MinLife = _MinLife; m_ModuleChanged = true; }
    void SetMaxLife(float _MaxLife) { m_Module.MaxLife = _MaxLife; m_ModuleChanged = true; }

    // 0 : Box, 1 : Sphere
    void SetSpawnShape(int _Shape) { m_Module.SpawnShape = _Shape; m_ModuleChanged = true; }
    void SetSpawnShapeScale(Vec3 _Scale) { m_Module.SpawnShapeScale = _Scale; m_ModuleChanged = true; }
    // 0 : Local, 1 : World;
    void SetSpaceType(int _Type) { m_Module.SpaceType = _Type; m_ModuleChanged = true; }

    void SetModlue(PARTICLE_MODULE _Module, bool _On) { m_Module.Module[(int)_Module] = _On;  m_ModuleChanged = true; }

    // AddVeolcityModule
    // 0 : Random Velocity, 1 : FromCenter, 2: ToCenter, 3 : Fixed
    void SetAddVelocityType(int _Type) { m_Module.AddVelocityType = _Type; m_ModuleChanged = true; }
    void SetMinSpeed(float _MinSpeed) { m_Module.AddMinSpeed = _MinSpeed;  m_ModuleChanged = true; }
    void SetMaxSpeed(float _MaxSpeed) { m_Module.AddMaxSpeed = _MaxSpeed;  m_ModuleChanged = true; }
    void SetFixedVelocity(Vec3 _FixedVelocityDir) { m_Module.AddVelocityFixedDir = _FixedVelocityDir; m_ModuleChanged = true; }

    // SpawnBurst
    void SetBurstParticleCount(int _Count) { m_Module.SpawnBurstCount = _Count;  m_ModuleChanged = true; }
    void SetBurstRepeatCount(int _Count) { m_Module.SpawnBurstRepeat = _Count;  m_ModuleChanged = true; }
    void SetBurstTerm(float _Term) { m_Module.SpawnBurstRepeatTime = _Term;  m_ModuleChanged = true; }

    // ScaleModule
    void SetStartScale(float _Start) { m_Module.StartScale = _Start;  m_ModuleChanged = true; }
    void SetEndScale(float _End) { m_Module.EndScale = _End;  m_ModuleChanged = true; }

    // DragModule
    void SetDragDestNormalizeAge(float _Age) { m_Module.DestNormalizedAge = _Age;  m_ModuleChanged = true; }
    void SetDragLimitSpeed(float _LimitSpeed) { m_Module.LimitSpeed = _LimitSpeed; m_ModuleChanged = true; }

    // NoiseForce
    void SetNoiseForceTerm(float _Term) { m_Module.NoiseForceTerm = _Term;  m_ModuleChanged = true; }
    void SetNoiseForceScale(float _Scale) { m_Module.NoiseForceScale = _Scale;  m_ModuleChanged = true; }

    // Render Module
    void SetEndColor(Vec4 _Color) { m_Module.EndColor = _Color; m_ModuleChanged = true; }
    void SetFadeOut(bool _FadeOut) { m_Module.FadeOut = _FadeOut;  m_ModuleChanged = true; }
    void SetFadOutStartRatio(float _Ratio) { m_Module.StartRatio = _Ratio;  m_ModuleChanged = true; }
    void SetVelocityAlignment(bool _Use, bool _CrossMesh) { m_Module.VelocityAlignment = _Use; m_Module.CrossMesh = _CrossMesh;  m_ModuleChanged = true; }

public:
    virtual void FinalTick() override;
    virtual void Render() override;
   
    // 저장 불러오기
    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

private:
    void CalcSpawnCount();
    virtual void CreateMaterial() override;

public:
    CLONE(CParticleRender);
    CParticleRender();
    CParticleRender(const CParticleRender& _Origin);
    ~CParticleRender();
};