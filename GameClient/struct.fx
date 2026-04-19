#ifndef _STRUCT
#define _STRUCT

// Light2D 정보
struct Light2DInfo
{
    int     Type;
    float3  Color; // 빛의 색상
    float3  Ambient; // 환경광, 광원이 존재하면서 최소한으로 발생하는 빛의 세기
    float3  LightDir; // 광원의 빛이 향하는 방향
    float3  WorldPos; // 광원의 위치 (포인트, 스포트)
    float   Radius; // 빛의 영향 반경(포인트, 스포트)
    float   Angle; // SpotLight 범위 각
};

struct tParticleModule
{
    float   SpawnRate;
    float4  StartColor;

    float3  MinScale;
    float3  MaxScale;
    float   MinLife;
    float   MaxLife;
    int     SpawnShape;
    float3  SpawnShapeScale;
    int     SpaceType;

    uint    BlockSpawnShape;
    float3  BlockSpawnShapeScale;

    uint    SpawnBurstCount;
    uint    SpawnBurstRepeat;
    float   SpawnBurstRepeatTime;

    uint    AddVelocityType;
    float3  AddVelocityFixedDir;
    float   AddMinSpeed;
    float   AddMaxSpeed;

    float   StartScale;
    float   EndScale;

    float   DestNormalizedAge;
    float   LimitSpeed;

    float   NoiseForceTerm;
    float   NoiseForceScale;

    float4  EndColor;
    int     FadeOut;
    float   StartRatio;
    uint    VelocityAlignment;
    uint    CrossMesh;

    float3  ObjectWorldPos;

    int     Module[7];
};

struct tParticle
{
    float4  LocalPos;
    float4  WorldPos;
    float4  WorldInitScale;
    float4  WorldScale;
    float4  Color;

    float4  Force;
    float4  Velocity;
    float   Mass;

    float   NoiseForceAccTime;
    float3  NoiseForceDir;

    float   Age;
    float   Life;
    float   NormalizedAge;

    int     Active;
};

struct tSpawnCount
{
    int     SpawnCount;
    float3  vPadding;
};


#endif
