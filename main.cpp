#include <windows.h>
#include <ctl3d.h>
#include <string>
#include <map>

#include "resources.h"

// Uncomment for debug purposes (showing integers)
#include "debug.hpp"

#include "helpers.hpp"
#include "wsprocs.hpp"
#include "noterapi.hpp"
#include "codepages.hpp"
#include "constants.hpp"
#include "responses.hpp"
#include "conversion.hpp"
#include "definitions.hpp"
#include "inihandling.hpp"

//////////////////////////////////////
//
//  GLOBAL VARIABLES
//
//////////////////////////////////////

LPSTR editWindowClass = "SimpleNoterEdit";

HBRUSH g_hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
WINDOWMEMORY winMem;
HWND g_hwnd;
char buffer[65536];
RAWCODEPAGE rawCodePage;
CODEPAGE mappedCodePage;
NOTER_CONNECTION_SETTINGS connectionSettings;
NOTER_CREDENTIALS credentials, tempCredentials, *auxCredentials;
MAINSETTINGS mainSettings;
NOTE_SUMMARY *notes=NULL;
long int noteCount=0;
long int mainLastResult=0;
unsigned int ctlRegs=0;
bool check3DChanged, editsChanged, editsChanged2, useTestCredentials, firstOptions=false;
HINSTANCE hCodePageLib=NULL;
HGLOBAL hCodePageDefinition=NULL;

//////////////////////////////////////
//
//  PROTOTYPES
//
//////////////////////////////////////

void inline edit_LockAllButtons(HWND);
void inline edit_UnlockAllButtons(HWND);
void inline properties_LockAllButtons(HWND);
void inline properties_UnlockAllButtons(HWND);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc2(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc2(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc3(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc4(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc5(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc6(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc7(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc8(HWND, UINT, WPARAM, LPARAM);

//////////////////////////////////////
//
//  ADDITIONAL PROCEDURES
//
//////////////////////////////////////

void inline main_LockAllButtons(HWND hwnd) {
    lockExitButton(hwnd);
    lockRefreshButton(hwnd);
    if(SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0)>=0) {
        lockOpenButton(hwnd);
        lockDeleteButton(hwnd);
    }
    return;
}

void inline main_UnlockAllButtons(HWND hwnd) {
    unlockExitButton(hwnd);
    if(noter_connectionSettingsAvailable(connectionSettings) && noter_credentialsAvailable(credentials)) {
        unlockRefreshButton(hwnd);
        if(SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0)>=0) {
            unlockOpenButton(hwnd);
            unlockDeleteButton(hwnd);
        }
    }
    return;
}

void inline edit_LockAllButtons(HWND hwnd) {
    main_LockAllButtons(g_hwnd);
    EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),false);
    EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON2),false);
    return;
}

void inline edit_UnlockAllButtons(HWND hwnd) {
    main_UnlockAllButtons(g_hwnd);
    if(winMem[hwnd]->note->id!=0) {
        EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON2),true);
    }
    return;
}

void inline properties_LockAllButtons(HWND hwnd) {
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON1),false);
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON2),false);
    edit_LockAllButtons(GetParent(hwnd));
    return;
}

void inline properties_UnlockAllButtons(HWND hwnd) {
    edit_UnlockAllButtons(GetParent(hwnd));
    return;
}

void inline connection_LockAllButtons(HWND hwnd) {
    main_LockAllButtons(GetParent(hwnd));
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON3),false);
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON4),false);
    EnableWindow(GetDlgItem(hwnd,IDOK),false);
    EnableWindow(GetDlgItem(hwnd,IDCANCEL),false);
    return;
}

void inline connection_UnlockAllButtons(HWND hwnd) {
    main_UnlockAllButtons(GetParent(hwnd));
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON3),true);
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON4),true);
    EnableWindow(GetDlgItem(hwnd,IDOK),true);
    EnableWindow(GetDlgItem(hwnd,IDCANCEL),true);
    return;
}

void inline credentials_LockAllButtons(HWND hwnd) {
    main_LockAllButtons(GetParent(hwnd));
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON5),false);
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON6),false);
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON7),false);
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON8),false);
    EnableWindow(GetDlgItem(hwnd,IDOK),false);
    EnableWindow(GetDlgItem(hwnd,IDCANCEL),false);
    return;
}

void inline credentials_UnlockAllButtons(HWND hwnd) {
    main_UnlockAllButtons(GetParent(hwnd));
    if(noter_connectionSettingsAvailable(connectionSettings)) {
        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON5),true);
    }
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON6),true);
    EnableWindow(GetDlgItem(hwnd,IDOK),true);
    EnableWindow(GetDlgItem(hwnd,IDCANCEL),true);
    return;
}

void inline userEdit_LockAllButtons(HWND hwnd) {
    main_LockAllButtons(GetParent(GetParent(hwnd)));
    EnableWindow(GetDlgItem(hwnd,IDOK),false);
    EnableWindow(GetDlgItem(hwnd,IDCANCEL),false);
    return;
}

void inline userEdit_UnlockAllButtons(HWND hwnd) {
    main_UnlockAllButtons(GetParent(GetParent(hwnd)));
    EnableWindow(GetDlgItem(hwnd,IDOK),true);
    EnableWindow(GetDlgItem(hwnd,IDCANCEL),true);
    return;
}

//////////////////////////////////////
//
//  EDIT WINDOW
//
//////////////////////////////////////

HWND createEditWindow(HWND hwnd, WINDOWMEMORY &winMem, NOTE *note) {
    EDITWINDOW *editWin = new EDITWINDOW;
    HINSTANCE hInstance=(HINSTANCE)GetWindowWord(hwnd,GWW_HINSTANCE);

    makeEditWindowTitle(editWin,note,false,mappedCodePage);
    
    editWin->hwnd =CreateWindow(editWindowClass, editWin->windowTitle.c_str(), WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    if(editWin->hwnd==NULL) {
        delete editWin;
        return NULL;
    }
    else {
        if(mainSettings.savePosSizes) {
            if((mainSettings.editWindowX!=CW_USEDEFAULT) && (mainSettings.editWindowY!=CW_USEDEFAULT) && (mainSettings.editWindowSizeX!=CW_USEDEFAULT) && (mainSettings.editWindowSizeY!=CW_USEDEFAULT)) {
                SetWindowPos(editWin->hwnd,NULL,mainSettings.editWindowX,mainSettings.editWindowY,mainSettings.editWindowSizeX,mainSettings.editWindowSizeY,SWP_NOZORDER);
            }
        }
        if(note==NULL) {
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
        else {
            editWin->note=note;
        }
        
        editWin->hStatic = CreateWindow("STATIC", STRING_EDITWIN_TITLE, WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        0, 0, 600, 16, editWin->hwnd, (HMENU)ID_EDIT_STATIC1, hInstance, NULL);

        editWin->hEditBox= CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                                        0, 16, 600, 24, editWin->hwnd, (HMENU)ID_EDIT_EDITBOX1, hInstance, NULL);

        editWin->hStatic2= CreateWindow("STATIC", STRING_EDITWIN_ENTRY, WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        0, 40, 600, 16, editWin->hwnd, (HMENU)ID_EDIT_STATIC2, hInstance, NULL);

        editWin->hEditBox2=CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
                                        0, 56, 600, 300, editWin->hwnd, (HMENU)ID_EDIT_EDITBOX2, hInstance, NULL);

        if(editWin->note->id==0) {
            editWin->hButton = CreateWindow("BUTTON", STRING_ADD, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                                            0, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON1, hInstance, NULL);
        }
        else {
            editWin->hButton = CreateWindow("BUTTON", STRING_UPDATE, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                                            0, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON1, hInstance, NULL);
        }

        if(editWin->note->id==0) {
            editWin->hButton2 =CreateWindow("BUTTON", STRING_PROPERTIES, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                                            96, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON2, hInstance, NULL);
        }
        else {
            editWin->hButton2 =CreateWindow("BUTTON", STRING_PROPERTIES, WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                            96, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON2, hInstance, NULL);
        }

        editWin->hButton3 =CreateWindow("BUTTON", STRING_CLOSE, WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                        192, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON3, hInstance, NULL);

        editWin->hStatic3 =CreateWindow("STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_GRAYRECT,
                                        288, 356, 312, 21, editWin->hwnd, (HMENU)ID_EDIT_STATIC3, hInstance, NULL);

        editWin->hStatic4= CreateWindow("STATIC", STRING_INFO_OK, WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        0, 377, 600, 16, editWin->hwnd, (HMENU)ID_EDIT_STATIC4, hInstance, NULL);

        bool warningState=false;
        if(editWin->note->id>0) {
            SetWindowText(editWin->hEditBox, toCodePage(mappedCodePage,(char*)editWin->note->subject.c_str()).c_str());
            warningState=decodeWarningState();
            SetWindowText(editWin->hEditBox2,toCodePage(mappedCodePage,(char*)editWin->note->entry.c_str()).c_str());
            warningState=(warningState || decodeWarningState());
        }

        editWin->subjectChanged=false;
        editWin->entryChanged=false;
        editWin->lastResult=0;
        
        winMem[editWin->hwnd]=editWin;

        if(mainSettings.use3DControls && Ctl3dEnabled()) {
            Ctl3dSubclassDlg(editWin->hwnd,ctlRegs);
        }

        if(mainSettings.savePosSizes) {
            if((mainSettings.editWindowStyle+1)==SW_SHOWMINIMIZED) {
                ShowWindow(editWin->hwnd,SW_SHOWNORMAL);
            }
            else {
                ShowWindow(editWin->hwnd,(mainSettings.editWindowStyle+1));
            }
        }
        else {
            if(mainSettings.editWindowSystem) {
                ShowWindow(editWin->hwnd,getState(hwnd));
            }
            else {
                ShowWindow(editWin->hwnd,(mainSettings.editWindowStyle+1));
            }
        }
        UpdateWindow(editWin->hwnd);

        if(warningState) {
            MessageBox(editWin->hwnd,STRING_MSG_UNSUPPORTED_CHARS,APPNAME,MB_ICONINFORMATION);
        }

        return editWin->hwnd;
    }
}

