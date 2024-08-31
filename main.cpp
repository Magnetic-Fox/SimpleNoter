#include <windows.h>
#include <ctl3d.h>
#include <string>
#include <map>

#include "resources.h"

#ifdef NOTER_DEBUG
    #include "debug.hpp"
#endif

#include "helpers.hpp"
#include "libutil.hpp"
#include "wsprocs.hpp"
#include "noterapi.hpp"
#include "winconst.hpp"
#include "codepages.hpp"
#include "constants.hpp"
#include "responses.hpp"
#include "additional.hpp"
#include "conversion.hpp"
#include "definitions.hpp"
#include "inihandling.hpp"

//////////////////////////////////////
//
//  GLOBAL VARIABLES
//
//////////////////////////////////////

// Windows types
HBRUSH                    g_hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
HBRUSH                    g_hBrush2= CreateSolidBrush(GetSysColor(COLOR_ACTIVECAPTION));
HWND                      g_hwnd;
HINSTANCE                 g_hInstance=NULL, hCodePageLib=NULL;
HGLOBAL                   hCodePageDefinition=NULL;
MSG                       *g_Msg;

// Own types
WINDOWMEMORY              winMem;
RAWCODEPAGE               rawCodePage;
CODEPAGE                  mappedCodePage;
NOTER_CONNECTION_SETTINGS connectionSettings;
NOTER_CREDENTIALS         credentials, tempCredentials, *auxCredentials;
MAINSETTINGS              mainSettings;
NOTE_SUMMARY              *notes=NULL;
LIBRARIES                 libraries;

// Standard types
long int                  noteCount=0;
long int                  mainLastResult=0;
unsigned int              ctlRegs=0;
bool                      check3DChanged, editsChanged, editsChanged2, useTestCredentials, firstOptions=false, codePageChanged;
char                      buffer[65536];

//////////////////////////////////////
//
//  PROTOTYPES
//
//////////////////////////////////////

// Additional procedures
HWND createEditWindow           (HWND, WINDOWMEMORY&, NOTE*);
ATOM registerMainWindowClass    (WNDCLASS*);
ATOM registerEditWindowClass    (WNDCLASS*);
void freeGlobalResources        (void);

// Main window procedures
LRESULT CALLBACK MainWndProc    (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditWndProc    (HWND, UINT, WPARAM, LPARAM);

// Dialog procedures
BOOL CALLBACK NotePropDlgProc   (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AboutDlgProc      (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK PreferencesDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ConnSettDlgProc   (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK CredsSettDlgProc  (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK UserRegDlgProc    (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK PassConfirmDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK PassChangeDlgProc (HWND, UINT, WPARAM, LPARAM);

BOOL CALLBACK NotesExpDlgProc   (HWND, UINT, WPARAM, LPARAM);

//////////////////////////////////////
//
//  EDIT WINDOW
//
//////////////////////////////////////

HWND createEditWindow(HWND hwnd, WINDOWMEMORY &winMem, NOTE *note) {
    EDITWINDOW *editWin = new EDITWINDOW;

    makeEditWindowTitle(editWin,note,false,mappedCodePage);
    
    editWin->hwnd =CreateWindow(NOTER_EDITWINDOW, editWin->windowTitle.c_str(), WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, g_hInstance, NULL);

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
        
        editWin->hStatic = CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_EDITWIN_TITLE), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        0, 0, 600, 16, editWin->hwnd, (HMENU)ID_EDIT_STATIC1, g_hInstance, NULL);

        editWin->hEditBox= CreateWindow(WC_EDIT, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                                        0, 16, 600, 24, editWin->hwnd, (HMENU)ID_EDIT_EDITBOX1, g_hInstance, NULL);

        editWin->hStatic2= CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_EDITWIN_ENTRY), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        0, 40, 600, 16, editWin->hwnd, (HMENU)ID_EDIT_STATIC2, g_hInstance, NULL);

        editWin->hEditBox2=CreateWindow(WC_EDIT, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
                                        0, 56, 600, 300, editWin->hwnd, (HMENU)ID_EDIT_EDITBOX2, g_hInstance, NULL);

        if(editWin->note->id==0) {
            editWin->hButton = CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_ADD), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                                            0, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON1, g_hInstance, NULL);
        }
        else {
            editWin->hButton = CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_UPDATE), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                                            0, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON1, g_hInstance, NULL);
        }

        if(editWin->note->id==0) {
            editWin->hButton2 =CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_PROPERTIES), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                                            96, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON2, g_hInstance, NULL);
        }
        else {
            editWin->hButton2 =CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_PROPERTIES), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                            96, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON2, g_hInstance, NULL);
        }

        editWin->hButton3 =CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_CLOSE), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                        192, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON3, g_hInstance, NULL);

        editWin->hStatic3 =CreateWindow(WC_STATIC, NULL, WS_CHILD | WS_VISIBLE | SS_GRAYRECT,
                                        288, 356, 312, 21, editWin->hwnd, (HMENU)ID_EDIT_STATIC3, g_hInstance, NULL);

        editWin->hStatic4= CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_INFO_OK), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        0, 377, 600, 16, editWin->hwnd, (HMENU)ID_EDIT_STATIC4, g_hInstance, NULL);

        bool warningState=false;
        if(editWin->note->id>0) {
            SetWindowText(editWin->hEditBox, toCodePage(mappedCodePage,(char*)editWin->note->subject.c_str()).c_str());
            warningState=decodeWarningState();
            SetWindowText(editWin->hEditBox2,toCodePage(mappedCodePage,(char*)editWin->note->entry.c_str()).c_str());
            warningState=(warningState || decodeWarningState());
        }

        editWin->subjectChanged=false;
        editWin->entryChanged=  false;
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
            MessageBox(editWin->hwnd,getStringFromTable(IDS_STRING_MSG_UNSUPPORTED_CHARS),getStringFromTable(IDS_APPNAME,1),MB_ICONINFORMATION);
        }

        return editWin->hwnd;
    }
}

//////////////////////////////////////
//
//  ADDITIONAL PROCEDURES
//
//////////////////////////////////////

ATOM registerMainWindowClass(WNDCLASS *wc) {
    wc->style=          0;
    wc->lpfnWndProc=    MainWndProc;
    wc->cbClsExtra=     0;
    wc->cbWndExtra=     0;
    wc->hInstance=      g_hInstance;
    wc->hIcon=          LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc->hCursor=        LoadCursor(NULL, IDC_ARROW);
    wc->hbrBackground=  g_hBrush;
    wc->lpszMenuName=   MAKEINTRESOURCE(IDR_MENU1);
    wc->lpszClassName=  NOTER_MAINWINDOW;
    return RegisterClass(wc);
}

ATOM registerEditWindowClass(WNDCLASS *wc) {
    wc->style=          0;
    wc->lpfnWndProc=    EditWndProc;
    wc->cbClsExtra=     0;
    wc->cbWndExtra=     0;
    wc->hInstance=      g_hInstance;
    wc->hIcon=          LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON2));
    wc->hCursor=        LoadCursor(NULL, IDC_ARROW);
    wc->hbrBackground=  g_hBrush;
    wc->lpszMenuName=   MAKEINTRESOURCE(IDR_MENU2);
    wc->lpszClassName=  NOTER_EDITWINDOW;
    return RegisterClass(wc);
}

void freeGlobalResources(void) {
    WSACleanup();
    DeleteObject(g_hBrush);
    DeleteObject(g_hBrush2);
    unloadCodePage(hCodePageLib,hCodePageDefinition);
    return;
}

