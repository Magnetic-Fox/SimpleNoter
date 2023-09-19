#include <windows.h>
#include <ctl3d.h>
#include <string>
#include <map>

#include "resources.h"

#include "debug.hpp"
#include "cp1250.hpp"
#include "wsprocs.hpp"
#include "noterapi.hpp"
#include "codepages.hpp"
#include "constants.hpp"
#include "conversion.hpp"
#include "inihandling.hpp"

#define ID_BUTTON1          1400
#define ID_BUTTON2          1401
#define ID_BUTTON3          1402
#define ID_BUTTON4          1403
#define ID_BUTTON5          1404
#define ID_LISTBOX          1500
#define ID_STATIC1          1600
#define ID_STATIC2          1601
#define ID_STATIC3          1602
#define ID_STATIC4          1603
#define ID_STATIC5          1604
#define ID_STATIC6          1605

#define ID_EDIT_BUTTON1     2400
#define ID_EDIT_BUTTON2     2401
#define ID_EDIT_BUTTON3     2402
#define ID_EDIT_EDITBOX1    2500
#define ID_EDIT_EDITBOX2    2501
#define ID_EDIT_STATIC1     2600
#define ID_EDIT_STATIC2     2601
#define ID_EDIT_STATIC3     2602
#define ID_EDIT_STATIC4     2603

LPSTR editWindowClass = "SimpleNoterEdit";

typedef struct editWindow
{
    HWND hwnd;
    HWND hStatic, hStatic2, hStatic3, hStatic4;
    HWND hEditBox, hEditBox2;
    HWND hButton, hButton2, hButton3;
    std::string windowTitle;
    bool subjectChanged, entryChanged;
    long int lastResult;
    NOTE *note;
} EDITWINDOW;

typedef std::map<HWND,EDITWINDOW*> WINDOWMEMORY;

HBRUSH g_hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
WINDOWMEMORY winMem;
HWND g_hwnd;
char buffer[65536];
CODEPAGE m_cp1250;
NOTER_CONNECTION_SETTINGS connectionSettings;
NOTER_CREDENTIALS credentials;
NOTE_SUMMARY *notes=NULL;
long int noteCount=0;
long int mainLastResult=0;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc2(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc2(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc3(HWND, UINT, WPARAM, LPARAM);

void inline lockExitButton(HWND hwnd)
{
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON4),false);
    return;
}

void inline unlockExitButton(HWND hwnd)
{
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON4),true);
    return;
}

void inline lockRefreshButton(HWND hwnd)
{
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON1),false);
    return;
}

void inline unlockRefreshButton(HWND hwnd)
{
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON1),true);
    return;
}

void inline lockOpenButton(HWND hwnd)
{
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON3),false);
    return;
}

void inline unlockOpenButton(HWND hwnd)
{
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON3),true);
    return;
}

void inline lockDeleteButton(HWND hwnd)
{
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON5),false);
    return;
}

void inline unlockDeleteButton(HWND hwnd)
{
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON5),true);
    return;
}

void inline main_LockAllButtons(HWND hwnd)
{
    lockExitButton(hwnd);
    lockRefreshButton(hwnd);
    if(SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0)>=0)
    {
        lockOpenButton(hwnd);
        lockDeleteButton(hwnd);
    }
    return;
}

void inline main_UnlockAllButtons(HWND hwnd)
{
    unlockExitButton(hwnd);
    unlockRefreshButton(hwnd);
    if(SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0)>=0)
    {
        unlockOpenButton(hwnd);
        unlockDeleteButton(hwnd);
    }
    return;
}

void inline edit_LockAllButtons(HWND hwnd)
{
    main_LockAllButtons(g_hwnd);
    EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),false);
    EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON2),false);
    return;
}

void inline edit_UnlockAllButtons(HWND hwnd)
{
    main_UnlockAllButtons(g_hwnd);
    if(winMem[hwnd]->note->id!=0)
    {
        EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON2),true);
    }
    return;
}

void inline properties_LockAllButtons(HWND hwnd)
{
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON1),false);
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON2),false);
    edit_LockAllButtons(GetParent(hwnd));
    return;
}

void inline properties_UnlockAllButtons(HWND hwnd)
{
    edit_UnlockAllButtons(GetParent(hwnd));
    return;
}

unsigned int getState(HWND hwnd)
{
    WINDOWPLACEMENT wp = { 0 };
    wp.length=sizeof(WINDOWPLACEMENT);
    if(GetWindowPlacement(hwnd,&wp))
    {
        return wp.showCmd;
    }
    else
    {
        return 0;
    }
}

long int inline MakeDialogBox(HWND hwnd, unsigned int type, void* procedure)
{
    long int result;
    HANDLE instHandle=(HINSTANCE)GetWindowWord(hwnd,GWW_HINSTANCE);
    FARPROC proc=MakeProcInstance((FARPROC)procedure, instHandle);
    result=DialogBox(instHandle, MAKEINTRESOURCE(type), hwnd, (DLGPROC)proc);
    FreeProcInstance(proc);
    return result;
}

void makeEditWindowTitle(EDITWINDOW *editWin, NOTE *note, bool set)
{
    if(note==NULL)
    {
        editWin->windowTitle = "~ Nowa notatka ~ - ";
    }
    else
    {
        editWin->windowTitle = toCodePage(m_cp1250,(char*)note->subject.c_str())+" - ";
    }
    editWin->windowTitle = editWin->windowTitle + APPNAME;
    if(set)
    {
        SetWindowText(editWin->hwnd,(char*)editWin->windowTitle.c_str());
    }
    return;
}

//////////////////////////////////////
//
//  OKNO EDYCJI
//
//////////////////////////////////////