//////////////////////////////////////
//
//  MAIN PROGRAM
//
//////////////////////////////////////

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    LPSTR mainWindowClass = "SimpleNoterMain";
    
    WNDCLASS wc = { 0 };
    WNDCLASS wc2= { 0 };

    HWND hwnd;
    HWND hButton, hButton2, hButton3, hButton4, hButton5;
    HWND hListBox;
    HWND hStatic, hStatic2, hStatic3, hStatic4, hStatic5, hStatic6;
    HWND temp;

    HACCEL hAccel;
    MSG msg;
    
    GetModuleFileName(hInstance,buffer,32767);
    std::string iniFile=getDefaultIniFile(buffer);
    connectionSettings=getConnectionSettings((char*)iniFile.c_str());
    credentials=getCredentials((char*)iniFile.c_str());
    mainSettings=getMainSettings((char*)iniFile.c_str());

    // temporary setting
    if(!loadCodePage("cp1250.dll",hCodePageLib,hCodePageDefinition,rawCodePage)) {
        MessageBox(NULL,STRING_MSG_CODEPAGE_ERROR,STRING_ERROR,MB_ICONSTOP | MB_OK);
        return 1;
    }
    
    prepareCodePage(mappedCodePage,rawCodePage);

    if(wsInit() == SOCKET_ERROR) {
        MessageBox(NULL,STRING_MSG_WINSOCK_ERROR,STRING_ERROR,MB_ICONSTOP | MB_OK);
        return 1;
    }
        
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

    if(!RegisterClass(&wc)) {
        MessageBox(NULL,STRING_MSG_WNDCLASS_ERROR,STRING_ERROR,MB_ICONSTOP | MB_OK);
        return 1;
    }

    if(!RegisterClass(&wc2)) {
        MessageBox(NULL,STRING_MSG_WNDCLASS_ERROR,STRING_ERROR,MB_ICONSTOP | MB_OK);
        return 1;
    }

    hwnd = CreateWindow(mainWindowClass, APPNAME, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        NULL, NULL, hInstance, NULL);

    if(hwnd == NULL) {
        MessageBox(NULL,STRING_MSG_WND_CREATE_ERROR,STRING_ERROR,MB_ICONSTOP | MB_OK);
        return 1;
    }

    if(mainSettings.savePosSizes) {
        if((mainSettings.mainWindowX!=CW_USEDEFAULT) && (mainSettings.mainWindowY!=CW_USEDEFAULT) && (mainSettings.mainWindowSizeX!=CW_USEDEFAULT) && (mainSettings.mainWindowSizeY!=CW_USEDEFAULT)) {
            SetWindowPos(hwnd,NULL,mainSettings.mainWindowX,mainSettings.mainWindowY,mainSettings.mainWindowSizeX,mainSettings.mainWindowSizeY,SWP_NOZORDER);
        }
    }

    g_hwnd=hwnd;

    hAccel=LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));
    if(!hAccel) {
        MessageBox(NULL,STRING_MSG_ACCELERATORS_ERROR,STRING_ERROR,MB_ICONSTOP | MB_OK);
        return 1;
    }

    hButton =  CreateWindow("BUTTON", STRING_DOWNLOAD, WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                            0, 0, 80, 21, hwnd, (HMENU)ID_BUTTON1, hInstance, NULL);

    hButton2 = CreateWindow("BUTTON", STRING_CREATE, WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                            80, 0, 80, 21, hwnd, (HMENU)ID_BUTTON2, hInstance, NULL);

    hButton3 = CreateWindow("BUTTON", STRING_OPEN, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                            160, 0, 80, 21, hwnd, (HMENU)ID_BUTTON3, hInstance, NULL);

    hButton5 = CreateWindow("BUTTON", STRING_DELETE, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                            240, 0, 80, 21, hwnd, (HMENU)ID_BUTTON5, hInstance, NULL);

    hButton4 = CreateWindow("BUTTON", STRING_EXIT, WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                            320, 0, 80, 21, hwnd, (HMENU)ID_BUTTON4, hInstance, NULL);

    hStatic6 = CreateWindow("STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_GRAYRECT,
                            400, 0, 200, 21, hwnd, (HMENU)ID_STATIC6, hInstance, NULL);

    hListBox = CreateWindow("LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
                            WS_VSCROLL | WS_TABSTOP | ES_AUTOVSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                            0, 21, 600, 300, hwnd, (HMENU)ID_LISTBOX, hInstance, NULL);

    if(mainSettings.use3DLists) {
        SetWindowPos(hListBox,NULL,0,22,600,298,SWP_NOZORDER);
    }
    else {
        SetWindowPos(hListBox,NULL,0,21,600,300,SWP_NOZORDER);
    }

    hStatic  = CreateWindow("STATIC", STRING_ID, WS_CHILD | WS_VISIBLE | SS_LEFT,
                            8, 329, 128, 16, hwnd, (HMENU)ID_STATIC1, hInstance, NULL);
                           
    hStatic2 = CreateWindow("STATIC", STRING_LAST_CHANGED, WS_CHILD | WS_VISIBLE | SS_LEFT,
                            8, 346, 128, 16, hwnd, (HMENU)ID_STATIC2, hInstance, NULL);
                           
    hStatic3 = CreateWindow("STATIC", STRING_NOT_CHOSEN, WS_CHILD | WS_VISIBLE | SS_LEFT,
                            137, 329, 454, 16, hwnd, (HMENU)ID_STATIC3, hInstance, NULL);
                           
    hStatic4 = CreateWindow("STATIC", STRING_NOT_CHOSEN, WS_CHILD | WS_VISIBLE | SS_LEFT,
                            137, 346, 454, 16, hwnd, (HMENU)ID_STATIC4, hInstance, NULL);
                           
    hStatic5 = CreateWindow("STATIC", STRING_INFO_OK, WS_CHILD | WS_VISIBLE | SS_LEFT,
                            0, 370, 600, 16, hwnd, (HMENU)ID_STATIC5, hInstance, NULL);

    if(mainSettings.use3DControls) {
        if(Ctl3dRegister(hInstance) && Ctl3dEnabled()) {
            ctlRegs=(CTL3D_ALL) & ~(CTL3D_STATICFRAMES);
            if(!mainSettings.use3DButtons) {
                ctlRegs=ctlRegs & ~(CTL3D_BUTTONS);
            }
            if(!mainSettings.use3DLists) {
                ctlRegs=ctlRegs & ~(CTL3D_LISTBOXES);
            }
            if(!mainSettings.use3DEdits) {
                ctlRegs=ctlRegs & ~(CTL3D_EDITS);
            }
            if(!mainSettings.use3DCombos) {
                ctlRegs=ctlRegs & ~(CTL3D_COMBOS);
            }
            Ctl3dSubclassDlg(hwnd,ctlRegs);
            if(mainSettings.use3DDialogs) {
                Ctl3dAutoSubclass(hInstance);
            }
        }
        else {
            MessageBox(hwnd,STRING_MSG_CTL3D_ERROR,STRING_WARNING,MB_ICONEXCLAMATION);
        }
    }

    if(mainSettings.savePosSizes) {
        if((mainSettings.mainWindowStyle+1)==SW_SHOWMINIMIZED) {
            ShowWindow(hwnd,SW_SHOWNORMAL);
        }
        else {
            ShowWindow(hwnd,(mainSettings.mainWindowStyle+1));
        }
    }
    else {
        if(mainSettings.mainWindowSystem) {
            ShowWindow(hwnd,nCmdShow);
        }
        else {
            ShowWindow(hwnd,(mainSettings.mainWindowStyle+1));
        }
    }
    UpdateWindow(hwnd);

    if(noter_connectionSettingsAvailable(connectionSettings)) {
        if(noter_credentialsAvailable(credentials)) {
            if(mainSettings.autoReload) {
                SendMessage(hwnd, WM_COMMAND, ID_BUTTON1, ID_FILE_RELOAD);
            }
        }
        else {
            lockRefreshButton(hwnd);
            lockOpenButton(hwnd);
            lockDeleteButton(hwnd);
            SendMessage(hwnd, WM_COMMAND, ID_OPTIONS_CREDENTIALS, 0);
        }
    }
    else {
        firstOptions=true;
        lockRefreshButton(hwnd);
        lockOpenButton(hwnd);
        lockDeleteButton(hwnd);
        SendMessage(hwnd, WM_COMMAND, ID_OPTIONS_CONNECTION, 0);
        if(noter_connectionSettingsAvailable(connectionSettings)) {
            SendMessage(hwnd, WM_COMMAND, ID_OPTIONS_CREDENTIALS, 0);
        }
    }

    firstOptions=false;

    while(GetMessage(&msg, NULL, 0, 0 )) {
        temp=GetParent(msg.hwnd);
        if(temp==NULL) {
            temp=msg.hwnd;
        }
        if(!TranslateAccelerator(temp, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return msg.wParam;
}

//////////////////////////////////////
//
//  MAIN WINDOW PROCEDURE
//
//////////////////////////////////////

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    std::string tempString="";
    long int index;
    long int result;
    unsigned long int width;
    unsigned long int height;
    int x, y, size_x, size_y;
    unsigned int state, compressionRatio;
    switch(msg) {
        case WM_CTLCOLOR:
            switch(HIWORD(lParam)) {
                case CTLCOLOR_STATIC:
                    if((LOWORD(lParam)!=GetDlgItem(hwnd,ID_STATIC5)) && (LOWORD(lParam)!=GetDlgItem(hwnd,ID_STATIC6))) {
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
            if(index>=0) {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_OPEN,MF_ENABLED);
                EnableMenuItem(GetMenu(hwnd),ID_FILE_DELETE,MF_ENABLED);
            }
            else {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_OPEN,MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),ID_FILE_DELETE,MF_GRAYED);
            }
            if(IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON1))) {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_RELOAD,MF_ENABLED);
            }
            else {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_RELOAD,MF_GRAYED);
            }
            if(IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON3))) {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_OPEN,MF_ENABLED);
            }
            else {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_OPEN,MF_GRAYED);
            }
            if(IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON4))) {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_EXIT,MF_ENABLED);
            }
            else {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_EXIT,MF_GRAYED);
            }
            if(IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON5))) {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_DELETE,MF_ENABLED);
            }
            else {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_DELETE,MF_GRAYED);
            }
            if(noter_connectionSettingsAvailable(connectionSettings)) {
                EnableMenuItem(GetMenu(hwnd),ID_OPTIONS_CREDENTIALS,MF_ENABLED);
            }
            else {
                EnableMenuItem(GetMenu(hwnd),ID_OPTIONS_CREDENTIALS,MF_GRAYED);
            }
            break;
        case WM_COMMAND:
            switch(wParam) {
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
                case ID_OPTIONS_CONNECTION:
                    if((MakeDialogBox(hwnd,IDD_DIALOG4,DlgProc4)==IDOK) && (editsChanged)) {
                        if((!firstOptions) && (noter_credentialsAvailable(credentials)) && (MessageBox(hwnd,STRING_MSG_WANT_RELOAD,APPNAME,MB_ICONQUESTION | MB_YESNO)==IDYES)) {
                            SendMessage(hwnd, WM_COMMAND, ID_BUTTON1, ID_FILE_RELOAD);
                        }
                    }
                    break;
                case ID_OPTIONS_CREDENTIALS:
                    if((MakeDialogBox(hwnd,IDD_DIALOG5,DlgProc5)==IDOK) && (editsChanged || firstOptions)) {
                        if(SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCOUNT, 0, 0)>0) {
                            SendMessage(hwnd, WM_COMMAND, ID_BUTTON1, ID_FILE_RELOAD);
                        }
                        else {
                            if(MessageBox(hwnd,STRING_MSG_WANT_RELOAD,APPNAME,MB_ICONQUESTION | MB_YESNO)==IDYES) {
                                SendMessage(hwnd, WM_COMMAND, ID_BUTTON1, ID_FILE_RELOAD);
                            }
                            if(firstOptions) {
                                firstOptions=false;
                            }
                        }
                    }
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
                    if(!IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON1))) {
                        break;
                    }
                    main_LockAllButtons(hwnd);
                    SetWindowText(GetDlgItem(hwnd,ID_STATIC5),STRING_LOADING_NOTE_LIST);
                    if(noteCount>0) {
                        freeNoteList(notes);
                    }
                    noteCount=noter_getNoteList(connectionSettings,credentials,buffer,notes);
                    index=SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0);
                    SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_RESETCONTENT, 0, 0);
                    for(long int x=0; x<noteCount; ++x) {
                        SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_ADDSTRING, 0, (LPARAM)toCodePage(mappedCodePage,(char*)notes[x].subject.c_str()).c_str());
                    }
                    if(index>=0) {
                        if(lParam==0) {
                            SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_SETCURSEL, 0, 0);
                        }
                        else {
                            SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_SETCURSEL, index, 0);
                        }
                    }
                    if(noteCount>=0) {
                        mainLastResult=INFO_LIST_SUCCESSFUL;
                        if(lParam!=0) {
                            tempString=noter_getAnswerString(mainLastResult)+STRING_SPACED_COUNT+IntToStr(noteCount)+".";
                            compressionRatio=getCompressionRatio();
                            if(compressionRatio!=100) {
                                tempString=tempString+STRING_SPACED_COMPRESSION+IntToStr(getCompressionRatio())+"%.";
                            }
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)tempString.c_str());
                        } else {
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC5),STRING_INFO_OK);
                        }
                    }
                    else {
                        mainLastResult=noteCount;
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                        EnableWindow(GetDlgItem(hwnd,ID_BUTTON3),false);
                        EnableWindow(GetDlgItem(hwnd,ID_BUTTON5),false);
                    }
                    main_UnlockAllButtons(hwnd);
                    break;
                case ID_BUTTON2:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON2))) {
                        break;
                    }
                    if(mainLastResult!=0) {
                        mainLastResult=0;
                        // SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                    }
                    SetWindowText(GetDlgItem(hwnd,ID_STATIC5),STRING_CREATING_EDIT_WINDOW);
                    if(createEditWindow(hwnd,winMem,NULL)==NULL) {
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),STRING_EDITWIN_CREATE_ERROR);
                        mainLastResult=-2048;
                    }
                    else {
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(INFO_OK).c_str());
                    }
                    break;
                case ID_BUTTON3:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON3))) {
                        break;
                    }
                    main_LockAllButtons(hwnd);
                    if(mainLastResult!=0) {
                        mainLastResult=0;
                        // SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                    }
                    SetWindowText(GetDlgItem(hwnd,ID_STATIC5),STRING_DOWNLOADING_NOTE);
                    index=SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0);
                    if(index>=0) {
                        NOTE *note=new NOTE;
                        result=noter_getNote(connectionSettings,credentials,notes[index].id,buffer,*note);
                        if(result>=0) {
                            HWND tempHwnd=createEditWindow(hwnd,winMem,note);
                            if(tempHwnd!=NULL) {
                                tempString=noter_getAnswerString(result)+STRING_SPACED_LAST_MOD_DATE+toCodePage(mappedCodePage,(char*)note->lastModified.c_str())+".";
                                compressionRatio=getCompressionRatio();
                                if(compressionRatio!=100) {
                                    tempString=tempString+STRING_SPACED_COMPRESSION+IntToStr(getCompressionRatio())+"%.";
                                }
                                SetWindowText(GetDlgItem(tempHwnd,ID_EDIT_STATIC4),(char*)tempString.c_str());
                                winMem[tempHwnd]->lastResult=result;
                                SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(INFO_OK).c_str());
                            }
                            else {
                                SetWindowText(GetDlgItem(hwnd,ID_STATIC5),STRING_EDITWIN_CREATE_ERROR);
                                mainLastResult=-2048;
                            }
                        }
                        else {
                            mainLastResult=result;
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                        }
                    }
                    main_UnlockAllButtons(hwnd);
                    break;
                case ID_BUTTON4:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON4))) {
                        break;
                    }
                    if(mainLastResult!=0) {
                        mainLastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                    }
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
                case ID_BUTTON5:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON5))) {
                        break;
                    }
                    if(mainLastResult!=0) {
                        mainLastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                    }
                    index=SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0);
                    tempString=STRING_MSG_WANT_NOTE_REMOVAL;
                    tempString=tempString+" \"";
                    tempString=tempString+toCodePage(mappedCodePage,(char*)notes[index].subject.c_str());
                    tempString=tempString+"\"?";
                    if(MessageBox(hwnd,(char*)tempString.c_str(),APPNAME,MB_ICONQUESTION | MB_YESNO)==IDYES) {
                        main_LockAllButtons(hwnd);
                        mainLastResult=noter_deleteNote(connectionSettings,credentials,notes[index].id,buffer);
                        main_UnlockAllButtons(hwnd);
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                        if(mainLastResult==INFO_NOTE_DELETED) {
                            SendMessage(hwnd,WM_COMMAND,ID_BUTTON1,0);
                        }
                    }
                    break;
                case ID_LISTBOX:
                    switch(HIWORD(lParam)) {
                        case LBN_DBLCLK:
                            SendMessage(hwnd, WM_COMMAND, ID_BUTTON3, 0);
                            break;
                        case LBN_SELCHANGE:
                            index=SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0);
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC3),IntToStr(notes[index].id).c_str());
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC4),notes[index].lastModified.c_str());
                            EnableWindow(GetDlgItem(hwnd,ID_BUTTON3),true);
                            EnableWindow(GetDlgItem(hwnd,ID_BUTTON5),true);
                            if(mainLastResult!=0) {
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

            if(width<240) {
                width=240;
            }
            if(height<240) {
                height=240;
            }

            if(mainSettings.use3DLists) {
                SetWindowPos(GetDlgItem(hwnd,ID_LISTBOX),NULL,0,0,width,height-88,SWP_NOZORDER | SWP_NOMOVE);
            }
            else {
                SetWindowPos(GetDlgItem(hwnd,ID_LISTBOX),NULL,0,0,width,height-86,SWP_NOZORDER | SWP_NOMOVE);
            }
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC6),NULL,0,0,width-400,21,SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC1),NULL,8,height-57,0,0,SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC2),NULL,8,height-40,0,0,SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC3),NULL,137,height-57,width-146,16,SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC4),NULL,137,height-40,width-146,16,SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC5),NULL,0,height-16,width,16,SWP_NOZORDER);

            break;
        case WM_GETMINMAXINFO:
            MINMAXINFO *lpMMI=(MINMAXINFO*)lParam;
            lpMMI->ptMinTrackSize.x=480;
            lpMMI->ptMinTrackSize.y=320;
            break;
        case WM_CLOSE:
            if(IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON4))) {
                if(winMem.size()>0) {
                    for(WINDOWMEMORY::iterator it = winMem.begin(); it != winMem.end(); ++it) {
                        SendMessage(it->second->hwnd, WM_CLOSE, 0, 0);
                    }
                }
                
                if(mainSettings.savePosSizes) {
                    getWindowCoordinates(hwnd,x,y,size_x,size_y,state);
                    if(state==SW_SHOWNORMAL) {
                        mainSettings.mainWindowStyle=state-1;
                        mainSettings.mainWindowX=x;
                        mainSettings.mainWindowY=y;
                        mainSettings.mainWindowSizeX=size_x;
                        mainSettings.mainWindowSizeY=size_y;
                    }
                    if(state==SW_SHOWMAXIMIZED) {
                        mainSettings.mainWindowStyle=state-1;
                    }

                    GetModuleFileName(GetWindowWord(hwnd,GWW_HINSTANCE),buffer,32767);
                    saveWindowCoordinatesSettings(mainSettings,(char*)getDefaultIniFile(buffer).c_str());
                }
                
                if(winMem.size()==0) {
                    DestroyWindow(hwnd);
                }
            }
            
            break;
        case WM_DESTROY:
            if(Ctl3dEnabled() && (!Ctl3dUnregister(GetWindowWord(hwnd,GWW_HINSTANCE)))) {
                MessageBox(0,STRING_MSG_CTL3D_UNREG_ERROR,STRING_WARNING,MB_ICONEXCLAMATION);
            }
            WSACleanup();
            if(noteCount>0) {
                freeNoteList(notes);
            }
            DeleteObject(g_hBrush);
            WinHelp(g_hwnd,"",HELP_QUIT,0);
            UnlockResource(hCodePageDefinition);
            FreeResource(hCodePageDefinition);
            FreeLibrary(hCodePageLib);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd,msg,wParam,lParam);
    }
    return 0;
}

