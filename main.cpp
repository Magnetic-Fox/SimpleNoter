#include <windows.h>
#include <ctl3d.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>

#include "resources.h"

#include "cp1250.hpp"
#include "wsprocs.hpp"
#include "noterapi.hpp"
#include "codepages.hpp"
#include "inihandling.hpp"

#define ID_BUTTON1  400
#define ID_BUTTON2  401
#define ID_BUTTON3  402
#define ID_BUTTON4  403

#define ID_LISTBOX  500

#define ID_STATIC1  600
#define ID_STATIC2  601
#define ID_STATIC3  602
#define ID_STATIC4  603
#define ID_STATIC5  604
#define ID_STATIC6  605

HBRUSH g_hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
char buffer[65536];
CODEPAGE m_cp1250;
NOTER_CONNECTION_SETTINGS connectionSettings;
NOTER_CREDENTIALS credentials;
NOTE_SUMMARY *notes=NULL;
long int noteCount=0;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void ShowInteger(long int integer)
{
    char test[20];
    ltoa(integer,test,10);
    MessageBox(0,test,"ShowInteger",MB_OK);
    return;
}

std::string IntToStr(long int input)
{
    std::string temp;
    char test[32];
    ltoa(input,test,10);
    temp=test;
    return temp;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    char path[256];
    GetModuleFileName(hInstance,path,256);
    std::string iniFile=getDefaultIniFile(path);
    connectionSettings=getConnectionSettings((char*)iniFile.c_str());
    credentials=getCredentials((char*)iniFile.c_str());
    prepareCodePage(m_cp1250,cp1250);

    if(wsInit() == SOCKET_ERROR)
    {
        MessageBox(NULL,"Inicjalizacja WinSocka nie powiod³a siê. Program zostanie zamkniêty.","B³¹d",MB_ICONSTOP | MB_OK);
        return 1;
    }
    
    LPSTR mainWindowClass = "SimpleNoterMain";
    LPSTR editWindowClass = "SimpleNoterEdit";
    
    WNDCLASS wc = { 0 };
    WNDCLASS wc2= { 0 };

    HWND hwnd, hwnd2;
    HWND hButton, hButton2, hButton3, hButton4;
    HWND hListBox;
    HWND hStatic, hStatic2, hStatic3, hStatic4, hStatic5, hStatic6;

    HACCEL hAccel;
    MSG Komunikat;
    
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = g_hBrush;
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
    wc.lpszClassName = mainWindowClass;

    if(!RegisterClass(&wc))
    {
        MessageBox(NULL,"Utworzenie klasy okna nie powiod³o siê. Program zostanie zamkniêty.","B³¹d",MB_ICONSTOP | MB_OK);
        return 1;
    }

    hwnd = CreateWindow(mainWindowClass, "Simple Noter v0.4", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        NULL, NULL, hInstance, NULL);
    if(hwnd == NULL)
    {
        MessageBox(NULL,"Utworzenie okna nie powiod³o siê. Program zostanie zamkniêty.","B³¹d",MB_ICONSTOP | MB_OK);
        return 1;
    }

    hAccel=LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));
    if(!hAccel)
    {
        MessageBox(NULL,"Nie uda³o siê za³adowaæ akceleratorów! Program zostanie zamkniêty.","B³¹d",MB_ICONSTOP | MB_OK);
        return 1;
    }

    hButton = CreateWindow("BUTTON", "Pobierz", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 80, 21,
                           hwnd, (HMENU)ID_BUTTON1, hInstance, NULL);

    hButton2= CreateWindow("BUTTON", "Utwórz", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 80, 0, 80, 21,
                           hwnd, (HMENU)ID_BUTTON2, hInstance, NULL);

    hButton3= CreateWindow("BUTTON", "Otwórz", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 160, 0, 80, 21,
                           hwnd, (HMENU)ID_BUTTON3, hInstance, NULL);

    hButton4= CreateWindow("BUTTON", "Zakoñcz", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 240, 0, 80, 21,
                           hwnd, (HMENU)ID_BUTTON4, hInstance, NULL);

    hStatic6= CreateWindow("STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_GRAYRECT, 320, 0, 280, 21, hwnd, (HMENU)ID_STATIC6, hInstance, NULL);

    hListBox= CreateWindow("LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_TABSTOP | ES_AUTOVSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                           0, 21, 600, 300, hwnd, (HMENU)ID_LISTBOX, hInstance, NULL);
    SetWindowPos(hListBox,NULL,0,21,600,300,SWP_NOZORDER);

    hStatic = CreateWindow("STATIC", "ID:", WS_CHILD | WS_VISIBLE | SS_LEFT, 8, 329, 128, 16, hwnd, (HMENU)ID_STATIC1, hInstance, NULL);
    hStatic2= CreateWindow("STATIC", "Ostatnie zmiany:", WS_CHILD | WS_VISIBLE | SS_LEFT, 8, 346, 128, 16, hwnd, (HMENU)ID_STATIC2, hInstance, NULL);
    hStatic3= CreateWindow("STATIC", "-- nie wybrano --", WS_CHILD | WS_VISIBLE | SS_LEFT, 137, 329, 454, 16, hwnd, (HMENU)ID_STATIC3, hInstance, NULL);
    hStatic4= CreateWindow("STATIC", "-- nie wybrano --", WS_CHILD | WS_VISIBLE | SS_LEFT, 137, 346, 454, 16, hwnd, (HMENU)ID_STATIC4, hInstance, NULL);
    hStatic5= CreateWindow("STATIC", "Wszystko w porz¹dku", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 370, 600, 16, hwnd, (HMENU)ID_STATIC5, hInstance, NULL);
    
    if(Ctl3dRegister(hInstance) && Ctl3dEnabled())
    {
        unsigned int ctlRegs=(CTL3D_ALL) & ~(CTL3D_STATICFRAMES);
        // unsigned int ctlRegs=CTL3D_ALL;
        //Ctl3dSubclassDlg(hwnd,ctlRegs);
        
        /*
        Ctl3dSubclassCtl(hPrzycisk);
        Ctl3dSubclassCtl(hPrzycisk2);
        Ctl3dSubclassCtl(hPrzycisk3);
        
        Ctl3dSubclassCtl(hText);
        Ctl3dSubclassCtl(hCheckBox);
        Ctl3dSubclassCtl(hListBox);
        Ctl3dSubclassCtl(hListBox2);
        Ctl3dSubclassCtl(hText2);
        */
        //Ctl3dAutoSubclass(hInstance);
    }
    else
    {
        MessageBox(hwnd,"Zarejestrowanie CTL3D nie powiod³o siê!","Ostrze¿enie",MB_ICONEXCLAMATION);
    }

    ShowWindow(hwnd,nCmdShow);
    UpdateWindow(hwnd);

    while(GetMessage(&Komunikat, NULL, 0, 0 ))
    {
        HWND temp=GetParent(Komunikat.hwnd);
        if(temp==NULL)
        {
            temp=Komunikat.hwnd;
        }
        if(!TranslateAccelerator(temp, hAccel, &Komunikat))
        {
            TranslateMessage(&Komunikat);
            DispatchMessage(&Komunikat);
        }
    }
    
    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CTLCOLOR:
            switch(HIWORD(lParam))
            {
                case CTLCOLOR_STATIC:
                    if((LOWORD(lParam)!=GetDlgItem(hwnd,ID_STATIC5)) && (LOWORD(lParam)!=GetDlgItem(hwnd,ID_STATIC6)))
                    {
                        SetBkMode((HDC)wParam, TRANSPARENT);
                        return g_hBrush;
                    }
                    break;
                case CTLCOLOR_BTN:
                    SetBkMode((HDC)wParam, TRANSPARENT);
                    break;
            }
            break;
        case WM_COMMAND:
            switch(wParam)
            {
                case ID_ACC_TAB:
                    SetFocus(GetNextDlgTabItem(hwnd,GetFocus(),false));
                    break;
                case ID_BUTTON1:
                    if(noteCount>0)
                    {
                        freeNoteList(notes);
                    }
                    noteCount=noter_getNoteList(connectionSettings,credentials,buffer,notes);
                    SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_RESETCONTENT, 0, 0);
                    for(long int x=0; x<noteCount; ++x)
                    {
                        SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_ADDSTRING, 0, (LPARAM)toCodePage(m_cp1250,(char*)notes[x].subject.c_str()).c_str());
                    }
                    break;
                case ID_LISTBOX:
                    switch(HIWORD(lParam))
                    {
                        case LBN_DBLCLK:
                            break;
                        case LBN_SELCHANGE:
                            long int index=SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0);
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC3),IntToStr(notes[index].id).c_str());
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC4),notes[index].lastModified.c_str());
                            break;
                    }
                    break;
            }
            break;
        case WM_SIZE:
            unsigned long int width= LOWORD(lParam);
            unsigned long int height=HIWORD(lParam);

            SetWindowPos(GetDlgItem(hwnd,ID_LISTBOX),NULL,0,0,width,height-86,SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC6),NULL,0,0,width-320,21,SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC1),NULL,8,height-57,0,0,SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC2),NULL,8,height-40,0,0,SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC3),NULL,137,height-57,width-146,16,SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC4),NULL,137,height-40,width-146,16,SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC5),NULL,0,height-16,width,16,SWP_NOZORDER);

            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            if(Ctl3dEnabled() && (!Ctl3dUnregister(GetWindowWord(hwnd,GWW_HINSTANCE))))
            {
                MessageBox(0,"Wyrejestrowanie aplikacji z CTL3D nie powiod³o siê!","Ostrze¿enie",MB_ICONEXCLAMATION);
            }
            WSACleanup();
            if(noteCount>0)
            {
                freeNoteList(notes);
            }
            DeleteObject(g_hBrush);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd,msg,wParam,lParam);
    }
    return 0;
}
