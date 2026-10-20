#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <dwmapi.h>
#include <d3d11.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

#include <vector>
#include <thread>
#include <mutex> 

#include "men.h"
#include "unity.h"
#include "Vec.h"

using namespace Unity;
Ptr32 Uinty_Base = 0;
std::mutex g_DataM;

bool 对象名字 = false;
bool 对象骨骼 = false;

struct BoneDraw
{
    Transform Self{};
    Transform Parent{};
    std::vector<Transform> Child{};
};

struct Darw_GameObject_Data
{
    Vec3 GameObject_Pos;
    Vec2 GameObject_ScreenPos;
    std::vector<BoneDraw> GameObject_Boen_ScrPos;
    string GameObject_Name;
    std::vector<string> Comp_Name;

    Darw_GameObject_Data(){}
    Darw_GameObject_Data(Vec3 Pos, Vec2 ScreenPos, string Name, std::vector<string> CompNames, std::vector<BoneDraw> Boen_Pos) : GameObject_Pos(Pos), GameObject_ScreenPos(ScreenPos), GameObject_Name(Name), Comp_Name(CompNames), GameObject_Boen_ScrPos(Boen_Pos){}
};
std::vector<Darw_GameObject_Data> Darw_GameObject;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int screenWidth = GetSystemMetrics(SM_CXSCREEN);
int screenHeight = GetSystemMetrics(SM_CYSCREEN);

struct ImGuiContext_t {
    HWND                      window = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* device_context = nullptr;
    IDXGISwapChain* swap_chain = nullptr;
    ID3D11RenderTargetView* render_target_view = nullptr;
    bool                      running = true;
    bool                      bShowMenu = true;
    WNDCLASSEXW               wc{};
};

static ImGuiContext_t g_ui;

static LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    if (ImGui_ImplWin32_WndProcHandler(window, message, w_param, l_param))
        return 0L;

    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0L;
    }

    return DefWindowProc(window, message, w_param, l_param);
}

static bool ImGui_Init()
{
    g_ui.wc.cbSize = sizeof(WNDCLASSEXW);
    g_ui.wc.style = CS_HREDRAW | CS_VREDRAW;
    g_ui.wc.lpfnWndProc = window_procedure;
    g_ui.wc.hInstance = GetModuleHandleW(NULL);
    g_ui.wc.lpszClassName = L"ImGui";

    RegisterClassExW(&g_ui.wc);

    g_ui.window = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        g_ui.wc.lpszClassName,
        L"ImGui",
        WS_POPUP,
        0, 0, screenWidth, screenHeight,
        nullptr, nullptr, g_ui.wc.hInstance, nullptr
    );
    if (!g_ui.window) return false;

    SetLayeredWindowAttributes(g_ui.window, 0, BYTE(255), LWA_ALPHA);

    {
        RECT client_area{};
        GetClientRect(g_ui.window, &client_area);

        RECT window_area{};
        GetWindowRect(g_ui.window, &window_area);

        POINT diff{};
        ClientToScreen(g_ui.window, &diff);

        const MARGINS margins
        {
            window_area.left + (diff.x - window_area.left),
            window_area.top + (diff.y - window_area.top),
            client_area.right,
            client_area.bottom
        };

        DwmExtendFrameIntoClientArea(g_ui.window, &margins);
    }

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferDesc.RefreshRate.Numerator = 60U;
    sd.BufferDesc.RefreshRate.Denominator = 60U;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1U;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2U;
    sd.OutputWindow = g_ui.window;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    constexpr D3D_FEATURE_LEVEL levels[2]{ D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    D3D_FEATURE_LEVEL level{};

    D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0U,
        levels, 2U, D3D11_SDK_VERSION,
        &sd, &g_ui.swap_chain, &g_ui.device, &level, &g_ui.device_context);

    ID3D11Texture2D* back_buffer{ nullptr };
    if (g_ui.swap_chain)
        g_ui.swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

    if (!back_buffer) {
        return false;
    }

    g_ui.device->CreateRenderTargetView(back_buffer, nullptr, &g_ui.render_target_view);
    back_buffer->Release();

    ShowWindow(g_ui.window, SW_SHOW);
    UpdateWindow(g_ui.window);
    SetWindowLongW(g_ui.window, GWL_EXSTYLE,
        GetWindowLongW(g_ui.window, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);

    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig fontCfg;
    fontCfg.MergeMode = false;
    fontCfg.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 30.0f, &fontCfg,
        io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

    ImGui_ImplWin32_Init(g_ui.window);
    ImGui_ImplDX11_Init(g_ui.device, g_ui.device_context);

    return true;
}

