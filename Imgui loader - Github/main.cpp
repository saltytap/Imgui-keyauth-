#include "Main.h"
#include <Windows.h>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <winbase.h>
#include <tchar.h>
#include "auth.hpp"
#include <CommCtrl.h>
#include <urlmon.h>
#pragma comment(lib,"urlmon.lib")
#include <filesystem>


#include "xorstr.hpp"
#include <tlhelp32.h>
#include <thread>
#include <random>


#include <Psapi.h>
#include <chrono>
#include <future>

#pragma comment(lib, "psapi.lib")

IDirect3DTexture9* masterlogo;

using namespace KeyAuth;


// made by Kasper#8666

//place your secrets here
std::string name = "";
std::string ownerid = "";
std::string secret = "";
std::string version = "1";
std::string url = "https://keyauth.win/api/1.2/";
std::string sslPin = "ssl pin key (optional)"; // don't change unless you intend to pin public certificate key. you can get here in the "Pin SHA256" field https://www.ssllabs.com/ssltest/analyze.html?d=ke
api KeyAuthApp(name, ownerid, secret, version, url, sslPin);

//Makes folder to place exe 
std::string path = "";
std::string svpath = "";
std::string file = ("");
std::fstream skey;
std::string key;


static int width = 350;
static int height = 200;


char Licence[50] = "";


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


bool LoginCheck = false;

typedef NTSTATUS(WINAPI* lpQueryInfo)(HANDLE, LONG, PVOID, ULONG, PULONG);

//all over the internet, i didnt make this
PVOID DetourFunc(BYTE* src, const BYTE* dst, const int len)
{
    BYTE* jmp = (BYTE*)malloc(len + 5); DWORD dwback;
    VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &dwback);
    memcpy(jmp, src, len); jmp += len; jmp[0] = 0xE9;

    *(DWORD*)(jmp + 1) = (DWORD)(src + len - jmp) - 5; src[0] = 0xE9;
    *(DWORD*)(src + 1) = (DWORD)(dst - src) - 5;

    VirtualProtect(src, len, dwback, &dwback);
    return (jmp - len);
}

//not proper way to detour, but since we arent continuing thread context we dont return context.
//to continue thread execution after detour do something like this I think
//void CaptureThread(PCONTEXT context, PVOID arg1, PVOID arg2)
//return (new ldrThunk) -> Thunk name(PCONTEXT context, PVOID arg1, PVOID arg2) <- current thread context.

void CaptureThread()
{
    //getting thread start address isnt needed, it just gives extra information on the thread stack which allows you to see some potential injection methods used
    auto ThreadStartAddr = [](HANDLE hThread) -> DWORD {

        //Hook NtQueryInformationThread
        lpQueryInfo ThreadInformation = (lpQueryInfo)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryInformationThread");

        DWORD StartAddress;
        //Get information from current thread handle
        ThreadInformation(hThread, 9, &StartAddress, sizeof(DWORD), NULL);

        return StartAddress;
    };

    //Gets handle of current thread. (HANDLE)(LONG_PTR)-1 is handle of CurrentProcess if you need it
    HANDLE CurrentThread = (HANDLE)(LONG_PTR)-2;
    //Gets thread information from thread handle.
    DWORD  StartAddress = ThreadStartAddr(CurrentThread);

    //address 0x7626B0E0 is a static address which is assigned to exit thread of the application
    //we need to whitelist it otherwise you cant close the application from usermode
    if (StartAddress != 0x7626B0E0) {
        printf("\n[+] Block [TID: %d][Start Address: %p]", (DWORD)GetThreadId(CurrentThread), (CHAR*)StartAddress);
        //Exits thread and stops potential code execution
        //if you dont term thread it will crash if you dont handle context properly
        if (!TerminateThread(CurrentThread, 0xC0C)) exit(0);
    }
    else exit(0);
}

BOOL HookLdrInitializeThunk()
{
    //Gets handle of ntdll.dll in the current process, which allows us to detour LdrInitializeThunk calls in given context
    HMODULE hModule = LoadLibraryA("ntdll.dll");
    if (hModule && (PBYTE)GetProcAddress(hModule, reinterpret_cast<LPCSTR>("LdrInitializeThunk")))
    {
        DetourFunc((PBYTE)GetProcAddress(hModule, "LdrInitializeThunk"), (PBYTE)CaptureThread, 5);
        return TRUE;
    }
    else return FALSE;
}

//you can also hook RtlGetFullPathName_U to get path of module loaded, but it was not worth it because 
//RtlGetFullPathName_U only get path after module was loaded which can be a insecurity (maybe).
//though hooking RtlGetFullPathName_U doesnt point to the right location on some manual map injectors but it works for Xenos and AlisAlias injector

// This was made by ShadowMonster#2247 So credit to him

int AntiCrack()
{
    //Havent tested all kernel injection methods
    if (HookLdrInitializeThunk()) printf("[+] Hook Success");
    else printf("[-] Hook Failed");

    std::promise<void>().get_future().wait();

    return 0;
}


