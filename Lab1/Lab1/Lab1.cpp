// Lab1.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "Lab1.h"
#include "RenderClass.h"
#include <dxgi.h>
#include <d3d11.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")

#define MAX_LOADSTRING 100
                          
WCHAR szTitle[MAX_LOADSTRING]=L"Lab1 Lapina Olga";
WCHAR szWindowClass[MAX_LOADSTRING]=L"Lab1Class";

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

RenderClass*         g_Render = nullptr;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        OutputDebugString(_T("Error in Init\n"));
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAB1));


    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            //OutputDebugString(_T("Render\n"));
            g_Render->Render();
        }
    }

    g_Render->Terminate();
    delete g_Render;

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.hInstance      = hInstance;
    wcex.hbrBackground  = NULL;
    wcex.lpszClassName  = szWindowClass;

    return RegisterClassExW(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
        nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      OutputDebugString(_T("Error in Create window\n"));
      return FALSE;
   }

   g_Render = new RenderClass();
   if (FAILED(g_Render->Init(hWnd)))
   {
       OutputDebugString(_T("Error in Init Renderer\n"));
       delete g_Render;
       return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        if (g_Render != nullptr && wParam != SIZE_MINIMIZED)
        {
            g_Render->Resize(hWnd);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

