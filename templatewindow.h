#ifndef TEMPLATEWINDOW_H
#define TEMPLATEWINDOW_H

#include <windows.h>
#include <stdbool.h>

HWND CreateEmptyWindow(HWND parent, HINSTANCE hInstance, int X, int Y, int nWidth, int nHeight, LPCSTR lpWindowName, WNDPROC WindowProcedure);


#endif