//////////////////////////////////////
//
//  EDIT WINDOW PROCEDURE
//
//////////////////////////////////////

LRESULT CALLBACK WndProc2(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    unsigned long int width;
    unsigned long int height;
    unsigned long int sel;
    unsigned int result;
    int x, y, size_x, size_y;
    unsigned int state;
    switch(msg) {
        case WM_CTLCOLOR:
            switch(HIWORD(lParam)) {
                case CTLCOLOR_STATIC:
                    if((LOWORD(lParam)!=GetDlgItem(hwnd,ID_EDIT_STATIC3)) && (LOWORD(lParam)!=GetDlgItem(hwnd,ID_EDIT_STATIC4))) {
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
            if(winMem[hwnd]->note->id==0) {
                EnableMenuItem(GetMenu(hwnd),ID_FILE_PROPERTIES,MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),ID_FILE_TONEWNOTE,MF_GRAYED);
                ModifyMenu(GetMenu(hwnd),ID_FILE_ADDUP,MF_BYCOMMAND | MF_STRING,ID_FILE_ADDUP,STRING_MENU_ADD);
                //if((winMem[hwnd]->subjectChanged) && (winMem[hwnd]->entryChanged))
                if(IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON1))) {
                    EnableMenuItem(GetMenu(hwnd),ID_FILE_ADDUP,MF_ENABLED);
                }
                else {
                    EnableMenuItem(GetMenu(hwnd),ID_FILE_ADDUP,MF_GRAYED);
                }
            }
            else {
                if(IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON2))) {
                    EnableMenuItem(GetMenu(hwnd),ID_FILE_PROPERTIES,MF_ENABLED);
                }
                else {
                    EnableMenuItem(GetMenu(hwnd),ID_FILE_PROPERTIES,MF_GRAYED);
                }
                EnableMenuItem(GetMenu(hwnd),ID_FILE_TONEWNOTE,MF_ENABLED);
                ModifyMenu(GetMenu(hwnd),ID_FILE_ADDUP,MF_BYCOMMAND | MF_STRING,ID_FILE_ADDUP,STRING_MENU_UPDATE);
                //if((winMem[hwnd]->subjectChanged) || (winMem[hwnd]->entryChanged))
                if(IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON1))) {
                    EnableMenuItem(GetMenu(hwnd),ID_FILE_ADDUP,MF_ENABLED);
                }
                else {
                    EnableMenuItem(GetMenu(hwnd),ID_FILE_ADDUP,MF_GRAYED);
                }
            }
            if((GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX1)) || (GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2))) {
                if(SendMessage(GetFocus(), EM_CANUNDO, 0, 0)) {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_UNDO,MF_ENABLED);
                }
                else {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_UNDO,MF_GRAYED);
                }
                sel=SendMessage(GetFocus(), EM_GETSEL, 0, 0);
                if(HIWORD(sel)!=LOWORD(sel)) {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_CUT,MF_ENABLED);
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_COPY,MF_ENABLED);
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_CLEAR,MF_ENABLED);
                }
                else {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_CUT,MF_GRAYED);
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_COPY,MF_GRAYED);
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_CLEAR,MF_GRAYED);
                }
                if(OpenClipboard(GetFocus())) {
                    if(IsClipboardFormatAvailable(CF_TEXT) || IsClipboardFormatAvailable(CF_OEMTEXT)) {
                        EnableMenuItem(GetMenu(hwnd),ID_EDIT_PASTE,MF_ENABLED);
                    }
                    else {
                        EnableMenuItem(GetMenu(hwnd),ID_EDIT_PASTE,MF_GRAYED);
                    }
                    CloseClipboard();
                }
                else {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_PASTE,MF_GRAYED);
                }
                if((GetWindowTextLength(GetFocus())>0) && (GetWindowTextLength(GetFocus())>(HIWORD(sel)-LOWORD(sel)))) {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_SELECTALL,MF_ENABLED);
                }
                else {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_SELECTALL,MF_GRAYED);
                }
            }
            else {
                EnableMenuItem(GetMenu(hwnd),ID_EDIT_UNDO,MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),ID_EDIT_CUT,MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),ID_EDIT_COPY,MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),ID_EDIT_PASTE,MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),ID_EDIT_CLEAR,MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),ID_EDIT_SELECTALL,MF_GRAYED);
            }
            break;
        case WM_COMMAND:
            switch(wParam) {
                case ID_ACC_ENTER:
                    SendMessage(GetFocus(), WM_CHAR, VK_RETURN, 0);
                    break;
                case ID_ACC_TAB:
                    if(GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2)) {
                        SendMessage(GetFocus(), WM_CHAR, VK_TAB, 0);
                    }
                    else {
                        SetFocus(GetNextDlgTabItem(hwnd,GetFocus(),false));
                    }
                    break;
                case ID_ACC_CTRLS:
                    SendMessage(hwnd, WM_COMMAND, ID_FILE_ADDUP, 0);
                    break;
                case ID_ACC_CTRLD:
                    if(winMem[hwnd]->note->id!=0) {
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
                    if(IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON1))) {
                        SendMessage(hwnd, WM_COMMAND, ID_EDIT_BUTTON1, 0);
                    }
                    break;
                case ID_FILE_PROPERTIES:
                    if(IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON2))) {
                        SendMessage(hwnd, WM_COMMAND, ID_EDIT_BUTTON2, 0);
                    }
                    break;
                case ID_FILE_TONEWNOTE:
                    if(winMem[hwnd]->lastResult!=0) {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    winMem[hwnd]->note->id=0;
                    EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),true);
                    EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON2),false);
                    SetWindowText(GetDlgItem(hwnd,ID_EDIT_BUTTON1),STRING_ADD);
                    winMem[hwnd]->subjectChanged=true;
                    winMem[hwnd]->entryChanged=true;
                    makeEditWindowTitle(winMem[hwnd],NULL,true,mappedCodePage);
                    SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),STRING_TO_NEW_NOTE);
                    winMem[hwnd]->lastResult=1024;
                    break;
                case ID_FILE_EXIT:
                    SendMessage(hwnd, WM_COMMAND, ID_EDIT_BUTTON3, 0);
                    break;
                case ID_EDIT_UNDO:
                    if((GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX1)) || (GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2))) {
                        if(winMem[hwnd]->lastResult!=0) {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),WM_UNDO,0,0);
                    }
                    break;
                case ID_EDIT_CUT:
                    if((GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX1)) || (GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2))) {
                        if(winMem[hwnd]->lastResult!=0) {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),WM_CUT,0,0);
                    }
                    break;
                case ID_EDIT_COPY:
                    if((GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX1)) || (GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2))) {
                        if(winMem[hwnd]->lastResult!=0) {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),WM_COPY,0,0);
                    }
                    break;
                case ID_EDIT_PASTE:
                    if((GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX1)) || (GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2))) {
                        if(winMem[hwnd]->lastResult!=0) {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),WM_PASTE,0,0);
                    }
                    break;
                case ID_EDIT_CLEAR:
                    if((GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX1)) || (GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2))) {
                        if(winMem[hwnd]->lastResult!=0) {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),WM_CLEAR,0,0);
                    }
                    break;
                case ID_EDIT_SELECTALL:
                    if((GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX1)) || (GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2))) {
                        if(winMem[hwnd]->lastResult!=0) {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),EM_SETSEL,0,65535);
                    }
                    break;
                case ID_HELP_HELP:
                    if(winMem[hwnd]->lastResult!=0) {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    WinHelp(g_hwnd,HELPFILE,HELP_CONTENTS,0);
                    break;
                case ID_HELP_HOWTO:
                    if(winMem[hwnd]->lastResult!=0) {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    WinHelp(g_hwnd,"",HELP_HELPONHELP,0);
                    break;
                case ID_HELP_ABOUT:
                    if(winMem[hwnd]->lastResult!=0) {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    MakeDialogBox(hwnd,IDD_DIALOG2,DlgProc2);
                    break;
                case ID_EDIT_EDITBOX1:
                    switch(HIWORD(lParam)) {
                        case EN_CHANGE:
                            if(winMem[hwnd]!=NULL) {
                                if(!winMem[hwnd]->subjectChanged) {
                                    winMem[hwnd]->subjectChanged=true;
                                    if(winMem[hwnd]->note->id==0) {
                                        if(winMem[hwnd]->entryChanged) {
                                            EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),true);
                                        }
                                    }
                                    else {
                                        EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),true);
                                    }
                                }
                                if(winMem[hwnd]->lastResult!=0) {
                                    winMem[hwnd]->lastResult=0;
                                    SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                                }
                            }
                            break;
                    }
                    break;
                case ID_EDIT_EDITBOX2:
                    switch(HIWORD(lParam)) {
                        case EN_CHANGE:
                            if(winMem[hwnd]!=NULL) {
                                if(!winMem[hwnd]->entryChanged) {
                                    winMem[hwnd]->entryChanged=true;
                                    if(winMem[hwnd]->note->id==0) {
                                        if(winMem[hwnd]->subjectChanged) {
                                            EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),true);
                                        }
                                    }
                                    else {
                                        EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),true);
                                    }
                                }
                                if(winMem[hwnd]->lastResult!=0) {
                                    winMem[hwnd]->lastResult=0;
                                    SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                                }
                            }
                            break;
                    }
                    break;
                case ID_EDIT_BUTTON1:
                    if((!IsWindowEnabled(GetDlgItem(g_hwnd,ID_BUTTON4))) || (!IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON1)))) {
                        break;
                    }
                    if(winMem[hwnd]->lastResult!=0) {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    GetWindowText(GetDlgItem(hwnd,ID_EDIT_EDITBOX1),buffer,65535);
                    winMem[hwnd]->note->subject=fromCodePage(rawCodePage,buffer);
                    GetWindowText(GetDlgItem(hwnd,ID_EDIT_EDITBOX2),buffer,65535);
                    winMem[hwnd]->note->entry=fromCodePage(rawCodePage,buffer);
                    if(winMem[hwnd]->note->id==0) {
                        edit_LockAllButtons(hwnd);
                        winMem[hwnd]->lastResult=noter_addNote(connectionSettings,credentials,*winMem[hwnd]->note,buffer);
                        edit_UnlockAllButtons(hwnd);
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        if(winMem[hwnd]->lastResult>=0) {
                            if(mainSettings.autoRefresh) {
                                SendMessage(g_hwnd,WM_COMMAND,ID_BUTTON1,0);
                            }
                            //EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),false);
                            //EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON2),true);
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_BUTTON1),STRING_UPDATE);
                            winMem[hwnd]->subjectChanged=false;
                            winMem[hwnd]->entryChanged=false;
                            makeEditWindowTitle(winMem[hwnd],winMem[hwnd]->note,true,mappedCodePage);
                        }
                        else {
                            EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),true);
                        }
                    }
                    else {
                        edit_LockAllButtons(hwnd);
                        winMem[hwnd]->lastResult=noter_updateNote(connectionSettings,credentials,*winMem[hwnd]->note,buffer);
                        edit_UnlockAllButtons(hwnd);
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        if(winMem[hwnd]->lastResult>=0) {
                            if(mainSettings.autoRefresh) {
                                SendMessage(g_hwnd,WM_COMMAND,ID_BUTTON1,0);
                            }
                            //EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),false);
                            winMem[hwnd]->subjectChanged=false;
                            winMem[hwnd]->entryChanged=false;
                            makeEditWindowTitle(winMem[hwnd],winMem[hwnd]->note,true,mappedCodePage);
                        }
                        else {
                            EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),true);
                        }
                    }
                    break;
                case ID_EDIT_BUTTON2:
                    if((!IsWindowEnabled(GetDlgItem(g_hwnd,ID_BUTTON4))) || (!IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON2)))) {
                        break;
                    }
                    if(winMem[hwnd]->lastResult!=0) {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    MakeDialogBox(hwnd,IDD_DIALOG1,DlgProc);
                    break;
                case ID_EDIT_BUTTON3:
                    if((!IsWindowEnabled(GetDlgItem(g_hwnd,ID_BUTTON4))) || (!IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON3)))) {
                        break;
                    }
                    if(winMem[hwnd]->lastResult!=0) {
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
        case WM_GETMINMAXINFO:
            MINMAXINFO *lpMMI=(MINMAXINFO*)lParam;
            lpMMI->ptMinTrackSize.x=480;
            lpMMI->ptMinTrackSize.y=320;
            break;
        case WM_CLOSE:
            if((!IsWindowEnabled(GetDlgItem(g_hwnd,ID_BUTTON4))) || (!IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON3)))) {
                break;
            }
            if((winMem[hwnd]!=NULL) && (winMem[hwnd]->subjectChanged || winMem[hwnd]->entryChanged)) {
                //GetWindowText(hwnd,buffer,32767);
                result=MessageBox(hwnd,STRING_MSG_WANT_CHANGES_SAVED,APPNAME,MB_ICONEXCLAMATION | MB_YESNOCANCEL);
            }
            else {
                result=IDNO;
            }
            if(result==IDYES) {
                SendMessage(hwnd,WM_COMMAND,ID_EDIT_BUTTON1,0);
            }
            else if(result==IDNO) {
                winMem[hwnd]->lastResult=0;
            }

            if(mainSettings.savePosSizes) {
                getWindowCoordinates(hwnd,x,y,size_x,size_y,state);
                if(state==SW_SHOWNORMAL) {
                    mainSettings.editWindowStyle=state-1;
                    mainSettings.editWindowX=x;
                    mainSettings.editWindowY=y;
                    mainSettings.editWindowSizeX=size_x;
                    mainSettings.editWindowSizeY=size_y;
                }
                if(state==SW_SHOWMAXIMIZED) {
                    mainSettings.editWindowStyle=state-1;
                }
            }
            
            if((result!=IDCANCEL) && (winMem[hwnd]->lastResult>=0)) {
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
//  NOTE PROPERTIES DIALOG PROCEDURE
//
//////////////////////////////////////

BOOL CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    NOTE tempNote;
    long int result;
    bool addUpTest=false;
    switch(msg) {
        case WM_INITDIALOG:
            addUpTest=IsWindowEnabled(GetDlgItem(GetParent(hwnd),ID_EDIT_BUTTON1));
            properties_LockAllButtons(hwnd);
            result=noter_getNote(connectionSettings,credentials,winMem[GetParent(hwnd)]->note->id,buffer,tempNote);
            properties_UnlockAllButtons(hwnd);
            EnableWindow(GetDlgItem(GetParent(hwnd),ID_EDIT_BUTTON1),addUpTest);
            if(result>=0) {
                SetWindowText(GetDlgItem(hwnd,IDC_STATIC1),(char*)toCodePage(mappedCodePage,(char*)tempNote.subject.c_str()).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_STATIC2),(char*)IntToStr(tempNote.id).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_STATIC3),(char*)toCodePage(mappedCodePage,(char*)tempNote.dateAdded.c_str()).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_STATIC4),(char*)toCodePage(mappedCodePage,(char*)tempNote.userAgent.c_str()).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_STATIC5),(char*)toCodePage(mappedCodePage,(char*)tempNote.lastModified.c_str()).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_STATIC6),(char*)toCodePage(mappedCodePage,(char*)tempNote.lastUserAgent.c_str()).c_str());
                if(tempNote.locked) {
                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON1),false);
                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON2),true);
                }
                else {
                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON1),true);
                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON2),false);
                }
            }
            else {
                //GetWindowText(GetParent(hwnd),buffer,32767);
                MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),APPNAME,MB_ICONHAND | MB_OK);
                EndDialog(hwnd,IDOK);
            }
            break;
            /*
        case WM_CTLCOLOR:
            switch(HIWORD(lParam)) {
                case CTLCOLOR_BTN:
                    SetBkMode((HDC)wParam, TRANSPARENT);
                    break;
            }
            break;
            */
        case WM_COMMAND:
            switch(wParam) {
                case IDOK:
                    EndDialog(hwnd,IDOK);
                    break;
                case IDC_BUTTON1:
                    result=noter_lockNote(connectionSettings,credentials,winMem[GetParent(hwnd)]->note->id,buffer);
                    if(result>=0) {
                        winMem[GetParent(hwnd)]->note->locked=true;
                        tempNote.locked=true;
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON1),false);
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON2),true);
                    }
                    else {
                        //GetWindowText(GetParent(hwnd),buffer,32767);
                        MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),APPNAME,MB_ICONHAND | MB_OK);
                    }
                    break;
                case IDC_BUTTON2:
                    result=noter_unlockNote(connectionSettings,credentials,winMem[GetParent(hwnd)]->note->id,buffer);
                    if(result>=0) {
                        winMem[GetParent(hwnd)]->note->locked=false;
                        tempNote.locked=false;
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON1),true);
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON2),false);
                    }
                    else {
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
//  PROGRAM INFORMATION DIALOG PROCEDURE
//
//////////////////////////////////////

BOOL CALLBACK DlgProc2(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    NOTE tempNote;
    switch(msg) {
        case WM_INITDIALOG:
            /*
            if(Ctl3dEnabled()) {
                unsigned int ctlRegs=CTL3D_ALL;
                Ctl3dSubclassDlg(hwnd,ctlRegs);
            }
            */
            SetWindowText(GetDlgItem(hwnd,IDC_STATIC7),APPNAME);
            break;
            /*
        case WM_CTLCOLOR:
            switch(HIWORD(lParam)) {
                case CTLCOLOR_BTN:
                    SetBkMode((HDC)wParam, TRANSPARENT);
                    break;
            }
            break;
            */
        case WM_COMMAND:
            switch(wParam) {
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
//  PREFERENCES DIALOG PROCEDURE
//
//////////////////////////////////////

BOOL CALLBACK DlgProc3(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    bool enabled;
    std::string iniFile;
    switch(msg) {
        case WM_INITDIALOG:
            check3DChanged=false;
            SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)STRING_NORMAL_WINDOW);
            SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)STRING_MINIMIZED_WINDOW);
            SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)STRING_MAXIMIZED_WINDOW);
            SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)STRING_NORMAL_WINDOW);
            SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)STRING_MINIMIZED_WINDOW);
            SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)STRING_MAXIMIZED_WINDOW);
            // temporary part
            SendMessage(GetDlgItem(hwnd, IDC_COMBO3), CB_ADDSTRING, 0, (LPARAM)"Polski");
            SendMessage(GetDlgItem(hwnd, IDC_COMBO4), CB_ADDSTRING, 0, (LPARAM)"1250");
            SendMessage(GetDlgItem(hwnd, IDC_COMBO3), CB_SETCURSEL, 0, 0);
            SendMessage(GetDlgItem(hwnd, IDC_COMBO4), CB_SETCURSEL, 0, 0);
            // end of temporary part
            SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_SETCURSEL, mainSettings.mainWindowStyle, 0);
            SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_SETCURSEL, mainSettings.editWindowStyle, 0);
            SendMessage(GetDlgItem(hwnd, IDC_COMBO1), WM_PAINT, 0, 0);
            SendMessage(GetDlgItem(hwnd, IDC_COMBO2), WM_PAINT, 0, 0);
            SendMessage(GetDlgItem(hwnd, IDC_COMBO3), WM_PAINT, 0, 0);
            SendMessage(GetDlgItem(hwnd, IDC_COMBO4), WM_PAINT, 0, 0);
            if(mainSettings.mainWindowSystem) {
                CheckRadioButton(hwnd, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
                EnableWindow(GetDlgItem(hwnd,IDC_COMBO1),false);
            }
            else {
                CheckRadioButton(hwnd, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
                EnableWindow(GetDlgItem(hwnd,IDC_COMBO1),true);
            }
            if(mainSettings.editWindowSystem) {
                CheckRadioButton(hwnd, IDC_RADIO3, IDC_RADIO4, IDC_RADIO3);
                EnableWindow(GetDlgItem(hwnd,IDC_COMBO2),false);
            }
            else {
                CheckRadioButton(hwnd, IDC_RADIO3, IDC_RADIO4, IDC_RADIO4);
                EnableWindow(GetDlgItem(hwnd,IDC_COMBO2),true);
            }
            if(mainSettings.autoReload) {
                CheckDlgButton(hwnd, IDC_CHECK2, BST_CHECKED);
            }
            else {
                CheckDlgButton(hwnd, IDC_CHECK2, BST_UNCHECKED);
            }
            if(mainSettings.autoRefresh) {
                CheckDlgButton(hwnd, IDC_CHECK9, BST_CHECKED);
            }
            else {
                CheckDlgButton(hwnd, IDC_CHECK9, BST_UNCHECKED);
            }
            if(mainSettings.savePosSizes) {
                CheckDlgButton(hwnd, IDC_CHECK10, BST_CHECKED);
            }
            else {
                CheckDlgButton(hwnd, IDC_CHECK10, BST_UNCHECKED);
            }
            if(mainSettings.use3DControls) {
                CheckDlgButton(hwnd, IDC_CHECK3, BST_CHECKED);
            }
            else {
                CheckDlgButton(hwnd, IDC_CHECK3, BST_UNCHECKED);
            }
            if(mainSettings.use3DButtons) {
                CheckDlgButton(hwnd, IDC_CHECK4, BST_CHECKED);
            }
            else {
                CheckDlgButton(hwnd, IDC_CHECK4, BST_UNCHECKED);
            }
            if(mainSettings.use3DLists) {
                CheckDlgButton(hwnd, IDC_CHECK5, BST_CHECKED);
            }
            else {
                CheckDlgButton(hwnd, IDC_CHECK5, BST_UNCHECKED);
            }
            if(mainSettings.use3DEdits) {
                CheckDlgButton(hwnd, IDC_CHECK6, BST_CHECKED);
            }
            else {
                CheckDlgButton(hwnd, IDC_CHECK6, BST_UNCHECKED);
            }
            if(mainSettings.use3DCombos) {
                CheckDlgButton(hwnd, IDC_CHECK7, BST_CHECKED);
            }
            else {
                CheckDlgButton(hwnd, IDC_CHECK7, BST_UNCHECKED);
            }
            if(mainSettings.use3DDialogs) {
                CheckDlgButton(hwnd, IDC_CHECK8, BST_CHECKED);
            }
            else {
                CheckDlgButton(hwnd, IDC_CHECK8, BST_UNCHECKED);
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
            switch(HIWORD(lParam)) {
                case CTLCOLOR_BTN:
                    SetBkMode((HDC)wParam, TRANSPARENT);
                    break;
            }
            break;
            */
        case WM_COMMAND:
            switch(wParam) {
                case IDOK:
                    GetModuleFileName(GetWindowWord(g_hwnd,GWW_HINSTANCE),buffer,32767);
                    iniFile=getDefaultIniFile(buffer);
                    mainSettings.mainWindowSystem=IsDlgButtonChecked(hwnd, IDC_RADIO1);
                    mainSettings.editWindowSystem=IsDlgButtonChecked(hwnd, IDC_RADIO3);
                    mainSettings.mainWindowStyle=SendMessage(GetDlgItem(hwnd,IDC_COMBO1), CB_GETCURSEL, 0, 0);
                    mainSettings.editWindowStyle=SendMessage(GetDlgItem(hwnd,IDC_COMBO2), CB_GETCURSEL, 0, 0);
                    mainSettings.autoReload=IsDlgButtonChecked(hwnd, IDC_CHECK2);
                    mainSettings.autoRefresh=IsDlgButtonChecked(hwnd, IDC_CHECK9);
                    if((mainSettings.savePosSizes==false) && (IsDlgButtonChecked(hwnd, IDC_CHECK10))) {
                        mainSettings.mainWindowX=CW_USEDEFAULT;
                        mainSettings.mainWindowY=CW_USEDEFAULT;
                        mainSettings.mainWindowSizeX=CW_USEDEFAULT;
                        mainSettings.mainWindowSizeY=CW_USEDEFAULT;
                        mainSettings.editWindowX=CW_USEDEFAULT;
                        mainSettings.editWindowY=CW_USEDEFAULT;
                        mainSettings.editWindowSizeX=CW_USEDEFAULT;
                        mainSettings.editWindowSizeY=CW_USEDEFAULT;
                    }
                    mainSettings.savePosSizes=IsDlgButtonChecked(hwnd, IDC_CHECK10);
                    mainSettings.use3DControls=IsDlgButtonChecked(hwnd, IDC_CHECK3);
                    mainSettings.use3DButtons=IsDlgButtonChecked(hwnd, IDC_CHECK4);
                    mainSettings.use3DLists=IsDlgButtonChecked(hwnd, IDC_CHECK5);
                    mainSettings.use3DEdits=IsDlgButtonChecked(hwnd, IDC_CHECK6);
                    mainSettings.use3DCombos=IsDlgButtonChecked(hwnd, IDC_CHECK7);
                    mainSettings.use3DDialogs=IsDlgButtonChecked(hwnd, IDC_CHECK8);
                    saveMainSettings(mainSettings,(char*)iniFile.c_str());
                    if(check3DChanged) {
                        MessageBox(hwnd,STRING_MSG_CTL3D_CHANGE,STRING_INFORMATION,MB_ICONINFORMATION | MB_OK);
                    }
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
                    check3DChanged=true;
                    enabled=IsDlgButtonChecked(hwnd, IDC_CHECK3);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK4),enabled);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK5),enabled);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK6),enabled);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK7),enabled);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK8),enabled);
                    break;
                case IDC_CHECK4:
                    check3DChanged=true;
                    break;
                case IDC_CHECK5:
                    check3DChanged=true;
                    break;
                case IDC_CHECK6:
                    check3DChanged=true;
                    break;
                case IDC_CHECK7:
                    check3DChanged=true;
                    break;
                case IDC_CHECK8:
                    check3DChanged=true;
                    break;
            }
            break;
        case WM_CLOSE:
            EndDialog(hwnd,IDCANCEL);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

