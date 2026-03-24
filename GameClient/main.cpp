#include "pch.h"

#include "Engine.h"

#include "Entity.h"
#include "Asset.h"
#include "AMesh.h"
#include "EditorMgr.h"

HINSTANCE hInst;

INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

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

    // Engine 초기화
    // 최상위 관리자
    if (FAILED(Engine::GetInst()->Init(hInstance, 1600, 900, true)))
    {
        return 0;
    }

    CreateTestLevel(); 
    
    // 메세지 루프
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAMECLIENT));
    MSG msg = {};

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
        } 
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
