#pragma once


struct Vertex
{
	Vec3	vPos;	// 3D 공간상에서 정점의 위치
	Vec2	vUV;	// 정점위에 이미지를 띄울 경우, 이미지의 어느 지점을 참고하는지 좌표
	Vec4	vColor; // 정점 자체의 색상
};
typedef Vertex Vtx;


// DebugInfo : 디버그 렌더링 요청사항 정보
struct DbgInfo
{
	DBG_SHAPE	Shape;

	Vec3		Pos;
	Vec3		Scale;
	Vec3		Rotation;

	Matrix		matWorld;

	Vec4		Color;
	float		Age;		
	float		Life;		

	bool		DepthTest;
};

// TaskMgr 가 처리할 작업 단위
struct TaskInfo
{
	TASK_TYPE Type;
	DWORD_PTR Param_0;
	DWORD_PTR Param_1;
	DWORD_PTR Param_2;
};

// Light2D 정보
struct Light2DInfo
{
	LIGHT_TYPE	Type;
	Vec3		Color;		// 빛의 색상
	Vec3		Ambient;	// 환경광, 광원이 존재하면서 최소한으로 발생하는 빛의 세기
	Vec3		LightDir;	// 광원의 빛이 향하는 방향
	Vec3		WorldPos;	// 광원의 위치 (포인트, 스포트)
	float		Radius;		// 빛의 영향 반경(포인트, 스포트)
	float		Angle;		// SpotLight 범위 각
};


struct TransformMatrix
{
	Matrix	matWorld;
	Matrix  matView;
	Matrix	matProj;
};
// extern, 전방선언, 유일한 전역변수, 모든파일(cpp) 에서 접근 가능
extern TransformMatrix g_Trans;

struct MtrlConst
{
	int		iArr[4];
	float	fArr[4];
	Vec2	v2Arr[4];
	Vec4	v4Arr[4];
	Matrix	mat[2];

	int		IsTex[(UINT)TEX_PARAM::TEX_END];
	int		Padding[2];
};

struct GlobalData
{
	Vec2	Resolution;		// 화면 해상도
	int		Light2DCount;	// 2D 광원 개수
	int		Light3DCount;	// 3D 광원 개수
	float	DeltaTime;		// DeltaTime
	float	Time;			// 누적 시간값
	float   EngineDT;		// DeltaTime
	float	EngineTime;		// 누적 시간값
};
extern GlobalData g_Global;