//////////////////////////////////////
//
//  MAIN PROGRAM
//
//////////////////////////////////////

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInstance=hInstance;

    storeStringTableInstance(hInstance);
    storeConnectionSettingsReference(&connectionSettings);
    storeCredentialsReference(&credentials);
    storeGlobalHWNDReference(&g_hwnd);
    storeWindowMemoryReference(&winMem);

    WNDCLASS wc = { 0 };
    WNDCLASS wc2= { 0 };

    HWND     hwnd;
    HWND     hButton, hButton2, hButton3, hButton4, hButton5;
    HWND     hListBox;
    HWND     hStatic, hStatic2, hStatic3, hStatic4, hStatic5, hStatic6;
    HWND     temp;

    HACCEL   hAccel;
    MSG      msg;

    // make global "reference" to the message variable
    g_Msg=&msg;
    
    GetModuleFileName(hInstance,buffer,32767);
    listAvailableLibs(buffer,libraries);
    std::string iniFile=getDefaultIniFile(buffer);
    connectionSettings=getConnectionSettings((char*)iniFile.c_str());
    credentials=getCredentials((char*)iniFile.c_str());
    mainSettings=getMainSettings((char*)iniFile.c_str(),&libraries);

    if(wsInit() == SOCKET_ERROR) {
        MessageBox(NULL,getStringFromTable(IDS_STRING_MSG_WINSOCK_ERROR),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONSTOP | MB_OK);
        freeGlobalResources();
        return 1;
    }

    if(!loadAndPrepareCodePage(mainSettings,libraries,hCodePageLib,hCodePageDefinition,rawCodePage,mappedCodePage)) {
        MessageBox(NULL,getStringFromTable(IDS_STRING_MSG_CODEPAGE_ERROR),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONSTOP | MB_OK);
        freeGlobalResources();
        return 1;
    }

    if((!registerMainWindowClass(&wc)) || (!registerEditWindowClass(&wc2))) {
        MessageBox(NULL,getStringFromTable(IDS_STRING_MSG_WNDCLASS_ERROR),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONSTOP | MB_OK);
        freeGlobalResources();
        return 1;
    }
    
    hwnd = CreateWindow(NOTER_MAINWINDOW, getStringFromTable(IDS_APPNAME), WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        NULL, NULL, hInstance, NULL);

    if(hwnd == NULL) {
        MessageBox(NULL,getStringFromTable(IDS_STRING_MSG_WND_CREATE_ERROR),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONSTOP | MB_OK);
        freeGlobalResources();
        return 1;
    }
    else {
        g_hwnd=hwnd;
    }

    hAccel=LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));
    if(!hAccel) {
        MessageBox(NULL,getStringFromTable(IDS_STRING_MSG_ACCELERATORS_ERROR),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONSTOP | MB_OK);
        freeGlobalResources();
        return 1;
    }

    if(mainSettings.savePosSizes) {
        if((mainSettings.mainWindowX!=CW_USEDEFAULT) && (mainSettings.mainWindowY!=CW_USEDEFAULT) && (mainSettings.mainWindowSizeX!=CW_USEDEFAULT) && (mainSettings.mainWindowSizeY!=CW_USEDEFAULT)) {
            SetWindowPos(hwnd,NULL,mainSettings.mainWindowX,mainSettings.mainWindowY,mainSettings.mainWindowSizeX,mainSettings.mainWindowSizeY,SWP_NOZORDER);
        }
    }

    hButton =  CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_DOWNLOAD), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                            0, 0, 80, 21, hwnd, (HMENU)ID_BUTTON1, hInstance, NULL);

    hButton2 = CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_CREATE), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                            80, 0, 80, 21, hwnd, (HMENU)ID_BUTTON2, hInstance, NULL);

    hButton3 = CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_OPEN), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                            160, 0, 80, 21, hwnd, (HMENU)ID_BUTTON3, hInstance, NULL);

    hButton5 = CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_DELETE), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                            240, 0, 80, 21, hwnd, (HMENU)ID_BUTTON5, hInstance, NULL);

    hButton4 = CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_EXIT), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                            320, 0, 80, 21, hwnd, (HMENU)ID_BUTTON4, hInstance, NULL);

    hStatic6 = CreateWindow(WC_STATIC, NULL, WS_CHILD | WS_VISIBLE | SS_GRAYRECT,
                            400, 0, 200, 21, hwnd, (HMENU)ID_STATIC6, hInstance, NULL);

    hListBox = CreateWindow(WC_LISTBOX, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
                            WS_VSCROLL | WS_TABSTOP | ES_AUTOVSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_EXTENDEDSEL,
                            0, 21, 600, 300, hwnd, (HMENU)ID_LISTBOX, hInstance, NULL);

    if(mainSettings.use3DLists) {
        SetWindowPos(hListBox,NULL,0,22,600,298,SWP_NOZORDER);
    }
    else {
        SetWindowPos(hListBox,NULL,0,21,600,300,SWP_NOZORDER);
    }

    hStatic  = CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_ID), WS_CHILD | WS_VISIBLE | SS_LEFT,
                            8, 329, 128, 16, hwnd, (HMENU)ID_STATIC1, hInstance, NULL);
                           
    hStatic2 = CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_LAST_CHANGED), WS_CHILD | WS_VISIBLE | SS_LEFT,
                            8, 346, 128, 16, hwnd, (HMENU)ID_STATIC2, hInstance, NULL);
                           
    hStatic3 = CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_NOT_CHOSEN), WS_CHILD | WS_VISIBLE | SS_LEFT,
                            137, 329, 454, 16, hwnd, (HMENU)ID_STATIC3, hInstance, NULL);
                           
    hStatic4 = CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_NOT_CHOSEN), WS_CHILD | WS_VISIBLE | SS_LEFT,
                            137, 346, 454, 16, hwnd, (HMENU)ID_STATIC4, hInstance, NULL);
                           
    hStatic5 = CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_INFO_OK), WS_CHILD | WS_VISIBLE | SS_LEFT,
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
            MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_CTL3D_ERROR),getStringFromTable(IDS_STRING_WARNING,1),MB_ICONEXCLAMATION | MB_OK);
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

    while(GetMessage(&msg, NULL, 0, 0)) {
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

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    std::string tempString="";
    long int index;
    long int result;
    unsigned long int width;
    unsigned long int height;
    int x, y, size_x, size_y;
    unsigned int state, compressionRatio, count, errorCount;
    unsigned int *selection=NULL;
    bool atLeastOneDeleted;
    MINMAXINFO *lpMMI;
    
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
            count=SendMessage(GetDlgItem(hwnd,ID_LISTBOX),      LB_GETSELCOUNT, 0, 0);
            EnableMenuItem(GetMenu(hwnd),ID_FILE_RELOAD,        IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON1))                                                            ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_FILE_OPEN,          IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON3))                                                            ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_FILE_EXIT,          IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON4))                                                            ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_FILE_DELETE,        IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON5))                                                            ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_FILE_IMPORT,        (noter_connectionSettingsAvailable(connectionSettings) && IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON4))) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_FILE_EXPORT,        ((count>0) && IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON4)))                                             ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_EDIT_SELECTALL,     (count!=noteCount)                                                                                      ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_OPTIONS_CONNECTION, IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON4))                                                            ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_OPTIONS_CREDENTIALS,(noter_connectionSettingsAvailable(connectionSettings) && IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON4))) ? MF_ENABLED : MF_GRAYED);
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
                case ID_ACC_DEL:
                    SendMessage(hwnd, WM_COMMAND, ID_BUTTON5, 0);
                    break;
                case ID_ACC_CTRLI:
                    SendMessage(hwnd, WM_COMMAND, ID_FILE_IMPORT, 0);
                    break;
                case ID_ACC_CTRLE:
                    SendMessage(hwnd, WM_COMMAND, ID_FILE_EXPORT, 0);
                    break;
                case ID_ACC_ALTF4:
                    SendMessage(hwnd, WM_COMMAND, ID_BUTTON4, 0);
                    break;
                case ID_ACC_CTRLA:
                    SendMessage(hwnd, WM_COMMAND, ID_EDIT_SELECTALL, 0);
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
                case ID_FILE_IMPORT:
                    if(IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON4))) {
                        // TODO: Import section
                    }
                    break;
                case ID_FILE_EXPORT:
                    if((SendMessage(GetDlgItem(hwnd,ID_LISTBOX),LB_GETSELCOUNT,0,0)>0) && IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON4))) {
                        MakeDialogBox(hwnd,IDD_DIALOG10,NotesExpDlgProc);
                    }
                    break;
                case ID_FILE_EXIT:
                    SendMessage(hwnd, WM_COMMAND, ID_BUTTON4, 0);
                    break;
                case ID_EDIT_SELECTALL:
                    if(SendMessage(GetDlgItem(hwnd,ID_LISTBOX),LB_GETSELCOUNT,0,0)!=noteCount) {
                        SendMessage(GetDlgItem(hwnd,ID_LISTBOX),LB_SETSEL,TRUE,-1);
                    }
                    break;
                case ID_OPTIONS_PREFERENCES:
                    if((MakeDialogBox(hwnd,IDD_DIALOG3,PreferencesDlgProc)==IDOK) && (codePageChanged)) {
                        if(MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WANT_RELOAD),getStringFromTable(IDS_APPNAME,1),MB_ICONQUESTION | MB_YESNO)==IDYES) {
                            SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_SETSEL, FALSE, -1);
                            SendMessage(hwnd, WM_COMMAND, ID_BUTTON1, ID_FILE_RELOAD);
                        }
                    }
                    break;
                case ID_OPTIONS_CONNECTION:
                    if((MakeDialogBox(hwnd,IDD_DIALOG4,ConnSettDlgProc)==IDOK) && (editsChanged)) {
                        if((!firstOptions) && (noter_credentialsAvailable(credentials)) && (MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WANT_RELOAD),getStringFromTable(IDS_APPNAME,1),MB_ICONQUESTION | MB_YESNO)==IDYES)) {
                            SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_SETSEL, FALSE, -1);
                            SendMessage(hwnd, WM_COMMAND, ID_BUTTON1, ID_FILE_RELOAD);
                        }
                    }
                    break;
                case ID_OPTIONS_CREDENTIALS:
                    if((MakeDialogBox(hwnd,IDD_DIALOG5,CredsSettDlgProc)==IDOK) && (editsChanged || firstOptions)) {
                        if(SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCOUNT, 0, 0)>0) {
                            SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_SETSEL, FALSE, -1);
                            SendMessage(hwnd, WM_COMMAND, ID_BUTTON1, ID_FILE_RELOAD);
                        }
                        else {
                            if(MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WANT_RELOAD),getStringFromTable(IDS_APPNAME,1),MB_ICONQUESTION | MB_YESNO)==IDYES) {
                                SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_SETSEL, FALSE, -1);
                                SendMessage(hwnd, WM_COMMAND, ID_BUTTON1, ID_FILE_RELOAD);
                            }
                            if(firstOptions) {
                                firstOptions=false;
                            }
                        }
                    }
                    break;
                case ID_HELP_HELP:
                    WinHelp(g_hwnd,getStringFromTable(IDS_HELPFILE),HELP_CONTENTS,0);
                    break;
                case ID_HELP_HOWTO:
                    WinHelp(g_hwnd,"",HELP_HELPONHELP,0);
                    break;
                case ID_HELP_ABOUT:
                    MakeDialogBox(hwnd,IDD_DIALOG2,AboutDlgProc);
                    break;
                // Refresh button
                case ID_BUTTON1:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON1))) {
                        break;
                    }
                    main_LockAllButtons(hwnd);
                    SetWindowText(GetDlgItem(hwnd,ID_STATIC5),getStringFromTable(IDS_STRING_LOADING_NOTE_LIST));
                    if(noteCount>0) {
                        freeNoteList(notes);
                    }
                    noteCount=noter_getNoteList(connectionSettings,credentials,buffer,notes);
                    count=getSelection(GetDlgItem(hwnd,ID_LISTBOX),selection);
                    SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_RESETCONTENT, 0, 0);
                    for(long int x=0; x<noteCount; ++x) {
                        SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_ADDSTRING, 0, (LPARAM)toCodePage(mappedCodePage,(char*)notes[x].subject.c_str()).c_str());
                    }
                    if(count>0) {
                        if(lParam==0) {
                            SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_SETSEL, TRUE, 0);
                        }
                        else {
                            setSelection(GetDlgItem(hwnd,ID_LISTBOX),selection,count);
                        }
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC3),IntToStr(notes[selection[0]].id).c_str());
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC4),notes[selection[0]].lastModified.c_str());
                    }
                    else {
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC3),getStringFromTable(IDS_STRING_NOT_CHOSEN));
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC4),getStringFromTable(IDS_STRING_NOT_CHOSEN));
                        EnableWindow(GetDlgItem(hwnd,ID_BUTTON3), false);
                        EnableWindow(GetDlgItem(hwnd,ID_BUTTON5), false);
                    }
                    if(noteCount>=0) {
                        mainLastResult=INFO_LIST_SUCCESSFUL;
                        if(lParam!=0) {
                            tempString=noter_getAnswerString(mainLastResult)+(std::string)getStringFromTable(IDS_STRING_SPACED_COUNT)+IntToStr(noteCount)+".";
                            compressionRatio=getCompressionRatio();
                            if(compressionRatio!=100) {
                                tempString=tempString+(std::string)getStringFromTable(IDS_STRING_SPACED_COMPRESSION)+IntToStr(getCompressionRatio())+"%.";
                            }
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)tempString.c_str());
                        } else {
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC5),getStringFromTable(IDS_STRING_INFO_OK));
                        }
                    }
                    else {
                        mainLastResult=noteCount;
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                        EnableWindow(GetDlgItem(hwnd,ID_BUTTON3), false);
                        EnableWindow(GetDlgItem(hwnd,ID_BUTTON5), false);
                        noteCount=0;
                    }
                    main_UnlockAllButtons(hwnd);
                    if(count==0) {
                        EnableWindow(GetDlgItem(hwnd,ID_BUTTON3), false);
                        EnableWindow(GetDlgItem(hwnd,ID_BUTTON5), false);
                    }
                    freeSelectionBuffer(selection);
                    break;
                // Create button
                case ID_BUTTON2:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON2))) {
                        break;
                    }
                    if(mainLastResult!=0) {
                        mainLastResult=0;
                    }
                    SetWindowText(GetDlgItem(hwnd,ID_STATIC5),getStringFromTable(IDS_STRING_CREATING_EDIT_WINDOW));
                    if(createEditWindow(hwnd,winMem,NULL)==NULL) {
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),getStringFromTable(IDS_STRING_EDITWIN_CREATE_ERROR));
                        mainLastResult=-2048;
                    }
                    else {
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(INFO_OK).c_str());
                    }
                    break;
                // Open button
                case ID_BUTTON3:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON3))) {
                        break;
                    }
                    count=getSelection(GetDlgItem(hwnd,ID_LISTBOX),selection);
                    errorCount=0;
                    if((count>=10) && (MessageBox(hwnd,(char*)(getStringFromTable(IDS_STRING_MSG_LIKE_TO_OPEN_SPACED)+IntToStr(count)+(std::string)getStringFromTable(IDS_STRING_MSG_NOTES_SPACED)).c_str(),getStringFromTable(IDS_APPNAME,1),MB_ICONQUESTION | MB_YESNO)==IDNO)) {
                        count=0;
                        freeSelectionBuffer(selection);
                    }
                    if(count>0) {
                        main_LockAllButtons(hwnd);
                        if(mainLastResult!=0) {
                            mainLastResult=0;
                        }
                        for(unsigned int x=0; x<count; ++x) {
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC5),getStringFromTable(IDS_STRING_DOWNLOADING_NOTE));
                            index=selection[x];
                            if(index>=0) {
                                NOTE *note=new NOTE;
                                result=noter_getNote(connectionSettings,credentials,notes[index].id,buffer,*note);
                                if(result>=0) {
                                    HWND tempHwnd=createEditWindow(hwnd,winMem,note);
                                    if(tempHwnd!=NULL) {
                                        tempString=noter_getAnswerString(result)+(std::string)getStringFromTable(IDS_STRING_SPACED_LAST_MOD_DATE)+toCodePage(mappedCodePage,(char*)note->lastModified.c_str())+".";
                                        compressionRatio=getCompressionRatio();
                                        if(compressionRatio!=100) {
                                            tempString=tempString+(std::string)getStringFromTable(IDS_STRING_SPACED_COMPRESSION)+IntToStr(getCompressionRatio())+"%.";
                                        }
                                        SetWindowText(GetDlgItem(tempHwnd,ID_EDIT_STATIC4),(char*)tempString.c_str());
                                        winMem[tempHwnd]->lastResult=result;
                                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(INFO_OK).c_str());
                                    }
                                    else {
                                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),getStringFromTable(IDS_STRING_EDITWIN_CREATE_ERROR));
                                        mainLastResult=-2048;
                                        ++errorCount;
                                    }
                                }
                                else {
                                    mainLastResult=result;
                                    SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                                    ++errorCount;
                                }
                            }
                        }
                        main_UnlockAllButtons(hwnd);
                        freeSelectionBuffer(selection);
                        if(errorCount>1) {
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC5),getStringFromTable(IDS_STRING_NOT_ALL_NOTES_LOADED));
                        }
                    }
                    break;
                // Close button
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
                // Remove button
                case ID_BUTTON5:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON5))) {
                        break;
                    }
                    if(mainLastResult!=0) {
                        mainLastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                    }
                    count=getSelection(GetDlgItem(hwnd,ID_LISTBOX),selection);
                    if(count==1) {
                        tempString=(std::string)getStringFromTable(IDS_STRING_MSG_WANT_NOTE_REMOVAL);
                        tempString=tempString+" \"";
                        tempString=tempString+toCodePage(mappedCodePage,(char*)notes[selection[0]].subject.c_str());
                        tempString=tempString+"\"?";
                    }
                    else {
                        tempString=(std::string)getStringFromTable(IDS_STRING_MSG_WANT_NOTES_REMOVAL);
                    }
                    errorCount=0;
                    atLeastOneDeleted=false;
                    if(MessageBox(hwnd,(char*)tempString.c_str(),getStringFromTable(IDS_APPNAME,1),MB_ICONQUESTION | MB_YESNO)==IDYES) {
                        main_LockAllButtons(hwnd);
                        for(unsigned int x=0; x<count; ++x) {
                            SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_SETSEL, FALSE, selection[x]);
                            mainLastResult=noter_deleteNote(connectionSettings,credentials,notes[selection[x]].id,buffer);
                            if(mainLastResult==INFO_NOTE_DELETED) {
                                selection[x]=0;
                                if(!atLeastOneDeleted) {
                                    atLeastOneDeleted=true;
                                }
                            }
                            else {
                                selection[x]=notes[selection[x]].id;
                                ++errorCount;
                            }
                        }
                        main_UnlockAllButtons(hwnd);
                        if(atLeastOneDeleted) {
                            SendMessage(hwnd,WM_COMMAND,ID_BUTTON1,0);
                        }
                        if((errorCount==0) || (errorCount==count)) {
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)noter_getAnswerString(mainLastResult).c_str());
                            if(errorCount>0) {
                                selectIndexes(GetDlgItem(hwnd,ID_LISTBOX),selection,count,notes,noteCount);
                            }
                        }
                        else {
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC5),getStringFromTable(IDS_STRING_NOT_ALL_NOTES_REMOVED));
                            selectIndexes(GetDlgItem(hwnd,ID_LISTBOX),selection,count,notes,noteCount);
                            if(SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETSELCOUNT, 0, 0)>0) {
                                EnableWindow(GetDlgItem(hwnd,ID_BUTTON3), true);
                                EnableWindow(GetDlgItem(hwnd,ID_BUTTON5), true);
                            }
                        }
                    }
                    freeSelectionBuffer(selection);
                    break;
                case ID_LISTBOX:
                    switch(HIWORD(lParam)) {
                        case LBN_DBLCLK:
                            SendMessage(hwnd, WM_COMMAND, ID_BUTTON3, 0);
                            break;
                        case LBN_SELCHANGE:
                            count=SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETSELCOUNT, 0, 0);
                            if(count==0) {
                                SetWindowText(GetDlgItem(hwnd,ID_STATIC3),getStringFromTable(IDS_STRING_NOT_CHOSEN));
                                SetWindowText(GetDlgItem(hwnd,ID_STATIC4),getStringFromTable(IDS_STRING_NOT_CHOSEN));
                                EnableWindow(GetDlgItem(hwnd,ID_BUTTON3), false);
                                EnableWindow(GetDlgItem(hwnd,ID_BUTTON5), false);
                            }
                            else {
                                index=SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0);
                                if(IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON4))) {
                                    SetWindowText(GetDlgItem(hwnd,ID_STATIC3),IntToStr(notes[index].id).c_str());
                                    SetWindowText(GetDlgItem(hwnd,ID_STATIC4),notes[index].lastModified.c_str());
                                    EnableWindow(GetDlgItem(hwnd,ID_BUTTON3), true);
                                    EnableWindow(GetDlgItem(hwnd,ID_BUTTON5), true);
                                }
                            }
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
                SetWindowPos(GetDlgItem(hwnd,ID_LISTBOX),NULL,0,0,width,height-88,      SWP_NOZORDER | SWP_NOMOVE);
            }
            else {
                SetWindowPos(GetDlgItem(hwnd,ID_LISTBOX),NULL,0,0,width,height-86,      SWP_NOZORDER | SWP_NOMOVE);
            }
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC6),NULL,0,0,width-400,21,             SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC1),NULL,8,height-57,0,0,              SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC2),NULL,8,height-40,0,0,              SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC3),NULL,137,height-57,width-146,16,   SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC4),NULL,137,height-40,width-146,16,   SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd,ID_STATIC5),NULL,0,height-16,width,16,         SWP_NOZORDER);

            break;
        case WM_GETMINMAXINFO:
            lpMMI=(MINMAXINFO*)lParam;
            lpMMI->ptMinTrackSize.x=480;
            lpMMI->ptMinTrackSize.y=320;
            break;
        case WM_CLOSE:
            if(IsWindowEnabled(GetDlgItem(hwnd,ID_BUTTON4))) {
                if(winMem.size()>0) {
                    for(WINDOWMEMORY::iterator it=winMem.begin(); it!=winMem.end(); ++it) {
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

                    GetModuleFileName(g_hInstance,buffer,32767);
                    saveWindowCoordinatesSettings(mainSettings,(char*)getDefaultIniFile(buffer).c_str());
                }
                if(winMem.size()==0) {
                    DestroyWindow(hwnd);
                }
            }
            
            break;
        case WM_DESTROY:
            if(Ctl3dEnabled() && (!Ctl3dUnregister(g_hInstance))) {
                MessageBox(0,getStringFromTable(IDS_STRING_MSG_CTL3D_UNREG_ERROR),getStringFromTable(IDS_STRING_WARNING,1),MB_ICONEXCLAMATION);
            }
            if(noteCount>0) {
                freeNoteList(notes);
            }
            WinHelp(g_hwnd,"",HELP_QUIT,0);
            freeGlobalResources();
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

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    unsigned long int width;
    unsigned long int height;
    unsigned long int sel;
    unsigned int result;
    int x, y, size_x, size_y;
    unsigned int state;
    MINMAXINFO *lpMMI;
    
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
                EnableMenuItem(GetMenu(hwnd),   ID_FILE_PROPERTIES, MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_FILE_TONEWNOTE,  MF_GRAYED);
                ModifyMenu(GetMenu(hwnd),       ID_FILE_ADDUP,      MF_BYCOMMAND | MF_STRING,ID_FILE_ADDUP,getStringFromTable(IDS_STRING_MENU_ADD));
                EnableMenuItem(GetMenu(hwnd),   ID_FILE_ADDUP,      IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON1)) ? MF_ENABLED : MF_GRAYED);
            }
            else {
                EnableMenuItem(GetMenu(hwnd),   ID_FILE_PROPERTIES, IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON2)) ? MF_ENABLED : MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_FILE_TONEWNOTE,  MF_ENABLED);
                ModifyMenu(GetMenu(hwnd),       ID_FILE_ADDUP,      MF_BYCOMMAND | MF_STRING,ID_FILE_ADDUP,getStringFromTable(IDS_STRING_MENU_UPDATE));
                EnableMenuItem(GetMenu(hwnd),   ID_FILE_ADDUP,      IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON1)) ? MF_ENABLED : MF_GRAYED);
            }
            if((GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX1)) || (GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2))) {
                EnableMenuItem(GetMenu(hwnd),   ID_EDIT_UNDO,       SendMessage(GetFocus(), EM_CANUNDO, 0, 0) ? MF_ENABLED : MF_GRAYED);
                sel=SendMessage(GetFocus(),     EM_GETSEL,          0, 0);
                EnableMenuItem(GetMenu(hwnd),   ID_EDIT_CUT,        (HIWORD(sel)!=LOWORD(sel)) ? MF_ENABLED : MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EDIT_COPY,       (HIWORD(sel)!=LOWORD(sel)) ? MF_ENABLED : MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EDIT_CLEAR,      (HIWORD(sel)!=LOWORD(sel)) ? MF_ENABLED : MF_GRAYED);
                if(OpenClipboard(GetFocus())) {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_PASTE,     (IsClipboardFormatAvailable(CF_TEXT) || IsClipboardFormatAvailable(CF_OEMTEXT)) ? MF_ENABLED : MF_GRAYED);
                    CloseClipboard();
                }
                else {
                    EnableMenuItem(GetMenu(hwnd),ID_EDIT_PASTE,     MF_GRAYED);
                }
                EnableMenuItem(GetMenu(hwnd),   ID_EDIT_SELECTALL,  ((GetWindowTextLength(GetFocus())>0) && (GetWindowTextLength(GetFocus())>(HIWORD(sel)-LOWORD(sel)))) ? MF_ENABLED : MF_GRAYED);
            }
            else {
                EnableMenuItem(GetMenu(hwnd),   ID_EDIT_UNDO,       MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EDIT_CUT,        MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EDIT_COPY,       MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EDIT_PASTE,      MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EDIT_CLEAR,      MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EDIT_SELECTALL,  MF_GRAYED);
            }
            break;
        case WM_COMMAND:
            switch(wParam) {
                case ID_ACC_ENTER:
                case ID_ACC_DEL:
                    // yeah... some ugly cheats, right...?
                    // this cheats makes delete and return keys usable in the edit boxes after declaring them as an accelerators in the resources
                    processMessages(g_Msg);
                    break;
                case ID_ACC_TAB:
                    if(GetFocus()==GetDlgItem(hwnd,ID_EDIT_EDITBOX2)) {
                        processMessages(g_Msg);
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
                    EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1), true);
                    EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON2), false);
                    SetWindowText(GetDlgItem(hwnd,ID_EDIT_BUTTON1),getStringFromTable(IDS_STRING_ADD));
                    winMem[hwnd]->subjectChanged=true;
                    winMem[hwnd]->entryChanged=true;
                    makeEditWindowTitle(winMem[hwnd],NULL,true,mappedCodePage);
                    SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),getStringFromTable(IDS_STRING_TO_NEW_NOTE));
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
                    WinHelp(g_hwnd,getStringFromTable(IDS_HELPFILE),HELP_CONTENTS,0);
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
                    MakeDialogBox(hwnd,IDD_DIALOG2,AboutDlgProc);
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
                        edit_LockAllButtons(g_hwnd,hwnd);
                        winMem[hwnd]->lastResult=noter_addNote(connectionSettings,credentials,*winMem[hwnd]->note,buffer);
                        edit_UnlockAllButtons(hwnd);
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        if(winMem[hwnd]->lastResult>=0) {
                            if(mainSettings.autoRefresh) {
                                SendMessage(g_hwnd,WM_COMMAND,ID_BUTTON1,0);
                            }
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_BUTTON1),getStringFromTable(IDS_STRING_UPDATE));
                            winMem[hwnd]->subjectChanged=false;
                            winMem[hwnd]->entryChanged=  false;
                            makeEditWindowTitle(winMem[hwnd],winMem[hwnd]->note,true,mappedCodePage);
                        }
                        else {
                            EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),true);
                        }
                    }
                    else {
                        edit_LockAllButtons(g_hwnd,hwnd);
                        winMem[hwnd]->lastResult=noter_updateNote(connectionSettings,credentials,*winMem[hwnd]->note,buffer);
                        edit_UnlockAllButtons(hwnd);
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        if(winMem[hwnd]->lastResult>=0) {
                            if(mainSettings.autoRefresh) {
                                SendMessage(g_hwnd,WM_COMMAND,ID_BUTTON1,0);
                            }
                            winMem[hwnd]->subjectChanged=false;
                            winMem[hwnd]->entryChanged=  false;
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
                    MakeDialogBox(hwnd,IDD_DIALOG1,NotePropDlgProc);
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

            if(width<240) {
                width=240;
            }
            if(height<240) {
                height=240;
            }

            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_STATIC1),  NULL,0,0,width,16,              SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_EDITBOX1), NULL,0,0,width,24,              SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_STATIC2),  NULL,0,0,width,16,              SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_EDITBOX2), NULL,0,0,width,height-93,       SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_BUTTON1),  NULL,0,height-37,0,0,           SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_BUTTON2),  NULL,96,height-37,0,0,          SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_BUTTON3),  NULL,192,height-37,0,0,         SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_STATIC3),  NULL,288,height-37,width-288,21,SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd,ID_EDIT_STATIC4),  NULL,0,height-16,width,16,      SWP_NOZORDER);
            
            break;
        case WM_GETMINMAXINFO:
            lpMMI=(MINMAXINFO*)lParam;
            lpMMI->ptMinTrackSize.x=480;
            lpMMI->ptMinTrackSize.y=320;
            break;
        case WM_CLOSE:
            if((!IsWindowEnabled(GetDlgItem(g_hwnd,ID_BUTTON4))) || (!IsWindowEnabled(GetDlgItem(hwnd,ID_EDIT_BUTTON3)))) {
                break;
            }
            if((winMem[hwnd]!=NULL) && (winMem[hwnd]->subjectChanged || winMem[hwnd]->entryChanged)) {
                result=MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WANT_CHANGES_SAVED),getStringFromTable(IDS_APPNAME,1),MB_ICONEXCLAMATION | MB_YESNOCANCEL);
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