int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{




    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, LOADER_BRAND, NULL };
    RegisterClassEx(&wc);
    main_hwnd = CreateWindow(wc.lpszClassName, LOADER_BRAND, WS_POPUP, 0, 0, 5, 5, NULL, NULL, wc.hInstance, NULL);


    if (!CreateDeviceD3D(main_hwnd)) {
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }


    ShowWindow(main_hwnd, SW_HIDE);
    UpdateWindow(main_hwnd);


    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();




    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {


        void Theme(); {

            ImGui::GetStyle().FrameRounding = 4.0f;
            ImGui::GetStyle().GrabRounding = 4.0f;

            ImVec4* colors = ImGui::GetStyle().Colors;
            ImGuiStyle* style = &ImGui::GetStyle();

            style->WindowPadding = ImVec2(15, 15);
            style->WindowRounding = 5.0f;
            style->FramePadding = ImVec2(5, 5);
            style->FrameRounding = 4.0f;
            style->ItemSpacing = ImVec2(12, 8);
            style->ItemInnerSpacing = ImVec2(8, 6);
            style->IndentSpacing = 25.0f;
            style->ScrollbarSize = 15.0f;
            style->ScrollbarRounding = 9.0f;
            style->GrabMinSize = 5.0f;
            style->GrabRounding = 3.0f;

            style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
            style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
            style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
            style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
            style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
            style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
            style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
            style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
            style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
            style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
            style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
            style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
            style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
            style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
            style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
            style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
            style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
            style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
            style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
            style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
            style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
            style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
            style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
            style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
            style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
            style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
            style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
            style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
            style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
            style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
            style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
            style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
            style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);



        }
    }

    ImGui_ImplWin32_Init(main_hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);




    DWORD window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize;
    RECT screen_rect;
    GetWindowRect(GetDesktopWindow(), &screen_rect);
    auto x = float(screen_rect.right - width) / 2.f;
    auto y = float(screen_rect.bottom - height) / 2.f;

    static int Tabs = 2;

    void hideStartupConsoleOnce();
    {
        HWND Stealth;
        AllocConsole();
        Stealth = FindWindowA("ConsoleWindowClass", NULL);
        ShowWindow(Stealth, 0);
    }

    bool InfWindow = false;
    bool login = true;
    static int switchTabs = 3;
    KeyAuthApp.init();

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));



    //main loop start here
    while (msg.message != WM_QUIT && !LoginCheck)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }


        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        {
            //login form test
        

            ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Once);

         
                //Window start render here
               
                ImGui::Begin("Panama Private", &loader_active, window_flags);
                {


                   
                        if (login)
                        {

                            if (Tabs == 2)
                            {

                                //removes custom folder with exe inside
                                std::filesystem::remove_all(path);

                            // reset point
                            start:


                                ImGui::Text("Licence:");


                                skey.close(); //close the file object


                                ImGui::InputText("  ", Licence, IM_ARRAYSIZE(Licence));


                                //Maybe add it as a clickable link like the real one
                                ImGui::SetCursorPos({ 150, 100 });




                                ImGui::SameLine();


                                if (ImGui::Button("Login##Log", ImVec2(100, 25)))
                                {
                                    KeyAuthApp.license(Licence);

                                    if (KeyAuthApp.checkblack()) {
                                        abort();
                                    }

                                    if (!KeyAuthApp.data.success)
                                    {
                                        //check if the user is blacklisted
                                        if (KeyAuthApp.checkblack()) {
                                            abort();
                                        }

                                        // MessageBox(NULL, TEXT((KeyAuthApp.data.message.c_str())), TEXT("Failed"), MB_OK);
                                        //saves a txt file with expired 
                                        skey.open("saved key.txt", std::ios::out);
                                        skey << "Expired";
                                        skey.close();
                                        Sleep(1000);
                                        //goes to start so it doesnt crash
                                        goto start;

                                    }

                                    else {

                                        //Checks if the user is blacklisted
                                        if (KeyAuthApp.checkblack()) {
                                            abort();
                                        }

                                        //saves key to txt inside same folder
                                        skey.open("saved key.txt", std::ios::out);
                                        skey << Licence;
                                        skey.close();


                                        KeyAuthApp.license(Licence);

                                        //renders new window
                                        login = false;
                                        InfWindow = true;

                                    }

                                }


                                ImGui::Text("Status: % s", KeyAuthApp.data.message.c_str());
                                ImGui::Text("Version: % s", KeyAuthApp.data.version.c_str());
                                ImGui::Text("Coded by Panama#3750");

                            }
                        }
                
        
            

                if (InfWindow == true) {




                    
                        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Once);
                       

                        ImGui::Begin(LOADER_BRAND, &loader_active, window_flags);
                        {

                           
                            //saves license to txt
                            skey.open("saved key.txt", std::ios::out);
                            skey << Licence;
                            skey.close();


                            ImGui::Separator();
                           
                            
                            //window inside when the user have logged in sucessfully
                            ImGui::Text("Welcome to Project EFT");
                            ImGui::Text("Hwid: % s", KeyAuthApp.data.hwid.c_str());
                            ImGui::Text("Days left: % s", KeyAuthApp.data.expiry.c_str());
                            ImGui::Separator();
                            
                            if (ImGui::Button("Load Cheat##Cheat", ImVec2(100, 25)))
                            {

                                //here goes your main code
                                MessageBox(NULL, TEXT(("Coming Soon....")), TEXT("Panama"), MB_OK);
                               
                            }
                           

                        }
                    
                }
            }




            ImGui::End();
        }
        ImGui::EndFrame();

        

        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }


        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);


        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
            ResetDevice();
        }
        if (!loader_active) {
            msg.message = WM_QUIT;
        }
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(main_hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);

}







































