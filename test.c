#include <windows.h>
#include <stdio.h>
// #include <stdlib.h>
#include <dwmapi.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

#include "resource.h"
#include "templatewindow.h"

const char* string_array[] = {"string1", "secondstring", "anothertext", "uwu", "onii-chan", "bruder muss los"};
const char g_szClassName[] = "myWindowClass";
static HINSTANCE me;

#define WM_RETICULATE_SPLINES (WM_USER + 0x0001)

static HWND edit_text;
static HWND mainWindow;

void* updateProgressBar(void* _args)
{
    HWND progress_bar;
    memcpy(&progress_bar, _args, sizeof(HWND));

    SendMessage(progress_bar, PBM_SETRANGE, 0, MAKELONG(0, 5000));
    for (int i = 0; i < 500; i++) {
        // SendMessage(progress_bar, PBM_STEPIT, 0, 0);
        // SendMessage(progress_bar, PBM_STEPIT, 0, 0);
        // SendMessage(progress_bar, PBM_STEPIT, 0, 0);
        // SendMessage(progress_bar, PBM_STEPIT, 0, 0);
        SendMessage(progress_bar, PBM_STEPIT, 0, 0);
        Sleep(rand() % 100);
    }

    while (true) {
        HWND new_progress_bar = (HWND) SendMessage(mainWindow, WM_RETICULATE_SPLINES, 0, 0);
        // HWND new_progress_bar = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, rand() % 500, rand() % 500, 200, GetSystemMetrics(SM_CYVSCROLL), mainWindow, NULL, me, NULL);
        SendMessage(new_progress_bar, PBM_SETRANGE, 0, MAKELONG(0, 1000));
        for (int i = 0; i < 100; i++) {
            SendMessage(new_progress_bar, PBM_STEPIT, 0, 0);
            Sleep(rand() % 100);
        }
    }

    HWND second_progress_bar;
    memcpy(&second_progress_bar, _args + sizeof(HWND), sizeof(HWND));
    SendMessage(second_progress_bar, PBM_SETRANGE, 0, MAKELONG(0, 5000));
    for (int i = 0; i < 500; i++) {
        // SendMessage(progress_bar, PBM_STEPIT, 0, 0);
        // SendMessage(progress_bar, PBM_STEPIT, 0, 0);
        // SendMessage(progress_bar, PBM_STEPIT, 0, 0);
        // SendMessage(progress_bar, PBM_STEPIT, 0, 0);
        SendMessage(second_progress_bar, PBM_STEPIT, 0, 0);
        Sleep(rand() % 100);
    }


    return NULL;
}

int get_number()
{
    static int number = 1;
    number *= 2;
    // number++;
    return number;
}

int windows_created;

pthread_t worker_thread;


// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_RETICULATE_SPLINES: {
            HWND newProgressBar = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, rand() % 750, rand() % 750, 200, GetSystemMetrics(SM_CYVSCROLL), mainWindow, NULL, me, NULL);
            return (LRESULT) newProgressBar;
        }
        case WM_CLOSE: {
            MessageBox(hwnd, "Fenster wird geschlossen...", NULL, MB_ICONQUESTION);
            // INITCOMMONCONTROLSEX icex = {
            //     .dwICC = ICC_PROGRESS_CLASS,
            //     .dwSize = sizeof(INITCOMMONCONTROLSEX)
            // };
            // InitCommonControlsEx(&icex);
            // HWND progress_bar = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE, 10, 10, 100, GetSystemMetrics(SM_CYVSCROLL), hwnd, NULL, me, NULL);

            // for (int i = 0; i < 10; i++) {
            //     SendMessage(progress_bar, PBM_STEPIT, 0, 0);
            //     Sleep(1000);
            // }
            DestroyWindow(hwnd);
            // char integer[18];
            // sprintf(integer, "%d", get_number());
            // sprintf(integer + 10, "%d", rand());
            // WinMain(NULL, NULL, integer, SW_SHOWNORMAL);
            break;
        }
        case WM_DESTROY:
            // MessageBox(hwnd, "nice one", "what", 0);
            PostQuitMessage(0);
            break;
        // case
        case WM_COMMAND:
            // printf("got here wm_command\n");
            if (lParam && HIWORD(wParam) == BN_CLICKED) {
                char* IQ_text = calloc(100, 1);
                int iq;
                int unlikely = 0;
                INITCOMMONCONTROLSEX icex = {
                    .dwICC = ICC_PROGRESS_CLASS,
                    .dwSize = sizeof(INITCOMMONCONTROLSEX)
                };
                InitCommonControlsEx(&icex);
                HWND progress_bar_text = CreateWindowEx(0, "STATIC", NULL, WS_CHILD | WS_VISIBLE, 10, 0, 2000, GetSystemMetrics(SM_CYVSCROLL), hwnd, NULL, me, NULL);
                SetWindowText(progress_bar_text, "Deleting system32...");
                HWND progress_bar = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 10, 20, 500, GetSystemMetrics(SM_CYVSCROLL), hwnd, NULL, me, NULL);
                void* to_leak = malloc(sizeof(HWND) * 2);
                memcpy(to_leak, &progress_bar, sizeof(HWND));
                HWND second_progress_bar = CreateWindowEx(WS_EX_STATICEDGE, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH | WS_CLIPSIBLINGS, 500+10, 20, 500, GetSystemMetrics(SM_CYVSCROLL), hwnd, NULL, me, NULL);
                SetWindowTheme(second_progress_bar, NULL, NULL);
                LONG style = GetWindowLong(second_progress_bar, GWL_EXSTYLE);
                style &= ~WS_EX_STATICEDGE;
                SetWindowLong(second_progress_bar, GWL_EXSTYLE, style);
                SendMessage(second_progress_bar, PBM_SETBKCOLOR, 0, RGB(255, 255, 255));
                memcpy(to_leak + sizeof(HWND), &second_progress_bar, sizeof(HWND));
                // mainWindow = hwnd;
                pthread_create(&worker_thread, NULL, updateProgressBar, to_leak);
                // pthread_detach(worker_thread);

                while (1) {
                    iq = rand() % 150;
                    if (iq < 70) continue;
                    int distance_from_100 = abs(iq - 100);
                    unlikely += distance_from_100;
                    if (distance_from_100 <= unlikely / 4) break;
                }
                // FILE* textfileiguess = fopen("G:\\Dokumente\\Google Takeout.7z", "rb");
                // fseek(textfileiguess, 0, SEEK_END);
                // size_t file_size = ftello64(textfileiguess);
                // char* text = malloc(file_size + 1);
                // rewind(textfileiguess);
                // fread(text, 1, file_size, textfileiguess);
                // fclose(textfileiguess);
                // sprintf(IQ_text, "\r\nCalculated IQ: %d", iq);
                SetWindowText(edit_text, IQ_text);
            }
            break;
        case WM_NCHITTEST: {
            UINT nPos = DefWindowProc(hwnd, msg, wParam, lParam);
            if ( nPos == HTCAPTION ) return HTNOWHERE;
            return nPos;
            break;
        }
        default:
            // printf("got here with msg %d\n", msg);
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, __attribute__((unused)) HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    srand(time(NULL));
    me = hInstance;
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    //Step 1: Registering the Window Class

    int icon_size = GetSystemMetrics(SM_CXICON);
    int iconsm_size = GetSystemMetrics(SM_CXSMICON);
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadImage(hInstance, MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, icon_size, icon_size, 0);
    wc.hIconSm       = LoadImage(hInstance, MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, iconsm_size, iconsm_size, 0);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MYMENU);
    wc.lpszClassName = hInstance ? g_szClassName : &lpCmdLine[10];

    if(!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Step 2: Creating the Window
    for (; windows_created < (hInstance ? 1 : strtol(lpCmdLine, NULL, 10)); windows_created++) {
        // int x = ((windows_created / 3) * 480 * 9 / 7) % 1920;
        // int y = (360 * 9 / 7 * windows_created) % 1080;
        int w = 300;
        int h = 200;
        // int x = (425 * windows_created) % 1920;
        // int y = (317 * windows_created) % 1080;
        int x = CW_USEDEFAULT;
        int y = CW_USEDEFAULT;
        RECT rect;
        rect.left   = x;
        rect.top    = y;
        rect.right  = x + w;
        rect.bottom = y + h;
        UINT style = WS_OVERLAPPEDWINDOW ;
        AdjustWindowRectEx(&rect, style, 0, 0);
        // DwmGetWindowAttribute()
        char window_name[30] = "high quality gui ";
        sprintf(window_name + 21, "%d", windows_created);
        hwnd = CreateWindowEx(
            WS_EX_CLIENTEDGE | WS_EX_LAYERED,
            g_szClassName,
            window_name,
            style,
            CW_USEDEFAULT, CW_USEDEFAULT, rect.right-rect.left, rect.bottom-rect.top,
            NULL, NULL, hInstance, NULL);
        mainWindow = hwnd;
        // AdjustWindowRect(&rect, 0, 0);
        // SetWindowPos(hwnd, NULL, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, 0);
        SetLayeredWindowAttributes(hwnd, 0, 230, LWA_ALPHA);

        // button
        for (int i = 0; i < 100; i++) {

            HWND childWindow = CreateEmptyWindow(hwnd, hInstance, i, i, 100, 100, "testwindownew", WndProc);


        CreateWindow(
            "BUTTON",  // Predefined class; Unicode assumed
            "Ok",      // Button text
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_CHECKBOX,  // Styles
            rand() % 1000,         // x position
            rand() % 1000,         // y position
            100,        // Button width
            60,        // Button height
            childWindow,     // Parent window
            (HMENU) IDOK,       // No menu.
            hInstance,
            NULL
        );      // Pointer not needed.
        CreateWindow(
            "BUTTON",  // Predefined class; Unicode assumed
            "Ok",      // Button text
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_CLIPSIBLINGS,  // Styles
            rand() % 1000,         // x position
            rand() % 1000,         // y position
            100,        // Button width
            60,        // Button height
            hwnd,     // Parent window
            NULL,       // No menu.
            hInstance,
            NULL
        );      // Pointer not needed.
        SetClassLongPtr(childWindow, GWLP_WNDPROC, (LONG_PTR) WndProc);

        // window.
        }

        // text above the button
        edit_text = CreateWindow(
            "EDIT",
            NULL,
            WS_BORDER | WS_CHILD | WS_VISIBLE | ES_CENTER | ES_READONLY | ES_MULTILINE,
            80,30,140,55,
            hwnd,
            NULL,
            hInstance,
            NULL
        );
        printf("x pos: %d, y pos: %d\n", (windows_created / 3) * 480, (360 * windows_created) % 1080);

        if(hwnd == NULL)
        {
            MessageBox(NULL, "Window Creation Failed!", "Error!",
                MB_ICONEXCLAMATION | MB_OK);
            return 0;
        }

        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
    }

    // Step 3: The Message Loop
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