BOOL CALLBACK NotePropDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    NOTE tempNote;
    long int result;
    bool addUpTest=false;
    switch(msg) {
        case WM_INITDIALOG:
            addUpTest=IsWindowEnabled(GetDlgItem(GetParent(hwnd),ID_EDIT_BUTTON1));
            properties_LockAllButtons(g_hwnd,hwnd);
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
                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON1), !tempNote.locked);
                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON2),  tempNote.locked);
            }
            else {
                MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),getStringFromTable(IDS_APPNAME,1),MB_ICONHAND | MB_OK);
                EndDialog(hwnd,IDOK);
            }
            break;
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
                        MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),getStringFromTable(IDS_APPNAME,1),MB_ICONHAND | MB_OK);
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
                        MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),getStringFromTable(IDS_APPNAME,1),MB_ICONHAND | MB_OK);
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

BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    NOTE tempNote;
    switch(msg) {
        case WM_INITDIALOG:
            SetWindowText(GetDlgItem(hwnd,IDC_STATIC7),getStringFromTable(IDS_APPNAME));
            break;
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

BOOL CALLBACK PreferencesDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    bool enabled;
    std::string iniFile;
    LIB_ITER lIt;
    unsigned int counter, counter2, selectedIndex, selectedIndex2;
    switch(msg) {
        case WM_INITDIALOG:
            check3DChanged=false;
            SetWindowText(GetDlgItem(hwnd,IDC_STATIC8),((std::string)(getStringFromTable(IDS_STRING_BUILD_DATE))+__DATE__+", "+__TIME__).c_str());
            SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)getStringFromTable(IDS_STRING_NORMAL_WINDOW));
            SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)getStringFromTable(IDS_STRING_MINIMIZED_WINDOW));
            SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)getStringFromTable(IDS_STRING_MAXIMIZED_WINDOW));
            SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)getStringFromTable(IDS_STRING_NORMAL_WINDOW));
            SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)getStringFromTable(IDS_STRING_MINIMIZED_WINDOW));
            SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)getStringFromTable(IDS_STRING_MAXIMIZED_WINDOW));
            SendMessage(GetDlgItem(hwnd, IDC_COMBO3), CB_ADDSTRING, 0, (LPARAM)getStringFromTable(IDS_STRING_BUILTIN_LANGUAGE));
            // begin temporary part
            SendMessage(GetDlgItem(hwnd, IDC_COMBO3), CB_SETCURSEL, 0, 0);
            // end temporary part
            counter=0;
            counter2=0;
            selectedIndex=0;
            selectedIndex2=0;
            for(lIt=libraries.begin(); lIt!=libraries.end(); ++lIt) {
                if(lIt->type==LIB_CODEPAGE) {
                    SendMessage(GetDlgItem(hwnd, IDC_COMBO4), CB_ADDSTRING, 0, (LPARAM)(char*)((lIt->relatedInfo)+" ["+(lIt->filename)+"]").c_str());
                    if(lIt->filename==mainSettings.selectedCodePage) {
                        selectedIndex=counter;
                    }
                    ++counter;
                }
                else if(lIt->type==LIB_STRINGTABLE) {
                    SendMessage(GetDlgItem(hwnd, IDC_COMBO3), CB_ADDSTRING, 0, (LPARAM)(char*)((lIt->relatedInfo)+" ["+(lIt->filename)+"]").c_str());
                    // part to do
                    ++counter2;
                }
            }
            SendMessage(GetDlgItem(hwnd, IDC_COMBO4), CB_SETCURSEL, selectedIndex, 0);
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
            CheckDlgButton(hwnd, IDC_CHECK2,    mainSettings.autoReload     ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECK9,    mainSettings.autoRefresh    ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECK10,   mainSettings.savePosSizes   ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECK3,    mainSettings.use3DControls  ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECK4,    mainSettings.use3DButtons   ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECK5,    mainSettings.use3DLists     ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECK6,    mainSettings.use3DEdits     ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECK7,    mainSettings.use3DCombos    ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECK8,    mainSettings.use3DDialogs   ? BST_CHECKED : BST_UNCHECKED);
            enabled=IsDlgButtonChecked(hwnd,    IDC_CHECK3);
            EnableWindow(GetDlgItem(hwnd,IDC_CHECK4),enabled);
            EnableWindow(GetDlgItem(hwnd,IDC_CHECK5),enabled);
            EnableWindow(GetDlgItem(hwnd,IDC_CHECK6),enabled);
            EnableWindow(GetDlgItem(hwnd,IDC_CHECK7),enabled);
            EnableWindow(GetDlgItem(hwnd,IDC_CHECK8),enabled);
            break;
        case WM_COMMAND:
            switch(wParam) {
                case IDOK:
                    GetModuleFileName(g_hInstance,buffer,32767);
                    iniFile=getDefaultIniFile(buffer);
                    mainSettings.mainWindowSystem=IsDlgButtonChecked(hwnd, IDC_RADIO1);
                    mainSettings.editWindowSystem=IsDlgButtonChecked(hwnd, IDC_RADIO3);
                    mainSettings.mainWindowStyle=SendMessage(GetDlgItem(hwnd,IDC_COMBO1), CB_GETCURSEL, 0, 0);
                    mainSettings.editWindowStyle=SendMessage(GetDlgItem(hwnd,IDC_COMBO2), CB_GETCURSEL, 0, 0);
                    mainSettings.autoReload= IsDlgButtonChecked(hwnd, IDC_CHECK2);
                    mainSettings.autoRefresh=IsDlgButtonChecked(hwnd, IDC_CHECK9);
                    if((mainSettings.savePosSizes==false) && (IsDlgButtonChecked(hwnd, IDC_CHECK10))) {
                        mainSettings.mainWindowX=       CW_USEDEFAULT;
                        mainSettings.mainWindowY=       CW_USEDEFAULT;
                        mainSettings.mainWindowSizeX=   CW_USEDEFAULT;
                        mainSettings.mainWindowSizeY=   CW_USEDEFAULT;
                        mainSettings.editWindowX=       CW_USEDEFAULT;
                        mainSettings.editWindowY=       CW_USEDEFAULT;
                        mainSettings.editWindowSizeX=   CW_USEDEFAULT;
                        mainSettings.editWindowSizeY=   CW_USEDEFAULT;
                    }
                    mainSettings.savePosSizes=  IsDlgButtonChecked(hwnd, IDC_CHECK10);
                    mainSettings.use3DControls= IsDlgButtonChecked(hwnd, IDC_CHECK3);
                    mainSettings.use3DButtons=  IsDlgButtonChecked(hwnd, IDC_CHECK4);
                    mainSettings.use3DLists=    IsDlgButtonChecked(hwnd, IDC_CHECK5);
                    mainSettings.use3DEdits=    IsDlgButtonChecked(hwnd, IDC_CHECK6);
                    mainSettings.use3DCombos=   IsDlgButtonChecked(hwnd, IDC_CHECK7);
                    mainSettings.use3DDialogs=  IsDlgButtonChecked(hwnd, IDC_CHECK8);
                    counter=0;
                    counter2=0;
                    selectedIndex2=SendMessage(GetDlgItem(hwnd,IDC_COMBO3), CB_GETCURSEL, 0, 0);
                    selectedIndex= SendMessage(GetDlgItem(hwnd,IDC_COMBO4), CB_GETCURSEL, 0, 0);
                    codePageChanged=false;
                    for(lIt=libraries.begin(); lIt!=libraries.end(); ++lIt) {
                        if(lIt->type==LIB_CODEPAGE) {
                            if(selectedIndex==counter) {
                                if(lIt->filename!=mainSettings.selectedCodePage) {
                                    codePageChanged=true;
                                }
                                mainSettings.selectedCodePage=lIt->filename;
                            }
                            ++counter;
                        }
                        else if(lIt->type==LIB_STRINGTABLE) {
                            if(selectedIndex2==counter2) {
                                // part to do
                            }
                            ++counter2;
                        }
                    }
                    saveMainSettings(mainSettings,(char*)iniFile.c_str());
                    if(codePageChanged) {
                        unloadCodePage(hCodePageLib,hCodePageDefinition);
                        if(!loadAndPrepareCodePage(mainSettings,libraries,hCodePageLib,hCodePageDefinition,rawCodePage,mappedCodePage)) {
                            MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_CODEPAGE_ERROR_2),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONSTOP | MB_OK);
                        }
                    }
                    if(check3DChanged) {
                        MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_CTL3D_CHANGE),getStringFromTable(IDS_STRING_INFORMATION,1),MB_ICONINFORMATION | MB_OK);
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
                case IDC_CHECK5:
                case IDC_CHECK6:
                case IDC_CHECK7:
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

