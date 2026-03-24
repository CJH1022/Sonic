#pragma once


class TimeMgr
	: public singleton<TimeMgr>
{
	SINGLE(TimeMgr)
private:
	LARGE_INTEGER	m_Frequency;
	LARGE_INTEGER	m_Prev;
	LARGE_INTEGER	m_Current;

	UINT			m_FPS;

	float			m_DeltaTime;	// 1 프레임 간격 시간
	float			m_Time;			// 게임이 켜진 이후로 흐른 시간(누적시간)

public:
	void Init();
	void Tick();

public:
	float GetDeltaTime() { return m_DeltaTime; }
	float GetTime() { return g_Global.Time; }

	float GetEngineDT() { return g_Global.EngineDT; }
	float GetEngineTime() { return g_Global.EngineTime; }
};

#define DT		TimeMgr::GetInst()->GetDeltaTime()
#define TIME	TimeMgr::GetInst()->GetTime()

#define E_DT	TimeMgr::GetInst()->GetEngineDT()
#define E_Time	TimeMgr::GetInst()->GetEngineTime()