HWND createEditWindow(HWND hwnd, WINDOWMEMORY &winMem, NOTE *note)
{
    EDITWINDOW *editWin = new EDITWINDOW;
    HINSTANCE hInstance=(HINSTANCE)GetWindowWord(hwnd,GWW_HINSTANCE);

    makeEditWindowTitle(editWin,note,false);
    
    editWin->hwnd =CreateWindow(editWindowClass, editWin->windowTitle.c_str(), WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    if(editWin->hwnd==NULL)
    {
        delete editWin;
        return NULL;
    }
    else
    {
        if(note==NULL)
        {
            editWin->note=new NOTE;
            editWin->note->id=0;
            editWin->note->subject="";
            editWin->note->entry="";
            editWin->note->dateAdded="";
            editWin->note->lastModified="";
            editWin->note->locked=false;
            editWin->note->userAgent="";
            editWin->note->lastUserAgent="";
        }
        else
        {
            editWin->note=note;
        }
        
        editWin->hStatic = CreateWindow("STATIC", "Tytu³:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        0, 0, 600, 16, editWin->hwnd, (HMENU)ID_EDIT_STATIC1, hInstance, NULL);

        editWin->hEditBox= CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                                        0, 16, 600, 24, editWin->hwnd, (HMENU)ID_EDIT_EDITBOX1, hInstance, NULL);

        editWin->hStatic2= CreateWindow("STATIC", "Treœæ:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        0, 40, 600, 16, editWin->hwnd, (HMENU)ID_EDIT_STATIC2, hInstance, NULL);

        editWin->hEditBox2=CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
                                        0, 56, 600, 300, editWin->hwnd, (HMENU)ID_EDIT_EDITBOX2, hInstance, NULL);

        if(editWin->note->id==0)
        {
            editWin->hButton = CreateWindow("BUTTON", "Dodaj", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                                            0, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON1, hInstance, NULL);
        }
        else
        {
            editWin->hButton = CreateWindow("BUTTON", "Aktualizuj", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                                            0, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON1, hInstance, NULL);
        }

        if(editWin->note->id==0)
        {
            editWin->hButton2 =CreateWindow("BUTTON", "W³aœciwoœci", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                                            96, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON2, hInstance, NULL);
        }
        else
        {
            editWin->hButton2 =CreateWindow("BUTTON", "W³aœciwoœci", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                            96, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON2, hInstance, NULL);
        }

        editWin->hButton3 =CreateWindow("BUTTON", "Zamknij", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                        192, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON3, hInstance, NULL);

        editWin->hStatic3 =CreateWindow("STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_GRAYRECT,
                                        288, 356, 312, 21, editWin->hwnd, (HMENU)ID_EDIT_STATIC3, hInstance, NULL);

        editWin->hStatic4= CreateWindow("STATIC", "Wszystko w porz¹dku.", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        0, 377, 600, 16, editWin->hwnd, (HMENU)ID_EDIT_STATIC4, hInstance, NULL);

        if(editWin->note->id>0)
        {
            SetWindowText(editWin->hEditBox, toCodePage(m_cp1250,(char*)editWin->note->subject.c_str()).c_str());
            SetWindowText(editWin->hEditBox2,toCodePage(m_cp1250,(char*)editWin->note->entry.c_str()).c_str());
        }

        editWin->subjectChanged=false;
        editWin->entryChanged=false;
        editWin->lastResult=0;
        
        winMem[editWin->hwnd]=editWin;
        
        //ShowWindow(editWin->hwnd,SW_SHOW);
        ShowWindow(editWin->hwnd,getState(hwnd));
        UpdateWindow(editWin->hwnd);

        return editWin->hwnd;
    }
}

void deleteWindow(WINDOWMEMORY &winMem, HWND hwnd)
{
    delete winMem[hwnd]->note;
    delete winMem[hwnd];
    winMem.erase(hwnd);
    return;
}

//////////////////////////////////////
//
//  G£ÓWNA CZÊŒÆ PROGRAMU
//
//////////////////////////////////////

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    GetModuleFileName(hInstance,buffer,32767);
    std::string iniFile=getDefaultIniFile(buffer);
    connectionSettings=getConnectionSettings((char*)iniFile.c_str());
    credentials=getCredentials((char*)iniFile.c_str());
    prepareCodePage(m_cp1250,cp1250);

    if(wsInit() == SOCKET_ERROR)
    {
        MessageBox(NULL,"Inicjalizacja sk³adnika WinSock nie powiod³a siê. Program zostanie zamkniêty.","B³¹d",MB_ICONSTOP | MB_OK);
        return 1;
    }
    
    LPSTR mainWindowClass = "SimpleNoterMain";
    
    WNDCLASS wc = { 0 };
    WNDCLASS wc2= { 0 };

    HWND hwnd;
    HWND hButton, hButton2, hButton3, hButton4, hButton5;
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

    wc2.style = 0;
    wc2.lpfnWndProc = WndProc2;
    wc2.cbClsExtra = 0;
    wc2.cbWndExtra = 0;
    wc2.hInstance = hInstance;
    wc2.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
    wc2.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc2.hbrBackground = g_hBrush;
    wc2.lpszMenuName = MAKEINTRESOURCE(IDR_MENU2);
    wc2.lpszClassName = editWindowClass;

    if(!RegisterClass(&wc))
    {
        MessageBox(NULL,"Utworzenie klasy okna nie powiod³o siê. Program zostanie zamkniêty.","B³¹d",MB_ICONSTOP | MB_OK);
        return 1;
    }

    if(!RegisterClass(&wc2))
    {
        MessageBox(NULL,"Utworzenie klasy okna nie powiod³o siê. Program zostanie zamkniêty.","B³¹d",MB_ICONSTOP | MB_OK);
        return 1;
    }

    hwnd = CreateWindow(mainWindowClass, APPNAME, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        NULL, NULL, hInstance, NULL);
                        
    if(hwnd == NULL)
    {
        MessageBox(NULL,"Utworzenie okna nie powiod³o siê. Program zostanie zamkniêty.","B³¹d",MB_ICONSTOP | MB_OK);
        return 1;
    }

    g_hwnd=hwnd;

    hAccel=LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));
    if(!hAccel)
    {
        MessageBox(NULL,"Nie uda³o siê za³adowaæ akceleratorów! Program zostanie zamkniêty.","B³¹d",MB_ICONSTOP | MB_OK);
        return 1;
    }

    hButton =  CreateWindow("BUTTON", "Pobierz", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                            0, 0, 80, 21, hwnd, (HMENU)ID_BUTTON1, hInstance, NULL);

    hButton2 = CreateWindow("BUTTON", "Utwórz", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                            80, 0, 80, 21, hwnd, (HMENU)ID_BUTTON2, hInstance, NULL);

    hButton3 = CreateWindow("BUTTON", "Otwórz", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                            160, 0, 80, 21, hwnd, (HMENU)ID_BUTTON3, hInstance, NULL);

    hButton5 = CreateWindow("BUTTON", "Usuñ", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                            240, 0, 80, 21, hwnd, (HMENU)ID_BUTTON5, hInstance, NULL);

    hButton4 = CreateWindow("BUTTON", "Zakoñcz", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                            320, 0, 80, 21, hwnd, (HMENU)ID_BUTTON4, hInstance, NULL);

    hStatic6 = CreateWindow("STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_GRAYRECT,
                            400, 0, 200, 21, hwnd, (HMENU)ID_STATIC6, hInstance, NULL);

    hListBox = CreateWindow("LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
                            WS_VSCROLL | WS_TABSTOP | ES_AUTOVSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                            0, 21, 600, 300, hwnd, (HMENU)ID_LISTBOX, hInstance, NULL);
                           
    SetWindowPos(hListBox,NULL,0,21,600,300,SWP_NOZORDER);

    hStatic  = CreateWindow("STATIC", "ID:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                            8, 329, 128, 16, hwnd, (HMENU)ID_STATIC1, hInstance, NULL);
                           
    hStatic2 = CreateWindow("STATIC", "Ostatnie zmiany:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                            8, 346, 128, 16, hwnd, (HMENU)ID_STATIC2, hInstance, NULL);
                           
    hStatic3 = CreateWindow("STATIC", "-- nie wybrano --", WS_CHILD | WS_VISIBLE | SS_LEFT,
                            137, 329, 454, 16, hwnd, (HMENU)ID_STATIC3, hInstance, NULL);
                           
    hStatic4 = CreateWindow("STATIC", "-- nie wybrano --", WS_CHILD | WS_VISIBLE | SS_LEFT,
                            137, 346, 454, 16, hwnd, (HMENU)ID_STATIC4, hInstance, NULL);
                           
    hStatic5 = CreateWindow("STATIC", "Wszystko w porz¹dku.", WS_CHILD | WS_VISIBLE | SS_LEFT,
                            0, 370, 600, 16, hwnd, (HMENU)ID_STATIC5, hInstance, NULL);
    
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

    SendMessage(hwnd, WM_COMMAND, ID_BUTTON1, ID_FILE_RELOAD);

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

//////////////////////////////////////
//
//  G£ÓWNE OKNO
//
//////////////////////////////////////

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    std::string tempString="";
    long int index;
    long int result;
    unsigned long int width;
    unsigned long int height;
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
        case WM_INITMENU:
            index=SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0);
            if(index>=0)
            {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_OPEN,MF_ENABLED);
                EnableMenuItem(GetMenu(hwnd),ID_FILE_DELETE,MF_ENABLED);
            }
            else
            {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_OPEN,MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),ID_FILE_DELETE,MF_GRAYED);
            }
            if(IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON1)))
            {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_RELOAD,MF_ENABLED);
            }
            else
            {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_RELOAD,MF_GRAYED);
            }
            if(IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON3)))
            {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_OPEN,MF_ENABLED);
            }
            else
            {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_OPEN,MF_GRAYED);
            }
            if(IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON4)))
            {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_EXIT,MF_ENABLED);
            }
            else
            {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_EXIT,MF_GRAYED);
            }
            if(IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON5)))
            {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_DELETE,MF_ENABLED);
            }
            else
            {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_DELETE,MF_GRAYED);
            }
            break;
        case WM_COMMAND:
            switch(wParam)
            {
                case ID_ACC_TAB:
                    SetFocus(GetNextDlgTabItem(hwnd,GetFocus(),false));
                    break;
                case ID_ACC_CTRLN:
                    SendMessage(hwnd, WM_COMMAND, ID_BUTTON2, 0);
                    break;
                case ID_ACC_ENTER:
                    SendMessage(hwnd, WM_COMMAND, ID_BUTTON3, 0);
                    break;
                case ID_ACC_F5:
                    SendMessage(hwnd, WM_COMMAND, ID_BUTTON1, ID_ACC_F5);
                    break;
                case ID_ACC_F8:
                    SendMessage(hwnd, WM_COMMAND, ID_BUTTON5, 0);
                    break;
                case ID_ACC_ALTF4:
                    SendMessage(hwnd, WM_COMMAND, ID_BUTTON4, 0);
                    break;
                case ID_ACC_F1:
                    SendMessage(hwnd, WM_COMMAND, ID_HELP_HELP, 0);
                    break;
                case ID_FILE_NEW:
                    SendMessage(hwnd, WM_COMMAND, ID_BUTTON2, 0);
                    break;
                case ID_FILE_OPEN:
                    SendMessage(hwnd, WM_COMMAND, ID_BUTTON3, 0);
                    break;
                case ID_FILE_RELOAD:
                    SendMessage(hwnd, WM_COMMAND, ID_BUTTON1, ID_FILE_RELOAD);
                    break;
                case ID_FILE_DELETE:
                    SendMessage(hwnd, WM_COMMAND, ID_BUTTON5, 0);
                    break;
                case ID_FILE_EXIT:
                    SendMessage(hwnd, WM_COMMAND, ID_BUTTON4, 0);
                    break;
                case ID_OPTIONS_PREFERENCES:
                    MakeDialogBox(hwnd,IDD_DIALOG3,DlgProc3);
                    break;
                case ID_HELP_HELP:
                    WinHelp(g_hwnd,HELPFILE,HELP_CONTENTS,0);
                    break;
                case ID_HELP_HOWTO:
                    WinHelp(g_hwnd,"",HELP_HELPONHELP,0);
                    break;
                case ID_HELP_ABOUT:
                    MakeDialogBox(hwnd,IDD_DIALOG2,DlgProc2);
                    break;
                case ID_BUTTON1:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON1)))
                    {
                        break;
                    }
                    main_LockAllButtons(hwnd);
                    SetWindowText(GetDlgItem(hwnd,ID_STATIC5),"Pobieranie listy notatek...");
                    if(noteCount>0)
                    {
                        freeNoteList(notes);
                    }
                    noteCount=noter_getNoteList(connectionSettings,credentials,buffer,notes);
                    index=SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0);
                    SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_RESETCONTENT, 0, 0);
                    for(long int x=0; x<noteCount; ++x)
                    {
                        SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_ADDSTRING, 0, (LPARAM)toCodePage(m_cp1250,(char*)notes[x].subject.c_str()).c_str());
                    }
                    if(index>=0)
                    {
                        if(lParam==0)
                        {
                            SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_SETCURSEL, 0, 0);
                        }
                        else
                        {
                            SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_SETCURSEL, index, 0);
                        }
                    }
                    if(noteCount>=0)
                    {
                        mainLastResult=INFO_LIST_SUCCESSFUL;
                        if(lParam!=0)
                        {
                            tempString=noter_getAnswerString(mainLastResult)+" Iloœæ: "+IntToStr(noteCount)+".";
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)tempString.c_str());
                        }
                    }
                    else
                    {
                        mainLastResult=noteCount;
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                        EnableWindow(GetDlgItem(hwnd,ID_BUTTON3),false);
                        EnableWindow(GetDlgItem(hwnd,ID_BUTTON5),false);
                    }
                    main_UnlockAllButtons(hwnd);
                    break;
                case ID_BUTTON2:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON2)))
                    {
                        break;
                    }
                    if(mainLastResult!=0)
                    {
                        mainLastResult=0;
                        // SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                    }
                    SetWindowText(GetDlgItem(hwnd,ID_STATIC5),"Tworzenie okna edycji...");
                    if(createEditWindow(hwnd,winMem,NULL)==NULL)
                    {
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),"Nie uda³o siê utworzyæ okna edycji.");
                        mainLastResult=-2048;
                    }
                    else
                    {
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(INFO_OK).c_str());
                    }
                    break;
                case ID_BUTTON3:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON3)))
                    {
                        break;
                    }
                    main_LockAllButtons(hwnd);
                    if(mainLastResult!=0)
                    {
                        mainLastResult=0;
                        // SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                    }
                    SetWindowText(GetDlgItem(hwnd,ID_STATIC5),"Pobieranie notatki...");
                    index=SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0);
                    if(index>=0)
                    {
                        NOTE *note=new NOTE;
                        result=noter_getNote(connectionSettings,credentials,notes[index].id,buffer,*note);
                        if(result>=0)
                        {
                            HWND tempHwnd=createEditWindow(hwnd,winMem,note);
                            if(tempHwnd!=NULL)
                            {
                                tempString=noter_getAnswerString(result)+" Data ostatniej modyfikacji: "+toCodePage(m_cp1250,(char*)note->lastModified.c_str())+".";
                                SetWindowText(GetDlgItem(tempHwnd,ID_EDIT_STATIC4),(char*)tempString.c_str());
                                winMem[tempHwnd]->lastResult=result;
                                SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(INFO_OK).c_str());
                            }
                            else
                            {
                                SetWindowText(GetDlgItem(hwnd,ID_STATIC5),"Nie uda³o siê utworzyæ okna edycji.");
                                mainLastResult=-2048;
                            }
                        }
                        else
                        {
                            mainLastResult=result;
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                        }
                    }
                    main_UnlockAllButtons(hwnd);
                    break;
                case ID_BUTTON4:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON4)))
                    {
                        break;
                    }
                    if(mainLastResult!=0)
                    {
                        mainLastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                    }
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
                case ID_BUTTON5:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON5)))
                    {
                        break;
                    }
                    if(mainLastResult!=0)
                    {
                        mainLastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                    }
                    index=SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0);
                    tempString="Czy na pewno chcesz usun¹æ notatkê";
                    tempString=tempString+" \"";
                    tempString=tempString+toCodePage(m_cp1250,(char*)notes[index].subject.c_str());
                    tempString=tempString+"\"?";
                    if(MessageBox(hwnd,(char*)tempString.c_str(),APPNAME,MB_ICONQUESTION | MB_YESNO)==IDYES)
                    {
                        main_LockAllButtons(hwnd);
                        mainLastResult=noter_deleteNote(connectionSettings,credentials,notes[index].id,buffer);
                        main_UnlockAllButtons(hwnd);
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                        if(mainLastResult==INFO_NOTE_DELETED)
                        {
                            SendMessage(hwnd,WM_COMMAND,ID_BUTTON1,0);
                        }
                    }
                    break;
                case ID_LISTBOX:
                    switch(HIWORD(lParam))
                    {
                        case LBN_DBLCLK:
                            SendMessage(hwnd, WM_COMMAND, ID_BUTTON3, 0);
                            break;
                        case LBN_SELCHANGE:
                            index=SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0);
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC3),IntToStr(notes[index].id).c_str());
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC4),notes[index].lastModified.c_str());
                            EnableWindow(GetDlgItem(hwnd,ID_BUTTON3),true);
                            EnableWindow(GetDlgItem(hwnd,ID_BUTTON5),true);
                            if(mainLastResult!=0)
                            {
                                mainLastResult=0;
                                SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                            }
                            break;
                    }
                    break;
            }
            break;
        case WM_SIZE:
            width= LOWORD(lParam);
            height=HIWORD(lParam);

            if(width<240)
            {
                width=240;
            }
            if(height<240)
            {
                height=240;
            }

            SetWindowPos(GetDlgItem(hwnd,ID_LISTBOX),NULL,0,0,width,height-86,SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC6),NULL,0,0,width-400,21,SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC1),NULL,8,height-57,0,0,SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC2),NULL,8,height-40,0,0,SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC3),NULL,137,height-57,width-146,16,SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC4),NULL,137,height-40,width-146,16,SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC5),NULL,0,height-16,width,16,SWP_NOZORDER);

            break;
        case WM_CLOSE:
            if(IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON4)))
            {
                if(winMem.size()>0)
                {
                    for(WINDOWMEMORY::iterator it = winMem.begin(); it != winMem.end(); ++it)
                    {
                        SendMessage(it->second->hwnd, WM_CLOSE, 0, 0);
                    }
                }
                if(winMem.size()==0)
                {
                    DestroyWindow(hwnd);
                }
            }
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
            WinHelp(g_hwnd,"",HELP_QUIT,0);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd,msg,wParam,lParam);
    }
    return 0;
}