static void ImGui_PollMessages()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT) g_ui.running = false;
    }

    if (GetAsyncKeyState(VK_INSERT) & 1) {
        g_ui.bShowMenu = !g_ui.bShowMenu;
        LONG style = GetWindowLong(g_ui.window, GWL_EXSTYLE);
        if (g_ui.bShowMenu) style &= ~WS_EX_TRANSPARENT;
        else                style |= WS_EX_TRANSPARENT;
        SetWindowLong(g_ui.window, GWL_EXSTYLE, style);
    }
}

static void ImGui_BeginFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

static void ImGui_EndFrame()
{
    ImGui::Render();

    constexpr float color[4]{ 0.f, 0.f, 0.f, 0.f };
    g_ui.device_context->OMSetRenderTargets(1U, &g_ui.render_target_view, nullptr);
    g_ui.device_context->ClearRenderTargetView(g_ui.render_target_view, color);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_ui.swap_chain->Present(1U, 0U);
}

static void ImGui_Shutdown()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_ui.swap_chain)         g_ui.swap_chain->Release();
    if (g_ui.device_context)     g_ui.device_context->Release();
    if (g_ui.device)             g_ui.device->Release();
    if (g_ui.render_target_view) g_ui.render_target_view->Release();

    if (g_ui.window)
        DestroyWindow(g_ui.window);

    UnregisterClassW(g_ui.wc.lpszClassName, g_ui.wc.hInstance);
}

struct CameraData {
    Camera Camera_Comp;
    Vec3 Pos;
    Mat4x4 W2C;
    Mat4x4 Proj;
}MainCamera;

std::vector<GameObject> GameObjects;
void FindMainCam() {
    MainCamera.Camera_Comp = GameObjectManager::FindGameObj_Name("GameCamera(Clone)", GameObjects).FindComp_Name("Camera").CompToTTT<Camera>();
    //printf("Camear addr %p\n", MainCamera.Camera_Comp.addr);
}

void ImGui_DrawMenu()
{  
    if (!g_ui.bShowMenu) return;

    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(390, 0), ImGuiCond_FirstUseEver);

    bool windowOpen = true;

    if (ImGui::Begin("GOM", &windowOpen))
    {
        
        ImGui::Text("游戏对象数量 %zu", GM.size());
        ImGui::Text("Camera %f %f %f", MainCamera.Pos.x, MainCamera.Pos.y, MainCamera.Pos.z);
        if (ImGui::Button("Start", ImVec2(100, 0)))Start();
        if (ImGui::Button("查找相机", ImVec2(100, 0)))FindMainCam();
        ImGui::Checkbox("对象名称", &对象名字);
        ImGui::Checkbox("对象骨骼", &对象骨骼);
    }
    ImGui::End();

    if (!windowOpen)
        g_ui.running = false;
}

void Unity::Start() {
    DWORD pid = get_pid_by_name(L"Human.exe");
    if (!pid) printf("没开游戏");

    hProc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    Uinty_Base = get_module_base(pid, L"UnityPlayer.dll");
    GOM_hash = read<Ptr32>(Uinty_Base + Offset::GOM_hash);
    Ptr32 Gomaddr = read<Ptr32>(Uinty_Base + Offset::GOM);
    GOM = GameObjectManager::Read(Gomaddr);
    typeTable = Uinty_Base + Offset::typeTable;
}

