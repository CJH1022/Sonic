#include "pch.h"
#include "EditorMgr.h"

#include "Imgui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

#include "Engine.h"
#include "Device.h"
#include "KeyMgr.h"
#include "RenderMgr.h"

#include "Menu.h"
#include "Inspector.h"
#include "Outliner.h"
#include "ImageEditor.h"
#include "ContentUI.h"
#include "ListUI.h"
#include "TreeUI.h"

#include "CEditorCamMoveScript.h"

EditorMgr::EditorMgr()
    : m_ShowDemo(false)
{

}

EditorMgr::~EditorMgr()
{
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}


void EditorMgr::Init()
{
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    //io.ConfigDockingAlwaysTabBar = true;
    //io.ConfigDockingTransparentPayload = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)
    io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
    io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(Engine::GetInst()->GetMainWndHwnd());
    ImGui_ImplDX11_Init(DEVICE, CONTEXT);

    // 게임 에디터 UI 만들기
    CreateEditorUI();    

    // Editor 용 GameObject 만들기
    CreateEditorObject();
}

void EditorMgr::Progress()
{
    Tick();

    Render();
}

void EditorMgr::Tick()
{
    // =============
    // Editor Object
    // =============
    {
        for (const auto& Object : m_EditorObject)        
            Object->Tick();

        for (const auto& Object : m_EditorObject)
            Object->FinalTick_Editor();
    }

    // ========
    // EditorUI
    // ========
    {
        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();


        m_FocusedUI = nullptr;

        if (KEY_TAP(KEY::ENTER))
            ImGui::SetWindowFocus(nullptr);

        // DemoUI
        if (m_ShowDemo)
            ImGui::ShowDemoWindow(&m_ShowDemo);

        // EditorUI
        for (const auto& pair : m_mapUI)
        {
            if (pair.second->IsActive())
                pair.second->Tick();
        }

        if (nullptr != m_FocusedUI)
            KeyMgr::GetInst()->SetActive(false);
        else
            KeyMgr::GetInst()->SetActive(true);
    }
}

void EditorMgr::Render()
{
    // Rendering
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void EditorMgr::CreateEditorUI()
{
    Ptr<EditorUI> pUI = nullptr;

    pUI = new Menu;
    AddUI(pUI->GetUIName(), pUI);

    pUI = new Inspector;
    AddUI(pUI->GetUIName(), pUI);

    pUI = new Outliner;
    AddUI(pUI->GetUIName(), pUI);

    pUI = new ContentUI;
    AddUI(pUI->GetUIName(), pUI);

    pUI = new ImageEditor;
    AddUI(pUI->GetUIName(), pUI);
    
    pUI = new ListUI;
    pUI->SetModal(true);
    pUI->SetActive(false);
    AddUI(pUI->GetUIName(), pUI);
}

void EditorMgr::CreateEditorObject()
{
    // Editor Camera Object 생성
    Ptr<GameObject> pObject = new GameObject;
    pObject->SetName(L"EditorCamera");

    pObject->AddComponent(new CTransform);
    pObject->AddComponent(new CCamera);
    pObject->AddComponent(new CEditorCamMoveScript);

    pObject->Camera()->LayerCheckAll();

    pObject->Camera()->SetProjType(PROJ_TYPE::ORTHOGRAPHIC);
    pObject->Camera()->SetFar(10000.f);
    pObject->Camera()->SetFOV(90.f);
    pObject->Camera()->SetOrthoScale(1.f);

    Vec2 vResolution = Device::GetInst()->GetRenderResolution();
    pObject->Camera()->SetAspectRatio(vResolution.x / vResolution.y); // 종횡비(AspectRatio)
    pObject->Camera()->SetWidth(vResolution.x);

    m_EditorObject.push_back(pObject);

    // Editor 용 카메라로서 RenderMgr 에 등록
    RenderMgr::GetInst()->RegisterEditorCamera(pObject->Camera());
}

void EditorMgr::AddUI(const string& _UIName, Ptr<EditorUI> _UI)
{
    Ptr<EditorUI> pUI = FindUI(_UIName);
    assert(nullptr == pUI);
    m_mapUI.insert(make_pair(_UIName, _UI));
}

Ptr<EditorUI> EditorMgr::FindUI(const string& _UIName)
{
    map<string, Ptr<EditorUI>>::iterator iter = m_mapUI.find(_UIName);

    if (iter == m_mapUI.end())
        return nullptr;

    return iter->second;
}