BOOL CALLBACK ConnSettDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
                CheckDlgButton(hwnd,IDC_CHECK11,connectionSettings.requestCompression ? BST_CHECKED : BST_UNCHECKED);
                connection_LockAllButtons(hwnd);
                serverInfo=noter_getServerInfo(connectionSettings,buffer);
                connection_UnlockAllButtons(hwnd);
                if(serverInfo.version==MATCH_VERSION) {
                    SetWindowText(GetDlgItem(hwnd,IDC_STATIC8), (char*)toCodePage(mappedCodePage,(char*)serverInfo.name.c_str()).c_str());
                    SetWindowText(GetDlgItem(hwnd,IDC_STATIC9), (char*)toCodePage(mappedCodePage,(char*)serverInfo.timezone.c_str()).c_str());
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
                            GetModuleFileName(g_hInstance,buffer,32767);
                            iniFile=getDefaultIniFile(buffer);
                            saveConnectionSettings(connectionSettings,(char*)iniFile.c_str());
                        }
                        EndDialog(hwnd,IDOK);
                    }
                    else {
                        MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_PORT_NUMBER),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
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
                            SetWindowText(GetDlgItem(hwnd,IDC_STATIC8), (char*)toCodePage(mappedCodePage,(char*)serverInfo.name.c_str()).c_str());
                            SetWindowText(GetDlgItem(hwnd,IDC_STATIC9), (char*)toCodePage(mappedCodePage,(char*)serverInfo.timezone.c_str()).c_str());
                            SetWindowText(GetDlgItem(hwnd,IDC_STATIC10),(char*)toCodePage(mappedCodePage,(char*)serverInfo.version.c_str()).c_str());
                            MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_CONN_ESTABLISHED),getStringFromTable(IDS_STRING_INFORMATION,1),MB_ICONINFORMATION | MB_OK);
                        }
                        else {
                            SetWindowText(GetDlgItem(hwnd,IDC_STATIC8), getStringFromTable(IDS_STRING_NOT_CONNECTED));
                            SetWindowText(GetDlgItem(hwnd,IDC_STATIC9), getStringFromTable(IDS_STRING_NOT_CONNECTED));
                            SetWindowText(GetDlgItem(hwnd,IDC_STATIC10),getStringFromTable(IDS_STRING_NOT_CONNECTED));
                            MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_CONNECTION_ERROR),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                        }
                    }
                    else {
                        MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_PORT_NUMBER),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
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
                                if((GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT1))>0) && (GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT2))>0) && (GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT3))>0)) {
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

BOOL CALLBACK CredsSettDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON7),  true);
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON8),  true);
                    }
                    else {
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON7),  false);
                        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON8),  false);
                    }
                }
                else {
                    EnableWindow(GetDlgItem(hwnd,IDOK),         false);
                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON6),  false);
                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON7),  false);
                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON8),  false);
                }
            }
            else {
                EnableWindow(GetDlgItem(hwnd,IDOK),         false);
                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON5),  false);
                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON6),  false);
                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON7),  false);
                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON8),  false);
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
                        GetModuleFileName(g_hInstance,buffer,32767);
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
                    if((editsChanged2) && (MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_CREDENTIALS_CHANGED),getStringFromTable(IDS_APPNAME,1),MB_ICONQUESTION | MB_YESNO)==IDNO)) {
                        break;
                    }
                    EndDialog(hwnd,IDCANCEL);
                    break;
                case IDC_BUTTON5:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDC_BUTTON5))) {
                        break;
                    }
                    auxCredentials=&tempCredentials;
                    if(MakeDialogBox(hwnd,IDD_DIALOG6,UserRegDlgProc)==IDOK) {
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
                                    MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_LOGIN_SUCCESSFUL),getStringFromTable(IDS_STRING_INFORMATION,1),MB_ICONINFORMATION | MB_OK);
                                }
                            }
                            else {
                                SetWindowText(GetDlgItem(hwnd,IDC_STATIC11),getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                SetWindowText(GetDlgItem(hwnd,IDC_STATIC12),getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                SetWindowText(GetDlgItem(hwnd,IDC_STATIC13),getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                SetWindowText(GetDlgItem(hwnd,IDC_STATIC14),getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                SetWindowText(GetDlgItem(hwnd,IDC_STATIC15),getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                EnableWindow( GetDlgItem(hwnd,IDC_BUTTON7), false);
                                EnableWindow( GetDlgItem(hwnd,IDC_BUTTON8), false);
                                MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                            }
                        }
                        else {
                            MessageBox(hwnd,getStringFromTable(IDS_STRING_NO_CREDENTIALS),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                        }
                    }
                    else {
                        MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_NO_CONN_SETTINGS),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
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
                        tempString=(std::string)getStringFromTable(IDS_STRING_MSG_ACC_DELETE_PART1)
                                    +auxCredentials->username
                                    +(std::string)getStringFromTable(IDS_STRING_MSG_ACC_DELETE_PART2,1);
                        if(MessageBox(hwnd,(char*)tempString.c_str(),getStringFromTable(IDS_APPNAME,1),MB_ICONQUESTION | MB_YESNO)==IDYES) {
                            if(MakeDialogBox(hwnd,IDD_DIALOG7,PassConfirmDlgProc)==IDOK) {
                                if(useTestCredentials) {
                                    SetWindowText(GetDlgItem(hwnd,IDC_EDIT4),"");
                                    SetWindowText(GetDlgItem(hwnd,IDC_EDIT5),"");
                                    SetWindowText(GetDlgItem(hwnd,IDC_STATIC11),getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                    SetWindowText(GetDlgItem(hwnd,IDC_STATIC12),getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                    SetWindowText(GetDlgItem(hwnd,IDC_STATIC13),getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                    SetWindowText(GetDlgItem(hwnd,IDC_STATIC14),getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                    SetWindowText(GetDlgItem(hwnd,IDC_STATIC15),getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                    EnableWindow( GetDlgItem(hwnd,IDC_BUTTON7), false);
                                    EnableWindow( GetDlgItem(hwnd,IDC_BUTTON8), false);
                                }
                                else {
                                    credentials.username="";
                                    credentials.password="";
                                    GetModuleFileName(g_hInstance,buffer,32767);
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
                        MessageBox(hwnd,getStringFromTable(IDS_STRING_NO_CREDENTIALS),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
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
                        if(MakeDialogBox(hwnd,IDD_DIALOG8,PassChangeDlgProc)==IDOK) {
                            SetWindowText(GetDlgItem(hwnd,IDC_EDIT5),(char*)tempCredentials.password.c_str());
                            useTestCredentials=true;
                            SendMessage(hwnd, WM_COMMAND, IDC_BUTTON6, IDC_BUTTON5);
                        }
                    }
                    else {
                        MessageBox(hwnd,getStringFromTable(IDS_STRING_NO_CREDENTIALS),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                    }
                    break;
                case IDC_EDIT4:
                case IDC_EDIT5:
                    switch(HIWORD(lParam)) {
                        case EN_CHANGE:
                            editsChanged=true;
                            if(GetWindowTextLength(GetDlgItem(hwnd,wParam))==0) {
                                EnableWindow(GetDlgItem(hwnd,IDOK),       false);
                                EnableWindow(GetDlgItem(hwnd,IDC_BUTTON6),false);
                            }
                            else {
                                if(GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT4))>0 && GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT5))>0) {
                                    EnableWindow(GetDlgItem(hwnd,IDOK),       true);
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

BOOL CALLBACK UserRegDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
                            MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_REGISTRATION_SUCC),getStringFromTable(IDS_APPNAME,1),MB_ICONINFORMATION | MB_OK);
                            EndDialog(hwnd,IDOK);
                        }
                        else {
                            tempString=(std::string)getStringFromTable(IDS_STRING_MSG_REGISTRATION_ERROR,1)+noter_getAnswerString(result);
                            MessageBox(hwnd,(char*)tempString.c_str(),getStringFromTable(IDS_APPNAME,1),MB_ICONEXCLAMATION | MB_OK);
                        }
                    }
                    else {
                        MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_PASSWORDS_NO_MATCH),getStringFromTable(IDS_APPNAME,1),MB_ICONEXCLAMATION | MB_OK);
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
                                if(GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT6))>0 && GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT7))>0 && GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT8))>0) {
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

BOOL CALLBACK PassConfirmDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
                            tempString=(std::string)getStringFromTable(IDS_STRING_MSG_USER_SPACED)+auxCredentials->username+(std::string)getStringFromTable(IDS_STRING_MSG_USER_DELETED_SPACED,1);
                            MessageBox(hwnd,(char*)tempString.c_str(),getStringFromTable(IDS_APPNAME,1),MB_ICONINFORMATION | MB_OK);
                            EndDialog(hwnd,IDOK);
                        }
                        else {
                            MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),getStringFromTable(IDS_APPNAME,1),MB_ICONEXCLAMATION | MB_OK);
                        }
                    }
                    else {
                        MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_PASSWORD),getStringFromTable(IDS_APPNAME,1),MB_ICONEXCLAMATION | MB_OK);
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