//////////////////////////////////////
//
//  CONNECTION SETTINGS DIALOG PROCEDURE
//
//////////////////////////////////////

BOOL CALLBACK DlgProc4(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    NOTER_SERVER_INFO serverInfo;
    NOTER_CONNECTION_SETTINGS tempConnectionSettings;
    char* ipAddress=NULL;
    std::string iniFile;
    switch(msg) {
        case WM_INITDIALOG:
            if(noter_connectionSettingsAvailable(connectionSettings)) {
                SetWindowText(GetDlgItem(hwnd,IDC_EDIT1),(char*)connectionSettings.serverAddress.c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_EDIT2),(char*)IntToStr(connectionSettings.port).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_EDIT3),(char*)connectionSettings.share.c_str());
                if(connectionSettings.requestCompression) {
                    CheckDlgButton(hwnd, IDC_CHECK11, BST_CHECKED);
                }
                else {
                    CheckDlgButton(hwnd, IDC_CHECK11, BST_UNCHECKED);
                }
                connection_LockAllButtons(hwnd);
                serverInfo=noter_getServerInfo(connectionSettings,buffer);
                connection_UnlockAllButtons(hwnd);
                if(serverInfo.version==MATCH_VERSION) {
                    SetWindowText(GetDlgItem(hwnd,IDC_STATIC8),(char*)toCodePage(mappedCodePage,(char*)serverInfo.name.c_str()).c_str());
                    SetWindowText(GetDlgItem(hwnd,IDC_STATIC9),(char*)toCodePage(mappedCodePage,(char*)serverInfo.timezone.c_str()).c_str());
                    SetWindowText(GetDlgItem(hwnd,IDC_STATIC10),(char*)toCodePage(mappedCodePage,(char*)serverInfo.version.c_str()).c_str());
                }
            }
            else {
                EnableWindow(GetDlgItem(hwnd,IDOK),false);
                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON3),false);
                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON4),false);
            }
            editsChanged=false;
            break;
        case WM_COMMAND:
            switch(wParam) {
                case IDOK:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDOK))) {
                        break;
                    }
                    GetWindowText(GetDlgItem(hwnd,IDC_EDIT2),buffer,65535);
                    if(checkIfInt(buffer)) {
                        if(editsChanged) {
                            GetWindowText(GetDlgItem(hwnd,IDC_EDIT1),buffer,65535);
                            connectionSettings.serverAddress=buffer;
                            GetWindowText(GetDlgItem(hwnd,IDC_EDIT2),buffer,65535);
                            connectionSettings.port=StrToInt(buffer);
                            GetWindowText(GetDlgItem(hwnd,IDC_EDIT3),buffer,65535);
                            connectionSettings.share=buffer;
                            connectionSettings.requestCompression=IsDlgButtonChecked(hwnd, IDC_CHECK11);
                            GetModuleFileName(GetWindowWord(g_hwnd,GWW_HINSTANCE),buffer,32767);
                            iniFile=getDefaultIniFile(buffer);
                            saveConnectionSettings(connectionSettings,(char*)iniFile.c_str());
                        }
                        EndDialog(hwnd,IDOK);
                    }
                    else {
                        MessageBox(hwnd,STRING_MSG_WRONG_PORT_NUMBER,STRING_ERROR,MB_ICONEXCLAMATION | MB_OK);
                    }
                    break;
                case IDCANCEL:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDCANCEL))) {
                        break;
                    }
                    EndDialog(hwnd,IDCANCEL);
                    break;
                case IDC_CHECK11:
                    editsChanged=true;  // yeah, a bit ugly...
                    break;
                case IDC_BUTTON4:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDC_BUTTON4))) {
                        break;
                    }
                    unsigned int test=0;
                    GetWindowText(GetDlgItem(hwnd,IDC_EDIT2),buffer,65535);
                    if(checkIfInt(buffer)) {
                        GetWindowText(GetDlgItem(hwnd,IDC_EDIT1),buffer,65535);
                        tempConnectionSettings.serverAddress=buffer;
                        GetWindowText(GetDlgItem(hwnd,IDC_EDIT2),buffer,65535);
                        tempConnectionSettings.port=StrToInt(buffer);
                        GetWindowText(GetDlgItem(hwnd,IDC_EDIT3),buffer,65535);
                        tempConnectionSettings.share=buffer;
                        connection_LockAllButtons(hwnd);
                        serverInfo=noter_getServerInfo(tempConnectionSettings,buffer);
                        connection_UnlockAllButtons(hwnd);
                        if(noter_checkServerVersion(serverInfo)) {
                            SetWindowText(GetDlgItem(hwnd,IDC_STATIC8),(char*)toCodePage(mappedCodePage,(char*)serverInfo.name.c_str()).c_str());
                            SetWindowText(GetDlgItem(hwnd,IDC_STATIC9),(char*)toCodePage(mappedCodePage,(char*)serverInfo.timezone.c_str()).c_str());
                            SetWindowText(GetDlgItem(hwnd,IDC_STATIC10),(char*)toCodePage(mappedCodePage,(char*)serverInfo.version.c_str()).c_str());
                            MessageBox(hwnd,STRING_MSG_CONN_ESTABLISHED,STRING_INFORMATION,MB_ICONINFORMATION | MB_OK);
                        }
                        else {
                            SetWindowText(GetDlgItem(hwnd,IDC_STATIC8),STRING_NOT_CONNECTED);
                            SetWindowText(GetDlgItem(hwnd,IDC_STATIC9),STRING_NOT_CONNECTED);
                            SetWindowText(GetDlgItem(hwnd,IDC_STATIC10),STRING_NOT_CONNECTED);
                            MessageBox(hwnd,STRING_MSG_CONNECTION_ERROR,STRING_ERROR,MB_ICONEXCLAMATION | MB_OK);
                        }
                    }
                    else {
                        MessageBox(hwnd,STRING_MSG_WRONG_PORT_NUMBER,STRING_ERROR,MB_ICONEXCLAMATION | MB_OK);
                    }
                    break;
                case IDC_EDIT1:
                case IDC_EDIT2:
                case IDC_EDIT3:
                    switch(HIWORD(lParam)) {
                        case EN_CHANGE:
                            if(GetWindowTextLength(GetDlgItem(hwnd,wParam))==0) {
                                EnableWindow(GetDlgItem(hwnd,IDOK),false);
                                if(wParam==IDC_EDIT1) {
                                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON3),false);
                                }
                                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON4),false);
                            }
                            else {
                                if(wParam==IDC_EDIT1) {
                                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON3),true);
                                }
                                if((GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT1))>0)
                                    && (GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT2))>0)
                                    && (GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT3))>0))
                                {
                                    EnableWindow(GetDlgItem(hwnd,IDOK),true);
                                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON4),true);
                                }
                            }
                            editsChanged=true;
                            break;
                    }
                    break;
            }
            break;
        case WM_CLOSE:
            if(IsWindowEnabled(GetDlgItem(hwnd,IDOK))) {
                EndDialog(hwnd,IDCANCEL);
            }
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

