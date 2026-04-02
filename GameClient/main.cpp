#include "pch.h"

#include "Engine.h"

#include "Entity.h"
#include "Asset.h"
#include "AMesh.h"
#include "EditorMgr.h"
#include "AssetMgr.h"
#include "ALevel.h"
#include "GameObject.h"
#include "LevelMgr.h"
#include "Source\\Scripts\\CPlayerScript.h"
#include "func.h"

HINSTANCE hInst;

INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

namespace
{
    wstring BuildAutoplayReportPath(const wchar_t* _FileName)
    {
        wchar_t modulePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

        wstring fullPath = modulePath;
        const size_t slashPos = fullPath.find_last_of(L"\\/");
        if (slashPos != wstring::npos)
            fullPath.erase(slashPos + 1);

        fullPath += _FileName;
        return fullPath;
    }
}

// SAL : 주석 언어
int APIENTRY wWinMain(_In_      HINSTANCE hInstance,
                     _In_opt_   HINSTANCE hPrevInstance,
                     _In_       LPWSTR    lpCmdLine,
                     _In_       int       nCmdShow)
{         
     // CRT new, delete, 디버깅 모드에서 메모리 누수 추적, 출력창에 알림
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    // 누수 발생지점 중단점 걸어주는 기능
    //_CrtSetBreakAlloc(203); 

    hInst = hInstance;
    const wchar_t* fullCmdLine = GetCommandLineW();
    const bool autoplaySurface = (nullptr != wcsstr(fullCmdLine, L"-autoplay_surface"));
    const bool autoplay = (nullptr != wcsstr(fullCmdLine, L"-autoplay")) || autoplaySurface;
    FILE* autoplayReport = nullptr;

    // Engine 초기화
    // 최상위 관리자
    if (FAILED(Engine::GetInst()->Init(hInstance, 1600, 900, !autoplay)))
    {
        return 0;
    }

    if (autoplay)
    {
        const wstring reportPath = BuildAutoplayReportPath(L"autoplay_boot_report.txt");
        _wfopen_s(&autoplayReport, reportPath.c_str(), L"w, ccs=UTF-8");
        if (nullptr != autoplayReport)
        {
            fwprintf(autoplayReport, L"autoplay boot report\n");
            fwprintf(autoplayReport, L"step=engine_init_ok path=%ls\n", reportPath.c_str());
            fflush(autoplayReport);
        }
    }

    if (autoplay)
    {
        if (nullptr != autoplayReport)
        {
            fwprintf(autoplayReport, L"step=before_level_load\n");
            fflush(autoplayReport);
        }

        if (autoplaySurface)
        {
            CreateSurfaceAutoplayLevel();

            if (nullptr != autoplayReport)
            {
                fwprintf(autoplayReport, L"step=after_surface_level_create\n");
                fflush(autoplayReport);
            }
        }
        else
        {
            AssetMgr::GetInst()->Load<ALevel>(L"AutoplayLevel", L"Level\\TestLevel.lv");

            if (nullptr != autoplayReport)
            {
                fwprintf(autoplayReport, L"step=after_level_load\n");
                fflush(autoplayReport);
            }

            ChangeLevel(L"AutoplayLevel");
            if (nullptr != autoplayReport)
            {
                fwprintf(autoplayReport, L"step=after_change_level\n");
                fflush(autoplayReport);
            }
        }

        ChangeLevelState(LEVEL_STATE::PLAY);
        if (nullptr != autoplayReport)
        {
            fwprintf(autoplayReport, L"step=after_change_level_state\n");
            fflush(autoplayReport);
        }
    }
    else
    {
        CreateOpenLevel();
    }

    // 메세지 루프
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAMECLIENT));
    MSG msg = {};
    const ULONGLONG autoplayStartTick = GetTickCount64();
    ULONGLONG autoplayLastReportTick = autoplayStartTick;

    if (autoplay)
    {
        _wfopen_s(&autoplayReport, L"autoplay_boot_report.txt", L"w, ccs=UTF-8");
        if (nullptr != autoplayReport)
        {
            fwprintf(autoplayReport, L"autoplay boot report\n");
        }
    }

    while (true)
    {
        // 메세지 큐에서 메세지를 꺼낸게 있다.
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;

            // 단축키 관련된 내용이면 TranslateAccelerator 함수에서 처리
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                // 단축키 관련된 이벤트가 아니면 TranslateMessage, DispatchMessage 함수를 이용해서 처리
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        // 메세지 큐에 메세지가 없었다.
        else
        {
            // Game 실행, 1 프레임
            if (FAILED(Engine::GetInst()->Progress()))
                break;

            if (autoplay)
            {
                const ULONGLONG nowTick = GetTickCount64();
                if (nullptr != autoplayReport && nowTick - autoplayLastReportTick >= 250)
                {
                    autoplayLastReportTick = nowTick;

                    const float elapsed = (float)(nowTick - autoplayStartTick) / 1000.f;
                    const LEVEL_STATE levelState = LevelMgr::GetInst()->GetLevelState();
                    Ptr<GameObject> pPlayer = nullptr;
                    Ptr<GameObject> pTileMap = nullptr;
                    Ptr<GameObject> pFirstTile = nullptr;
                    CPlayerScript* pPlayerScript = nullptr;
                    Vec3 playerPos = Vec3(0.f, 0.f, 0.f);
                    Vec3 tileMapPos = Vec3(0.f, 0.f, 0.f);
                    Vec3 tileMapScale = Vec3(0.f, 0.f, 0.f);
                    Vec3 firstTileWorldPos = Vec3(0.f, 0.f, 0.f);
                    Vec3 firstTileWorldScale = Vec3(0.f, 0.f, 0.f);
                    int tileChildCount = 0;
                    int isGround = 0;
                    Vec2 normal = Vec2(0.f, 0.f);

                    if (nullptr != LevelMgr::GetInst()->GetCurLevel())
                    {
                        pPlayer = LevelMgr::GetInst()->FindObjectByName(L"Player");
                        pTileMap = LevelMgr::GetInst()->FindObjectByName(L"TileMapRender");
                    }

                    if (nullptr != pPlayer)
                    {
                        playerPos = pPlayer->Transform()->GetRelativePos();
                        pPlayerScript = pPlayer->GetScript<CPlayerScript>().Get();
                    }

                    if (nullptr != pTileMap)
                    {
                        tileMapPos = pTileMap->Transform()->GetRelativePos();
                        tileMapScale = pTileMap->Transform()->GetRelativeScale();
                        tileChildCount = (int)pTileMap->GetChild().size();

                        if (0 < tileChildCount)
                        {
                            pFirstTile = pTileMap->GetChild(0);
                            if (nullptr != pFirstTile)
                            {
                                firstTileWorldPos = pFirstTile->Transform()->GetWorldPos();
                                firstTileWorldScale = pFirstTile->Transform()->GetWorldScale();
                            }
                        }
                    }

                    if (nullptr != pPlayerScript)
                    {
                        isGround = pPlayerScript->GetIsGround() ? 1 : 0;
                        normal = pPlayerScript->GetNormal();
                    }

                    fwprintf(autoplayReport,
                             L"time=%.3f level_state=%d has_player=%d has_player_script=%d pos=(%.2f,%.2f) ground=%d normal=(%.3f,%.3f) has_tilemap=%d tilemap_pos=(%.2f,%.2f) tilemap_scale=(%.2f,%.2f) tile_children=%d has_first_tile=%d first_tile_world=(%.2f,%.2f) first_tile_scale=(%.2f,%.2f)\n",
                             elapsed, (int)levelState, pPlayer != nullptr ? 1 : 0, pPlayerScript != nullptr ? 1 : 0,
                             playerPos.x, playerPos.y, isGround, normal.x, normal.y,
                             pTileMap != nullptr ? 1 : 0,
                             tileMapPos.x, tileMapPos.y, tileMapScale.x, tileMapScale.y, tileChildCount,
                             pFirstTile != nullptr ? 1 : 0,
                             firstTileWorldPos.x, firstTileWorldPos.y, firstTileWorldScale.x, firstTileWorldScale.y);
                    fflush(autoplayReport);
                }

                if (nowTick - autoplayStartTick > 25000)
                    PostQuitMessage(0);
            }
        } 
    }

    if (nullptr != autoplayReport)
    {
        fclose(autoplayReport);
        autoplayReport = nullptr;
    }

    return (int) msg.wParam;
}


#include "KeyMgr.h"
LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch (message)
    {
    case WM_MOUSEWHEEL:
    {
        KeyMgr::GetInst()->SetMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
    }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