BOOL CALLBACK PassChangeDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
                    GetWindowText(GetDlgItem(hwnd,IDC_EDIT9), buffer,65535);
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
                                MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_PASSWORD_CHANGED),getStringFromTable(IDS_APPNAME,1),MB_ICONINFORMATION | MB_OK);
                                EndDialog(hwnd,IDOK);
                            }
                            else {
                                MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),getStringFromTable(IDS_APPNAME,1), MB_ICONEXCLAMATION | MB_OK);
                            }
                        }
                        else {
                            MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_PASSWORD),getStringFromTable(IDS_APPNAME,1),MB_ICONEXCLAMATION | MB_OK);
                        }
                    }
                    else {
                        MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_PASSWORDS_NO_MATCH),getStringFromTable(IDS_APPNAME,1),MB_ICONEXCLAMATION | MB_OK);
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
                                if(GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT9))>0 && GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT10))>0 && GetWindowTextLength(GetDlgItem(hwnd,IDC_EDIT11))>0) {
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
//  NOTES IMPORT DIALOG PROCEDURE (TODO)
//
//////////////////////////////////////



//////////////////////////////////////
//
//  NOTES EXPORT DIALOG PROCEDURE
//
//////////////////////////////////////

BOOL CALLBACK NotesExpDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_INITDIALOG:
            setProgress(hwnd,IDC_STATIC16,IDC_STATIC16_MAXWIDTH,0);
            CheckDlgButton(hwnd, IDC_CHECK14, BST_CHECKED);
            CheckDlgButton(hwnd, IDC_CHECK15, BST_CHECKED);
            CheckDlgButton(hwnd, IDC_CHECK16, BST_CHECKED);
            CheckDlgButton(hwnd, IDC_CHECK17, BST_CHECKED);
            CheckRadioButton(hwnd, IDC_RADIO7, IDC_RADIO8, IDC_RADIO7);
            EnableWindow(GetDlgItem(hwnd,IDC_EDIT14),false);
            EnableWindow(GetDlgItem(hwnd,IDC_EDIT15),false);
            EnableWindow(GetDlgItem(hwnd,IDC_CHECK13),false);
            EnableWindow(GetDlgItem(hwnd,IDC_CHECK18),false);
            if(SendMessage(GetDlgItem(GetParent(hwnd),ID_LISTBOX),LB_GETSELCOUNT, 0, 0)>1) {
                CheckRadioButton(hwnd, IDC_RADIO5, IDC_RADIO6, IDC_RADIO6);
                CheckDlgButton(hwnd, IDC_CHECK12, BST_CHECKED);
            }
            else {
                CheckRadioButton(hwnd, IDC_RADIO5, IDC_RADIO6, IDC_RADIO5);
                EnableWindow(GetDlgItem(hwnd,IDC_CHECK12),false);
                EnableWindow(GetDlgItem(hwnd,IDC_RADIO6),false);
                EnableWindow(GetDlgItem(hwnd,IDC_RADIO7),false);
                EnableWindow(GetDlgItem(hwnd,IDC_RADIO8),false);
                EnableWindow(GetDlgItem(hwnd,IDC_CHECK19),false);
            }
            break;
        case WM_CTLCOLOR:
            switch(HIWORD(lParam)) {
                case CTLCOLOR_STATIC:
                    if(LOWORD(lParam)==GetDlgItem(hwnd,IDC_STATIC16)) {
                        return g_hBrush2;
                    }
                    break;
            }
            break;
        case WM_COMMAND:
            switch(wParam) {
                case IDC_RADIO5:
                    EnableWindow(GetDlgItem(hwnd,IDC_RADIO7),false);
                    EnableWindow(GetDlgItem(hwnd,IDC_RADIO8),false);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK13),false);
                    EnableWindow(GetDlgItem(hwnd,IDC_EDIT14),false);
                    EnableWindow(GetDlgItem(hwnd,IDC_EDIT15),false);
                    if(SendMessage(GetDlgItem(GetParent(hwnd),ID_LISTBOX),LB_GETSELCOUNT, 0, 0)>1) {
                        EnableWindow(GetDlgItem(hwnd,IDC_CHECK18),true);
                    }
                    else {
                        EnableWindow(GetDlgItem(hwnd,IDC_CHECK18),false);
                    }
                    CheckRadioButton(hwnd, IDC_RADIO5, IDC_RADIO6, IDC_RADIO5);
                    break;
                case IDC_RADIO6:
                    EnableWindow(GetDlgItem(hwnd,IDC_RADIO7),true);
                    EnableWindow(GetDlgItem(hwnd,IDC_RADIO8),true);
                    if(IsDlgButtonChecked(hwnd,IDC_RADIO8)) {
                        EnableWindow(GetDlgItem(hwnd,IDC_EDIT14),true);
                        EnableWindow(GetDlgItem(hwnd,IDC_CHECK13),true);
                        if(IsDlgButtonChecked(hwnd,IDC_CHECK13)) {
                            EnableWindow(GetDlgItem(hwnd,IDC_EDIT15),true);
                        }
                        else {
                            EnableWindow(GetDlgItem(hwnd,IDC_EDIT15),false);
                        }
                    }
                    else {
                        EnableWindow(GetDlgItem(hwnd,IDC_EDIT14),false);
                        EnableWindow(GetDlgItem(hwnd,IDC_CHECK13),false);
                    }
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK18),false);
                    CheckRadioButton(hwnd, IDC_RADIO5, IDC_RADIO6, IDC_RADIO6);
                    break;
                case IDC_RADIO7:
                    CheckRadioButton(hwnd, IDC_RADIO7, IDC_RADIO8, IDC_RADIO7);
                    EnableWindow(GetDlgItem(hwnd,IDC_EDIT14),false);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK13),false);
                    EnableWindow(GetDlgItem(hwnd,IDC_EDIT15),false);
                    break;
                case IDC_RADIO8:
                    CheckRadioButton(hwnd, IDC_RADIO7, IDC_RADIO8, IDC_RADIO8);
                    EnableWindow(GetDlgItem(hwnd,IDC_EDIT14),true);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHECK13),true);
                    if(IsDlgButtonChecked(hwnd,IDC_CHECK13)) {
                        EnableWindow(GetDlgItem(hwnd,IDC_EDIT15),true);
                    }
                    else {
                        EnableWindow(GetDlgItem(hwnd,IDC_EDIT15),false);
                    }
                    break;
                case IDC_CHECK13:
                    if(IsDlgButtonChecked(hwnd,IDC_CHECK13)) {
                        EnableWindow(GetDlgItem(hwnd,IDC_EDIT15),true);
                    }
                    else {
                        EnableWindow(GetDlgItem(hwnd,IDC_EDIT15),false);
                    }
                    break;
                case IDC_BUTTON9:
                    // TODO: save file window part
                    break;
                case IDC_EXPORT:
                    // TODO: export part
                    break;
                case IDCANCEL:
                    EndDialog(hwnd,IDCANCEL);
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