//////////////////////////////////////
//
//  CREDENTIALS SETTINGS DIALOG PROCEDURE
//
//////////////////////////////////////

BOOL CALLBACK DlgProc5(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    USER_INFO userInfo;
    long int result;
    std::string tempString, iniFile;
    switch(msg) {
        case WM_INITDIALOG:
            if(noter_connectionSettingsAvailable(connectionSettings)) {
                if(noter_credentialsAvailable(credentials)) {
                    SetWindowText(GetDlgItem(hwnd,IDC_EDIT4),(char*)credentials.username.c_str());
                    SetWindowText(GetDlgItem(hwnd,IDC_EDIT5),(char*)credentials.password.c_str());
                    credentials_LockAllButtons(hwnd);
                    result=noter_getUserInfo(connectionSettings,credentials,buffer,userInfo);
                    credentials_UnlockAllButtons(hwnd);
                    if(result>=0) {
                        SetWindowText(GetDlgItem(hwnd,IDC_STATIC11),(char*)IntToStr(userInfo.ID).c_str());
                        SetWindowText(GetDlgItem(hwnd,IDC_STATIC12),(char*)userInfo.dateRegistered.c_str());
                        SetWindowText(GetDlgItem(hwnd,IDC_STATIC13),(char*)userInfo.userAgent.c_str());
                        SetWindowText(GetDlgItem(hwnd,IDC_STATIC14),(char*)userInfo.lastChanged.c_str());
                        SetWindowText(GetDlgItem(hwnd,IDC_STATIC15),(char*)userInfo.lastUserAgent.c_str());
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON7),true);
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON8),true);
                    }
                    else {
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON7),false);
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON8),false);
                    }
                }
                else {
                    EnableWindow(GetDlgItem(hwnd,IDOK),false);
                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON6),false);
                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON7),false);
                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON8),false);
                }
            }
            else {
                EnableWindow(GetDlgItem(hwnd,IDOK),false);
                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON5),false);
                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON6),false);
                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON7),false);
                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON8),false);
            }
            editsChanged=false;
            editsChanged2=false;
            useTestCredentials=false;
            break;
        case WM_COMMAND:
            switch(wParam) {
                case IDOK:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDOK))) {
                        break;
                    }
                    if(editsChanged) {
                        GetWindowText(GetDlgItem(hwnd,IDC_EDIT4),buffer,65535);
                        credentials.username=buffer;
                        GetWindowText(GetDlgItem(hwnd,IDC_EDIT5),buffer,65535);
                        credentials.password=buffer;
                        GetModuleFileName(GetWindowWord(g_hwnd,GWW_HINSTANCE),buffer,32767);
                        iniFile=getDefaultIniFile(buffer);
                        saveCredentials(credentials,(char*)iniFile.c_str());
                        EnableWindow(GetDlgItem(GetParent(hwnd),ID_BUTTON1),true);
                    }
                    EndDialog(hwnd,IDOK);
                    break;
                case IDCANCEL:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDCANCEL))) {
                        break;
                    }
                    if((editsChanged2) && (MessageBox(hwnd,STRING_MSG_CREDENTIALS_CHANGED,APPNAME,MB_ICONQUESTION | MB_YESNO)==IDNO)) {
                        break;
                    }
                    EndDialog(hwnd,IDCANCEL);
                    break;
                case IDC_BUTTON5:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDC_BUTTON5))) {
                        break;
                    }
                    auxCredentials=&tempCredentials;
                    if(MakeDialogBox(hwnd,IDD_DIALOG6,DlgProc6)==IDOK) {
                        if(editsChanged2) {
                            SetWindowText(GetDlgItem(hwnd,IDC_EDIT4),(char*)tempCredentials.username.c_str());
                            SetWindowText(GetDlgItem(hwnd,IDC_EDIT5),(char*)tempCredentials.password.c_str());
                            useTestCredentials=true;
                            SendMessage(hwnd, WM_COMMAND, IDC_BUTTON6, IDC_BUTTON5);
                        }
                    }
                    break;
                case IDC_BUTTON6:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDC_BUTTON6))) {
                        break;
                    }
                    if(noter_connectionSettingsAvailable(connectionSettings)) {
                        GetWindowText(GetDlgItem(hwnd,IDC_EDIT4),buffer,65535);
                        tempCredentials.username=buffer;
                        GetWindowText(GetDlgItem(hwnd,IDC_EDIT5),buffer,65535);
                        tempCredentials.password=buffer;
                        if(noter_credentialsAvailable(tempCredentials)) {
                            credentials_LockAllButtons(hwnd);
                            result=noter_getUserInfo(connectionSettings,tempCredentials,buffer,userInfo);
                            credentials_UnlockAllButtons(hwnd);
                            if(result>=0) {
                                SetWindowText(GetDlgItem(hwnd,IDC_STATIC11),(char*)IntToStr(userInfo.ID).c_str());
                                SetWindowText(GetDlgItem(hwnd,IDC_STATIC12),(char*)userInfo.dateRegistered.c_str());
                                SetWindowText(GetDlgItem(hwnd,IDC_STATIC13),(char*)userInfo.userAgent.c_str());
                                SetWindowText(GetDlgItem(hwnd,IDC_STATIC14),(char*)userInfo.lastChanged.c_str());
                                SetWindowText(GetDlgItem(hwnd,IDC_STATIC15),(char*)userInfo.lastUserAgent.c_str());
                                if(editsChanged) {
                                    useTestCredentials=true;
                                }
                                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON7),true);
                                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON8),true);
                                if(lParam!=IDC_BUTTON5) {
                                    MessageBox(hwnd,STRING_MSG_LOGIN_SUCCESSFUL,STRING_INFORMATION,MB_ICONINFORMATION | MB_OK);
                                }
                            }
                            else {
                                SetWindowText(GetDlgItem(hwnd,IDC_STATIC11),STRING_NOT_LOGGED_IN);
                                SetWindowText(GetDlgItem(hwnd,IDC_STATIC12),STRING_NOT_LOGGED_IN);
                                SetWindowText(GetDlgItem(hwnd,IDC_STATIC13),STRING_NOT_LOGGED_IN);
                                SetWindowText(GetDlgItem(hwnd,IDC_STATIC14),STRING_NOT_LOGGED_IN);
                                SetWindowText(GetDlgItem(hwnd,IDC_STATIC15),STRING_NOT_LOGGED_IN);
                                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON7),false);
                                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON8),false);
                                MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),STRING_ERROR,MB_ICONEXCLAMATION | MB_OK);
                            }
                        }
                        else {
                            MessageBox(hwnd,STRING_NO_CREDENTIALS,STRING_ERROR,MB_ICONEXCLAMATION | MB_OK);
                        }
                    }
                    else {
                        MessageBox(hwnd,STRING_MSG_NO_CONN_SETTINGS,STRING_ERROR,MB_ICONEXCLAMATION | MB_OK);
                    }
                    break;
                case IDC_BUTTON7:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDC_BUTTON7))) {
                        break;
                    }
                    if(useTestCredentials) {
                        auxCredentials=&tempCredentials;
                    }
                    else {
                        auxCredentials=&credentials;
                    }
                    if(noter_credentialsAvailable(*auxCredentials)) {
                        tempString=STRING_MSG_ACC_DELETE_PART1
                                    +auxCredentials->username
                                    +STRING_MSG_ACC_DELETE_PART2;
                        if(MessageBox(hwnd,(char*)tempString.c_str(),APPNAME,MB_ICONQUESTION | MB_YESNO)==IDYES) {
                            if(MakeDialogBox(hwnd,IDD_DIALOG7,DlgProc7)==IDOK) {
                                if(useTestCredentials) {
                                    SetWindowText(GetDlgItem(hwnd,IDC_EDIT4),"");
                                    SetWindowText(GetDlgItem(hwnd,IDC_EDIT5),"");
                                    SetWindowText(GetDlgItem(hwnd,IDC_STATIC11),STRING_NOT_LOGGED_IN);
                                    SetWindowText(GetDlgItem(hwnd,IDC_STATIC12),STRING_NOT_LOGGED_IN);
                                    SetWindowText(GetDlgItem(hwnd,IDC_STATIC13),STRING_NOT_LOGGED_IN);
                                    SetWindowText(GetDlgItem(hwnd,IDC_STATIC14),STRING_NOT_LOGGED_IN);
                                    SetWindowText(GetDlgItem(hwnd,IDC_STATIC15),STRING_NOT_LOGGED_IN);
                                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON7),false);
                                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON8),false);
                                }
                                else {
                                    credentials.username="";
                                    credentials.password="";
                                    GetModuleFileName(GetWindowWord(g_hwnd,GWW_HINSTANCE),buffer,32767);
                                    iniFile=getDefaultIniFile(buffer);
                                    saveCredentials(credentials,(char*)iniFile.c_str());
                                    lockRefreshButton(GetParent(hwnd));
                                    lockOpenButton(GetParent(hwnd));
                                    lockDeleteButton(GetParent(hwnd));
                                    EndDialog(hwnd,IDOK);
                                }
                            }
                        }
                    }
                    else {
                        MessageBox(hwnd,STRING_NO_CREDENTIALS,STRING_ERROR,MB_ICONEXCLAMATION | MB_OK);
                    }
                    break;
                case IDC_BUTTON8:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDC_BUTTON8))) {
                        break;
                    }
                    if(useTestCredentials) {
                        auxCredentials=&tempCredentials;
                    }
                    else {
                        tempCredentials=credentials;
                        auxCredentials=&tempCredentials;
                    }
                    if(noter_credentialsAvailable(*auxCredentials)) {
                        if(MakeDialogBox(hwnd,IDD_DIALOG8,DlgProc8)==IDOK) {
                            SetWindowText(GetDlgItem(hwnd,IDC_EDIT5),(char*)tempCredentials.password.c_str());
                            useTestCredentials=true;
                            SendMessage(hwnd, WM_COMMAND, IDC_BUTTON6, IDC_BUTTON5);
                        }
                    }
                    else {
                        MessageBox(hwnd,STRING_NO_CREDENTIALS,STRING_ERROR,MB_ICONEXCLAMATION | MB_OK);
                    }
                    break;
                case IDC_EDIT4:
                case IDC_EDIT5:
                    switch(HIWORD(lParam)) {
                        case EN_CHANGE:
                            editsChanged=true;
                            if(GetWindowTextLength(GetDlgItem(hwnd,wParam))==0) {
                                EnableWindow(GetDlgItem(hwnd,IDOK),false);
                                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON6),false);
                            }
                            else {
                                if(GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT4))>0
                                    && GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT5))>0)
                                {
                                    EnableWindow(GetDlgItem(hwnd,IDOK),true);
                                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON6),true);
                                }
                            }
                            break;
                    }
                    break;
            }
            break;
        case WM_CLOSE:
            SendMessage(hwnd, WM_COMMAND, IDCANCEL, 0);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

