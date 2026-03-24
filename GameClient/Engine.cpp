#include "pch.h"
#include "Engine.h"

#include "Device.h"
#include "TimeMgr.h"
#include "KeyMgr.h"
#include "LevelMgr.h"
#include "RenderMgr.h"
#include "TaskMgr.h"
#include "EditorMgr.h"

Engine::Engine()
    : m_hInst(nullptr)
    , m_hWnd(nullptr)
    , m_Resolution{}
    , m_EditorMode(true)
{
}

Engine::~Engine()
{
}

int Engine::Progress()
{
    // DeltaTime 계산
    TimeMgr::GetInst()->Tick();

    // Key 상태 계산
    KeyMgr::GetInst()->Tick();
      
    // LevelMgr 업데이트
    LevelMgr::GetInst()->Progress();

    // RenderMgr 렌더링
    RenderMgr::GetInst()->Progress();
       
    // EditorMgr
    if (m_EditorMode)
    {
        EditorMgr::GetInst()->Progress();
    }    

    // 렌더타겟에 그려진 그림을, 윈도우 비트맵으로 복사
    Device::GetInst()->Present();

    // 다음 프레임에 적용될 작업 처리
    TaskMgr::GetInst()->Progress();

    return S_OK;
}