//////////////////////////////////////
//
//  OKNO EDYCJI
//
//////////////////////////////////////

LRESULT CALLBACK WndProc2(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    unsigned long int width;
    unsigned long int height;
    unsigned long int sel;
    unsigned int result;
    switch(msg)
    {
        case WM_CTLCOLOR:
            switch(HIWORD(lParam))
            {
                case CTLCOLOR_STATIC:
                    if((LOWORD(lParam)!=GetDlgItem(hwnd,ID_EDIT_STATIC3)) && (LOWORD(lParam)!=GetDlgItem(hwnd,ID_EDIT_STATIC4)))
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
        case WM_INITMENU:
            if(winMem[hwnd]->note->id==0)
            {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_PROPERTIES,MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),ID_FILE_TONEWNOTE,MF_GRAYED);
                ModifyMenu(GetMenu(hwnd),ID_FILE_ADDUP,MF_BYCOMMAND | MF_STRING,ID_FILE_ADDUP,"Dodaj\tCtrl+S");
                //if((winMem[hwnd]->subjectChanged) && (winMem[hwnd]->entryChanged))
                if(IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON1)))
                {
                    EnableMenuItem(GetMenu(hwnd),ID_FILE_ADDUP,MF_ENABLED);
                }
                else
                {
                    EnableMenuItem(GetMenu(hwnd),ID_FILE_ADDUP,MF_GRAYED);
                }
            }
            else
            {
                if(IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON2)))
                {
                    EnableMenuItem(GetMenu(hwnd),ID_FILE_PROPERTIES,MF_ENABLED);
                }
                else
                {
                    EnableMenuItem(GetMenu(hwnd),ID_FILE_PROPERTIES,MF_GRAYED);
                }
                EnableMenuItem(GetMenu(hwnd),ID_FILE_TONEWNOTE,MF_ENABLED);
                ModifyMenu(GetMenu(hwnd),ID_FILE_ADDUP,MF_BYCOMMAND | MF_STRING,ID_FILE_ADDUP,"Aktualizuj\tCtrl+S");
                //if((winMem[hwnd]->subjectChanged) || (winMem[hwnd]->entryChanged))
                if(IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON1)))
                {
                    EnableMenuItem(GetMenu(hwnd),ID_FILE_ADDUP,MF_ENABLED);
                }
                else
                {
                    EnableMenuItem(GetMenu(hwnd),ID_FILE_ADDUP,MF_GRAYED);
                }
            }
            if((GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX1)) || (GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2)))
            {
                if(SendMessage(GetFocus(), EM_CANUNDO, 0, 0))
                {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_UNDO,MF_ENABLED);
                }
                else
                {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_UNDO,MF_GRAYED);
                }
                sel=SendMessage(GetFocus(), EM_GETSEL, 0, 0);
                if(HIWORD(sel)!=LOWORD(sel))
                {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_CUT,MF_ENABLED);
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_COPY,MF_ENABLED);
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_CLEAR,MF_ENABLED);
                }
                else
                {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_CUT,MF_GRAYED);
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_COPY,MF_GRAYED);
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_CLEAR,MF_GRAYED);
                }
                if(OpenClipboard(GetFocus()))
                {
                    if(IsClipboardFormatAvailable(CF_TEXT) || IsClipboardFormatAvailable(CF_OEMTEXT))
                    {
                        EnableMenuItem(GetMenu(hwnd),ID_EDIT_PASTE,MF_ENABLED);
                    }
                    else
                    {
                        EnableMenuItem(GetMenu(hwnd),ID_EDIT_PASTE,MF_GRAYED);
                    }
                    CloseClipboard();
                }
                else
                {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_PASTE,MF_GRAYED);
                }
                if((GetWindowTextLength(GetFocus())>0) && (GetWindowTextLength(GetFocus())>(HIWORD(sel)-LOWORD(sel))))
                {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_SELECTALL,MF_ENABLED);
                }
                else
                {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_SELECTALL,MF_GRAYED);
                }
            }
            else
            {
                EnableMenuItem(GetMenu(hwnd),ID_EDIT_UNDO,MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),ID_EDIT_CUT,MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),ID_EDIT_COPY,MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),ID_EDIT_PASTE,MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),ID_EDIT_CLEAR,MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),ID_EDIT_SELECTALL,MF_GRAYED);
            }
            break;
        case WM_COMMAND:
            switch(wParam)
            {
                case ID_ACC_ENTER:
                    SendMessage(GetFocus(), WM_CHAR, VK_RETURN, 0);
                    break;
                case ID_ACC_TAB:
                    if(GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2))
                    {
                        SendMessage(GetFocus(), WM_CHAR, VK_TAB, 0);
                    }
                    else
                    {
                        SetFocus(GetNextDlgTabItem(hwnd,GetFocus(),false));
                    }
                    break;
                case ID_ACC_CTRLS:
                    SendMessage(hwnd, WM_COMMAND, ID_FILE_ADDUP, 0);
                    break;
                case ID_ACC_CTRLALTN:
                    if(winMem[hwnd]->note->id!=0)
                    {
                        SendMessage(hwnd, WM_COMMAND, ID_FILE_TONEWNOTE, 0);
                    }
                    break;
                case ID_ACC_CTRLA:
                    SendMessage(hwnd, WM_COMMAND, ID_EDIT_SELECTALL, 0);
                    break;
                case ID_ACC_ALTF4:
                    SendMessage(hwnd, WM_COMMAND, ID_EDIT_BUTTON3, 0);
                    break;
                case ID_ACC_F1:
                    SendMessage(hwnd, WM_COMMAND, ID_HELP_HELP, 0);
                    break;
                case ID_FILE_ADDUP:
                    if(IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON1)))
                    {
                        SendMessage(hwnd, WM_COMMAND, ID_EDIT_BUTTON1, 0);
                    }
                    break;
                case ID_FILE_PROPERTIES:
                    if(IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON2)))
                    {
                        SendMessage(hwnd, WM_COMMAND, ID_EDIT_BUTTON2, 0);
                    }
                    break;
                case ID_FILE_TONEWNOTE:
                    if(winMem[hwnd]->lastResult!=0)
                    {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    winMem[hwnd]->note->id=0;
                    EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),true);
                    EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON2),false);
                    SetWindowText(GetDlgItem(hwnd,ID_EDIT_BUTTON1),"Dodaj");
                    winMem[hwnd]->subjectChanged=true;
                    winMem[hwnd]->entryChanged=true;
                    makeEditWindowTitle(winMem[hwnd],NULL,true);
                    SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),"Przekierowano na now¹ notatkê.");
                    winMem[hwnd]->lastResult=1024;
                    break;
                case ID_FILE_EXIT:
                    SendMessage(hwnd, WM_COMMAND, ID_EDIT_BUTTON3, 0);
                    break;
                case ID_EDIT_UNDO:
                    if((GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX1)) || (GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2)))
                    {
                        if(winMem[hwnd]->lastResult!=0)
                        {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),WM_UNDO,0,0);
                    }
                    break;
                case ID_EDIT_CUT:
                    if((GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX1)) || (GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2)))
                    {
                        if(winMem[hwnd]->lastResult!=0)
                        {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),WM_CUT,0,0);
                    }
                    break;
                case ID_EDIT_COPY:
                    if((GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX1)) || (GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2)))
                    {
                        if(winMem[hwnd]->lastResult!=0)
                        {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),WM_COPY,0,0);
                    }
                    break;
                case ID_EDIT_PASTE:
                    if((GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX1)) || (GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2)))
                    {
                        if(winMem[hwnd]->lastResult!=0)
                        {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),WM_PASTE,0,0);
                    }
                    break;
                case ID_EDIT_CLEAR:
                    if((GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX1)) || (GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2)))
                    {
                        if(winMem[hwnd]->lastResult!=0)
                        {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),WM_CLEAR,0,0);
                    }
                    break;
                case ID_EDIT_SELECTALL:
                    if((GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX1)) || (GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2)))
                    {
                        if(winMem[hwnd]->lastResult!=0)
                        {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),EM_SETSEL,0,65535);
                    }
                    break;
                case ID_HELP_HELP:
                    if(winMem[hwnd]->lastResult!=0)
                    {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    WinHelp(g_hwnd,HELPFILE,HELP_CONTENTS,0);
                    break;
                case ID_HELP_HOWTO:
                    if(winMem[hwnd]->lastResult!=0)
                    {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    WinHelp(g_hwnd,"",HELP_HELPONHELP,0);
                    break;
                case ID_HELP_ABOUT:
                    if(winMem[hwnd]->lastResult!=0)
                    {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    MakeDialogBox(hwnd,IDD_DIALOG2,DlgProc2);
                    break;
                case ID_EDIT_EDITBOX1:
                    switch(HIWORD(lParam))
                    {
                        case EN_CHANGE:
                            if(winMem[hwnd]!=NULL)
                            {
                                if(!winMem[hwnd]->subjectChanged)
                                {
                                    winMem[hwnd]->subjectChanged=true;
                                    if(winMem[hwnd]->note->id==0)
                                    {
                                        if(winMem[hwnd]->entryChanged)
                                        {
                                            EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),true);
                                        }
                                    }
                                    else
                                    {
                                        EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),true);
                                    }
                                }
                                if(winMem[hwnd]->lastResult!=0)
                                {
                                    winMem[hwnd]->lastResult=0;
                                    SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                                }
                            }
                            break;
                    }
                    break;
                case ID_EDIT_EDITBOX2:
                    switch(HIWORD(lParam))
                    {
                        case EN_CHANGE:
                            if(winMem[hwnd]!=NULL)
                            {
                                if(!winMem[hwnd]->entryChanged)
                                {
                                    winMem[hwnd]->entryChanged=true;
                                    if(winMem[hwnd]->note->id==0)
                                    {
                                        if(winMem[hwnd]->subjectChanged)
                                        {
                                            EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),true);
                                        }
                                    }
                                    else
                                    {
                                        EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),true);
                                    }
                                }
                                if(winMem[hwnd]->lastResult!=0)
                                {
                                    winMem[hwnd]->lastResult=0;
                                    SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                                }
                            }
                            break;
                    }
                    break;
                case ID_EDIT_BUTTON1:
                    if((!IsWindowEnabled(GetDlgItem(g_hwnd,ID_BUTTON4))) || (!IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON1))))
                    {
                        break;
                    }
                    if(winMem[hwnd]->lastResult!=0)
                    {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    GetWindowText(GetDlgItem(hwnd,ID_EDIT_EDITBOX1),buffer,65535);
                    winMem[hwnd]->note->subject=fromCodePage(cp1250,buffer);
                    GetWindowText(GetDlgItem(hwnd,ID_EDIT_EDITBOX2),buffer,65535);
                    winMem[hwnd]->note->entry=fromCodePage(cp1250,buffer);
                    if(winMem[hwnd]->note->id==0)
                    {
                        edit_LockAllButtons(hwnd);
                        winMem[hwnd]->lastResult=noter_addNote(connectionSettings,credentials,*winMem[hwnd]->note,buffer);
                        edit_UnlockAllButtons(hwnd);
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        if(winMem[hwnd]->lastResult>=0)
                        {
                            SendMessage(g_hwnd,WM_COMMAND,ID_BUTTON1,0);
                            //EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),false);
                            //EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON2),true);
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_BUTTON1),"Aktualizuj");
                            winMem[hwnd]->subjectChanged=false;
                            winMem[hwnd]->entryChanged=false;
                            makeEditWindowTitle(winMem[hwnd],winMem[hwnd]->note,true);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),true);
                        }
                    }
                    else
                    {
                        edit_LockAllButtons(hwnd);
                        winMem[hwnd]->lastResult=noter_updateNote(connectionSettings,credentials,*winMem[hwnd]->note,buffer);
                        edit_UnlockAllButtons(hwnd);
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        if(winMem[hwnd]->lastResult>=0)
                        {
                            SendMessage(g_hwnd,WM_COMMAND,ID_BUTTON1,0);
                            //EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),false);
                            winMem[hwnd]->subjectChanged=false;
                            winMem[hwnd]->entryChanged=false;
                            makeEditWindowTitle(winMem[hwnd],winMem[hwnd]->note,true);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),true);
                        }
                    }
                    break;
                case ID_EDIT_BUTTON2:
                    if((!IsWindowEnabled(GetDlgItem(g_hwnd,ID_BUTTON4))) || (!IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON2))))
                    {
                        break;
                    }
                    if(winMem[hwnd]->lastResult!=0)
                    {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    MakeDialogBox(hwnd,IDD_DIALOG1,DlgProc);
                    break;
                case ID_EDIT_BUTTON3:
                    if((!IsWindowEnabled(GetDlgItem(g_hwnd,ID_BUTTON4))) || (!IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON3))))
                    {
                        break;
                    }
                    if(winMem[hwnd]->lastResult!=0)
                    {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
            }   
            break;
        case WM_SIZE:
            width= LOWORD(lParam);
            height=HIWORD(lParam);

            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_STATIC1),NULL,0,0,width,16,SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_EDITBOX1),NULL,0,0,width,24,SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_STATIC2),NULL,0,0,width,16,SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_EDITBOX2),NULL,0,0,width,height-93,SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_BUTTON1),NULL,0,height-37,0,0,SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_BUTTON2),NULL,96,height-37,0,0,SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_BUTTON3),NULL,192,height-37,0,0,SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_STATIC3),NULL,288,height-37,width-288,21,SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_STATIC4),NULL,0,height-16,width,16,SWP_NOZORDER);
            
            break;
        case WM_CLOSE:
            if((!IsWindowEnabled(GetDlgItem(g_hwnd,ID_BUTTON4))) || (!IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON3))))
            {
                break;
            }
            if((winMem[hwnd]!=NULL) && (winMem[hwnd]->subjectChanged || winMem[hwnd]->entryChanged))
            {
                //GetWindowText(hwnd,buffer,32767);
                result=MessageBox(hwnd,"Czy chcesz zapisaæ zmiany?",APPNAME,MB_ICONEXCLAMATION | MB_YESNOCANCEL);
            }
            else
            {
                result=IDNO;
            }
            if(result==IDYES)
            {
                SendMessage(hwnd,WM_COMMAND,ID_EDIT_BUTTON1,0);
            }
            else if(result==IDNO)
            {
                winMem[hwnd]->lastResult=0;
            }
            if((result!=IDCANCEL) && (winMem[hwnd]->lastResult>=0))
            {
                DestroyWindow(hwnd);
            }
            break;
        case WM_DESTROY:
            deleteWindow(winMem,hwnd);
            break;
        default:
            return DefWindowProc(hwnd,msg,wParam,lParam);
    }
    return 0;
}