//////////////////////////////////////
//
//  USER REGISTRATION DIALOG PROCEDURE
//
//////////////////////////////////////

BOOL CALLBACK DlgProc6(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    NOTER_CREDENTIALS tempCredentials;
    std::string tempUserName, tempPassword, tempSecPassword, tempString;
    long int result;
    switch(msg) {
        case WM_INITDIALOG:
            EnableWindow(GetDlgItem(hwnd,IDOK),false);
            editsChanged2=false;
            break;
        case WM_COMMAND:
            switch(wParam) {
                case IDOK:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDOK))) {
                        break;
                    }
                    GetWindowText(GetDlgItem(hwnd,IDC_EDIT6),buffer,65535);
                    tempUserName=buffer;
                    GetWindowText(GetDlgItem(hwnd,IDC_EDIT7),buffer,65535);
                    tempPassword=buffer;
                    GetWindowText(GetDlgItem(hwnd,IDC_EDIT8),buffer,65535);
                    tempSecPassword=buffer;
                    if(tempPassword==tempSecPassword) {
                        tempCredentials.username=tempUserName;
                        tempCredentials.password=tempPassword;
                        userEdit_LockAllButtons(hwnd);
                        result=noter_registerUser(connectionSettings,tempCredentials,buffer);
                        userEdit_UnlockAllButtons(hwnd);
                        if(result>=0) {
                            auxCredentials->username=tempCredentials.username;
                            auxCredentials->password=tempCredentials.password;
                            MessageBox(hwnd,STRING_MSG_REGISTRATION_SUCC,APPNAME,MB_ICONINFORMATION | MB_OK);
                            EndDialog(hwnd,IDOK);
                        }
                        else {
                            tempString=STRING_MSG_REGISTRATION_ERROR+noter_getAnswerString(result);
                            MessageBox(hwnd,(char*)tempString.c_str(),APPNAME,MB_ICONEXCLAMATION | MB_OK);
                        }
                    }
                    else {
                        MessageBox(hwnd,STRING_MSG_PASSWORDS_NO_MATCH,APPNAME,MB_ICONEXCLAMATION | MB_OK);
                    }
                    break;
                case IDCANCEL:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDCANCEL))) {
                        break;
                    }
                    editsChanged2=false;
                    EndDialog(hwnd,IDCANCEL);
                    break;
                case IDC_EDIT6:
                case IDC_EDIT7:
                case IDC_EDIT8:
                    switch(HIWORD(lParam)) {
                        case EN_CHANGE:
                            editsChanged2=true;
                            if(GetWindowTextLength(GetDlgItem(hwnd,wParam))==0) {
                                EnableWindow(GetDlgItem(hwnd,IDOK),false);
                            }
                            else {
                                if(GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT6))>0
                                    && GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT7))>0
                                    && GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT8))>0)
                                {
                                    EnableWindow(GetDlgItem(hwnd,IDOK),true);
                                }
                            }
                            break;
                    }
                    break;
                            
            }
            break;
        case WM_CLOSE:
            if(!IsWindowEnabled(GetDlgItem(hwnd,IDCANCEL))) {
                break;
            }
            editsChanged2=false;
            EndDialog(hwnd,IDCANCEL);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

