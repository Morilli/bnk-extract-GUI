#include <windows.h>
#include <stdbool.h>
#include <stdio.h>

#include "resource.h"


HWND CreateEmptyWindow(HWND parent, HINSTANCE hInstance, int X, int Y, int nWidth, int nHeight, LPCSTR lpWindowName, WNDPROC WindowProcedure)
{
    const char className[] = "TemplateWindow";

    WNDCLASSEX newWindowClass = {
        .cbSize        = sizeof(WNDCLASSEX),
        .style         = WS_EX_CLIENTEDGE,
        .lpfnWndProc   = WindowProcedure,
        .cbClsExtra    = 0,
        .cbWndExtra    = 0,
        .hInstance     = hInstance,
        .hIcon         = 0,
        .hIconSm       = 0,
        .hCursor       = LoadCursor(NULL, IDC_ARROW),
        .hbrBackground = (HBRUSH)(COLOR_WINDOWFRAME),
        .lpszMenuName  = MAKEINTRESOURCE(IDR_MYMENU),
        .lpszClassName = className
    };
    printf("got here\n");

    if (!RegisterClassEx(&newWindowClass)) {
        // MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return NULL;
    }
    printf("abc\n");

    HWND newWindow = CreateWindowEx(WS_EX_CLIENTEDGE, className, lpWindowName, WS_OVERLAPPEDWINDOW, X, Y, nWidth, nHeight, parent, NULL, hInstance, NULL);
    ShowWindow(newWindow, SW_SHOWNORMAL);

    printf("got here also\n");
    return newWindow;
}