//////////////////////////////////////
//
//  DIALOG W£AŒCIWOŒCI NOTATKI
//
//////////////////////////////////////

BOOL CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    NOTE tempNote;
    long int result;
    switch(msg)
    {
        case WM_INITDIALOG:
            properties_LockAllButtons(hwnd);
            result=noter_getNote(connectionSettings,credentials,winMem[GetParent(hwnd)]->note->id,buffer,tempNote);
            properties_UnlockAllButtons(hwnd);
            if(result>=0)
            {
                SetWindowText(GetDlgItem(hwnd,IDC_STATIC1),(char*)toCodePage(m_cp1250,(char*)tempNote.subject.c_str()).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_STATIC2),(char*)IntToStr(tempNote.id).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_STATIC3),(char*)toCodePage(m_cp1250,(char*)tempNote.dateAdded.c_str()).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_STATIC4),(char*)toCodePage(m_cp1250,(char*)tempNote.userAgent.c_str()).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_STATIC5),(char*)toCodePage(m_cp1250,(char*)tempNote.lastModified.c_str()).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_STATIC6),(char*)toCodePage(m_cp1250,(char*)tempNote.lastUserAgent.c_str()).c_str());
                if(tempNote.locked)
                {
                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON1),false);
                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON2),true);
                }
                else
                {
                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON1),true);
                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON2),false);
                }
            }
            else
            {
                //GetWindowText(GetParent(hwnd),buffer,32767);
                MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),APPNAME,MB_ICONHAND | MB_OK);
                EndDialog(hwnd,IDOK);
            }
            break;
            /*
        case WM_CTLCOLOR:
            switch(HIWORD(lParam))
            {
                case CTLCOLOR_BTN:
                    SetBkMode((HDC)wParam, TRANSPARENT);
                    break;
            }
            break;
            */
        case WM_COMMAND:
            switch(wParam)
            {
                case IDOK:
                    EndDialog(hwnd,IDOK);
                    break;
                case IDC_BUTTON1:
                    result=noter_lockNote(connectionSettings,credentials,winMem[GetParent(hwnd)]->note->id,buffer);
                    if(result>=0)
                    {
                        winMem[GetParent(hwnd)]->note->locked=true;
                        tempNote.locked=true;
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON1),false);
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON2),true);
                    }
                    else
                    {
                        //GetWindowText(GetParent(hwnd),buffer,32767);
                        MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),APPNAME,MB_ICONHAND | MB_OK);
                    }
                    break;
                case IDC_BUTTON2:
                    result=noter_unlockNote(connectionSettings,credentials,winMem[GetParent(hwnd)]->note->id,buffer);
                    if(result>=0)
                    {
                        winMem[GetParent(hwnd)]->note->locked=false;
                        tempNote.locked=false;
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON1),true);
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON2),false);
                    }
                    else
                    {
                        //GetWindowText(GetParent(hwnd),buffer,32767);
                        MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),APPNAME,MB_ICONHAND | MB_OK);
                    }
                    break;
            }
            break;
        case WM_CLOSE:
            EndDialog(hwnd,IDOK);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