//////////////////////////////////////
//
//  PASSWORD CONFIRMATION DIALOG PROCEDURE
//
//////////////////////////////////////

BOOL CALLBACK DlgProc7(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    long int result;
    std::string tempSecPassword, tempString;
    switch(msg) {
        case WM_INITDIALOG:
            EnableWindow(GetDlgItem(hwnd,IDOK),false);
            break;
        case WM_COMMAND:
            switch(wParam) {
                case IDOK:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDOK))) {
                        break;
                    }
                    GetWindowText(GetDlgItem(hwnd,IDC_EDIT12),buffer,65535);
                    tempSecPassword=buffer;
                    if(tempSecPassword==auxCredentials->password) {
                        result=noter_removeUser(connectionSettings,*auxCredentials,buffer);
                        if(result>=0) {
                            tempString=STRING_MSG_USER_SPACED+auxCredentials->username+STRING_MSG_USER_DELETED_SPACED;
                            MessageBox(hwnd,(char*)tempString.c_str(),APPNAME,MB_ICONINFORMATION | MB_OK);
                            EndDialog(hwnd,IDOK);
                        }
                        else {
                            MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),APPNAME,MB_ICONEXCLAMATION | MB_OK);
                        }
                    }
                    else {
                        MessageBox(hwnd,STRING_MSG_WRONG_PASSWORD,APPNAME,MB_ICONEXCLAMATION | MB_OK);
                    }
                    break;
                case IDCANCEL:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDCANCEL))) {
                        break;
                    }
                    EndDialog(hwnd,IDCANCEL);
                    break;
                case IDC_EDIT12:
                    switch(HIWORD(lParam)) {
                        case EN_CHANGE:
                            if(GetWindowTextLength(GetDlgItem(hwnd,wParam))==0) {
                                EnableWindow(GetDlgItem(hwnd,IDOK),false);
                            }
                            else {
                                EnableWindow(GetDlgItem(hwnd,IDOK),true);
                            }
                    }
                    break;
            }
            break;
        case WM_CLOSE:
            if(!IsWindowEnabled(GetDlgItem(hwnd,IDCANCEL))) {
                break;
            }
            EndDialog(hwnd,IDCANCEL);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