void Unity::Updata()
{
    std::vector<UnityData> gm;
    std::vector<Darw_GameObject_Data> draw;

    GameObjects.clear();
    for (auto& obj : GOM.List_traverse_1()) GameObjects.push_back(obj);
    for (auto& obj : GOM.List_traverse_2()) GameObjects.push_back(obj);

    for (auto& obj : GameObjects)
    {
        GB_Data GB(obj.GetName(), obj);
        //printf("Game Object Name %s\n", obj.GetName().c_str());

        std::vector<Component> Comps = obj.FindCompALL();
        std::vector<CP_Data> CP;
        std::vector<string> CompNames;
        Vec2 S_lodc;
        std::vector<BoneDraw> B_S_lodc;
		Vec3 WorldPos;
        for (auto& C : Comps)
        {
            std::string tn = C.GetTypeName();
            CP.push_back(CP_Data(tn, C));
            CompNames.push_back(tn);

            if (tn == "MonoBehaviour")
            {
                //std::string scriptName = C.CompToTTT<MonoBehaviour>().GetScriptName();
            }

            if (tn == "Transform" && 对象名字)
            {            
              MainCamera.Camera_Comp.WorldToScreen(C.CompToTTT<Transform>().GetWorldPos(), S_lodc, screenWidth, screenHeight);
            }

            if (tn == "SkinnedMeshRenderer" && 对象骨骼)
            {
                B_S_lodc.clear();
                SkinnedMeshRenderer SM = C.CompToTTT<SkinnedMeshRenderer>();

                std::vector<Transform> pending{ SM.GetBoneTransform(0) };
                for (int i = 0; i < pending.size(); i++) { 
                    Transform cur = pending[i];
                    BoneDraw bd{ cur, cur.GetParent(), cur.GetChild() };
                    for (auto& c : bd.Child) if (c.addr) pending.push_back(c);
                    B_S_lodc.push_back(bd);
                }
            }
        }

        gm.push_back(UnityData(GB, CP));
        draw.push_back(Darw_GameObject_Data(WorldPos, S_lodc, obj.GetName(), CompNames, B_S_lodc));
    }

    std::lock_guard<std::mutex> lk(g_DataM);
    GM.swap(gm);
    Darw_GameObject.swap(draw);
}

void ImGui_DrawGameObject()
{
    std::lock_guard<std::mutex> lk(g_DataM);
    ImDrawList* draw = ImGui::GetForegroundDrawList();

    if (对象名字)
    {
        for (auto& d : Darw_GameObject)
        {
            draw->AddText(ImVec2(d.GameObject_ScreenPos.x, d.GameObject_ScreenPos.y), IM_COL32(255, 255, 255, 255), d.GameObject_Name.c_str());
        }
    }

    if (对象骨骼)
    {
        for (auto& d : Darw_GameObject) {
            for (auto& Bone : d.GameObject_Boen_ScrPos) {
                Vec2 a, b;
                if (!MainCamera.Camera_Comp.WorldToScreen(Bone.Self.GetWorldPos(), a, screenWidth, screenHeight)) continue;
                if (!MainCamera.Camera_Comp.WorldToScreen(Bone.Parent.GetWorldPos(), b, screenWidth, screenHeight)) continue;

                draw->AddLine(ImVec2(a.x, a.y), ImVec2(b.x, b.y), IM_COL32(0, 0, 0, 255), 1.5f);
            }
        }
    }
}

void TTTTTT() {
    while (g_ui.running)
    {
        Unity::Updata();
        Sleep(8);
    }
}

int main()
{
    Unity::Start();
    ImGui_Init();
    //std::thread updateT(Uinty::Updata);

    std::thread update_T(TTTTTT);

    while (g_ui.running)
    {
        if (MainCamera.Camera_Comp.addr) {
            MainCamera.W2C = MainCamera.Camera_Comp.GetWorldToCameraMatrix();
            MainCamera.Proj = MainCamera.Camera_Comp.GetProjectionMatrix();
        }
        ImGui_PollMessages();
        if (!g_ui.running) break;
        ImGui_BeginFrame();
        ImGui_DrawMenu();
        //Uinty::Updata();
        ImGui_DrawGameObject();
        ImGui_EndFrame();
    }

    if (update_T.joinable())
        update_T.join();

    ImGui_Shutdown();
}