//////////////////////////////////////
//
//  DIALOG INFORMACJI O PROGRAMIE
//
//////////////////////////////////////

BOOL CALLBACK DlgProc2(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    NOTE tempNote;
    switch(msg)
    {
        case WM_INITDIALOG:
            /*
            if(Ctl3dEnabled())
            {
                unsigned int ctlRegs=CTL3D_ALL;
                Ctl3dSubclassDlg(hwnd,ctlRegs);
            }
            */
            SetWindowText(GetDlgItem(hwnd,IDC_STATIC7),APPNAME);
            break;
            /*
        case WM_CTLCOLOR:
            switch(HIWORD(lParam))
            {
                case CTLCOLOR_BTN:
                    SetBkMode((HDC)wParam, TRANSPARENT);
                    break;
            }
            break;
            */
        case WM_COMMAND:
            switch(wParam)
            {
                case IDOK:
                    EndDialog(hwnd,IDOK);
                    break;
            }
        case WM_CLOSE:
            EndDialog(hwnd,IDOK);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

//////////////////////////////////////
//
//  DIALOG PREFERENCJI
//
//////////////////////////////////////

BOOL CALLBACK DlgProc3(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    bool enabled, option1, option2;
    switch(msg)
    {
        case WM_INITDIALOG:
            SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)"Normalne okno");
            SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)"Zminimalizowane");
            SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)"Zmaksymalizowane");
            SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)"Normalne okno");
            SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)"Zminimalizowane");
            SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)"Zmaksymalizowane");
            SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_SETCURSEL, 0, 0);
            SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_SETCURSEL, 0, 0);
            SendMessage(GetDlgItem(hwnd, IDC_COMBO1), WM_PAINT, 0, 0);
            SendMessage(GetDlgItem(hwnd, IDC_COMBO2), WM_PAINT, 0, 0);
            option1=true;
            option2=true;
            if(option1)
            {
                CheckRadioButton(hwnd, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
                EnableWindow(GetDlgItem(hwnd,IDC_COMBO1),false);
            }
            else
            {
                CheckRadioButton(hwnd, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
                EnableWindow(GetDlgItem(hwnd,IDC_COMBO1),true);
            }
            if(option2)
            {
                CheckRadioButton(hwnd, IDC_RADIO3, IDC_RADIO4, IDC_RADIO3);
                EnableWindow(GetDlgItem(hwnd,IDC_COMBO2),false);
            }
            else
            {
                CheckRadioButton(hwnd, IDC_RADIO3, IDC_RADIO4, IDC_RADIO4);
                EnableWindow(GetDlgItem(hwnd,IDC_COMBO2),true);
            }
            enabled=IsDlgButtonChecked(hwnd, IDC_CHECK3);
            EnableWindow(GetDlgItem(hwnd,IDC_CHECK4),enabled);
            EnableWindow(GetDlgItem(hwnd,IDC_CHECK5),enabled);
            EnableWindow(GetDlgItem(hwnd,IDC_CHECK6),enabled);
            EnableWindow(GetDlgItem(hwnd,IDC_CHECK7),enabled);
            EnableWindow(GetDlgItem(hwnd,IDC_CHECK8),enabled);
            break;
            /*
        case WM_CTLCOLOR:
            switch(HIWORD(lParam))
            {
                case CTLCOLOR_BTN:
                    SetBkMode((HDC)wParam, TRANSPARENT);
                    break;
            }
            break;
            */
        case WM_COMMAND:
            switch(wParam)
            {
                case IDOK:
                    EndDialog(hwnd,IDOK);
                    break;
                case IDCANCEL:
                    EndDialog(hwnd,IDCANCEL);
                    break;
                case IDC_RADIO1:
                    CheckRadioButton(hwnd, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
                    EnableWindow(GetDlgItem(hwnd,IDC_COMBO1),false);
                    break;
                case IDC_RADIO2:
                    CheckRadioButton(hwnd, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
                    EnableWindow(GetDlgItem(hwnd,IDC_COMBO1),true);
                    break;
                case IDC_RADIO3:
                    CheckRadioButton(hwnd, IDC_RADIO3, IDC_RADIO4, IDC_RADIO3);
                    EnableWindow(GetDlgItem(hwnd,IDC_COMBO2),false);
                    break;
                case IDC_RADIO4:
                    CheckRadioButton(hwnd, IDC_RADIO3, IDC_RADIO4, IDC_RADIO4);
                    EnableWindow(GetDlgItem(hwnd,IDC_COMBO2),true);
                    break;
                case IDC_CHECK3:
                    enabled=IsDlgButtonChecked(hwnd, IDC_CHECK3);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK4),enabled);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK5),enabled);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK6),enabled);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK7),enabled);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK8),enabled);
                    break;
            }
            break;
        case WM_CLOSE:
            EndDialog(hwnd,IDOK);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}