//////////////////////////////////////
//
//  PASSWORD CHANGE DIALOG PROCEDURE
//
//////////////////////////////////////

BOOL CALLBACK DlgProc8(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    std::string tempOldPassword, tempNewPassword, tempSecNewPassword;
    long int result;
    switch(msg) {
        case WM_INITDIALOG:
            EnableWindow(GetDlgItem(hwnd,IDOK),false);
            editsChanged2=false;
            break;
        case WM_COMMAND:
            switch(wParam) {
                case IDOK:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDOK))) {
                        break;
                    }
                    GetWindowText(GetDlgItem(hwnd,IDC_EDIT9),buffer,65535);
                    tempOldPassword=buffer;
                    GetWindowText(GetDlgItem(hwnd,IDC_EDIT10),buffer,65535);
                    tempNewPassword=buffer;
                    GetWindowText(GetDlgItem(hwnd,IDC_EDIT11),buffer,65535);
                    tempSecNewPassword=buffer;
                    if(tempNewPassword==tempSecNewPassword) {
                        if(tempOldPassword==auxCredentials->password) {
                            userEdit_LockAllButtons(hwnd);
                            result=noter_changeUserPassword(connectionSettings,*auxCredentials,(char*)tempNewPassword.c_str(),buffer);
                            userEdit_UnlockAllButtons(hwnd);
                            if(result>=0) {
                                auxCredentials->password=tempNewPassword;
                                MessageBox(hwnd,STRING_MSG_PASSWORD_CHANGED,APPNAME,MB_ICONINFORMATION | MB_OK);
                                EndDialog(hwnd,IDOK);
                            }
                            else {
                                MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),APPNAME,MB_ICONEXCLAMATION | MB_OK);
                            }
                        }
                        else {
                            MessageBox(hwnd,STRING_MSG_WRONG_PASSWORD,APPNAME,MB_ICONEXCLAMATION | MB_OK);
                        }
                    }
                    else {
                        MessageBox(hwnd,STRING_MSG_PASSWORDS_NO_MATCH,APPNAME,MB_ICONEXCLAMATION | MB_OK);
                    }
                    break;
                case IDCANCEL:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDCANCEL))) {
                        break;
                    }
                    editsChanged2=false;
                    EndDialog(hwnd,IDCANCEL);
                    break;
                case IDC_EDIT9:
                case IDC_EDIT10:
                case IDC_EDIT11:
                    switch(HIWORD(lParam)) {
                        case EN_CHANGE:
                            editsChanged2=true;
                            if(GetWindowTextLength(GetDlgItem(hwnd,wParam))==0) {
                                EnableWindow(GetDlgItem(hwnd,IDOK),false);
                            }
                            else {
                                if(GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT9))>0
                                    && GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT10))>0
                                    && GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT11))>0)
                                {
                                    EnableWindow(GetDlgItem(hwnd,IDOK),true);
                                }
                            }
                            break;
                    }
                    break;
            }
            break;
        case WM_CLOSE:
            if(!IsWindowEnabled(GetDlgItem(hwnd,IDCANCEL))) {
                break;
            }
            editsChanged2=false;
            EndDialog(hwnd,IDCANCEL);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}
