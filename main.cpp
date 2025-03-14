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
HBRUSH                      g_hBrushBtnFace=CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
HBRUSH                      g_hBrushActCapt=CreateSolidBrush(GetSysColor(COLOR_ACTIVECAPTION));
HBRUSH                      g_hBrushWindow= CreateSolidBrush(GetSysColor(COLOR_WINDOW));
HBRUSH                      g_hBrushWinText=CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT));
HWND                        g_hwnd;
MSG                         *g_Msg;
HFONT                       hUnderlinedFont=prepareUnderlinedFontObject();
HINSTANCE                   hCodePageLib=NULL;
HGLOBAL                     hCodePageDefinition=NULL;

// Own types
WINDOWMEMORY                winMem;
RAWCODEPAGE                 rawCodePage;
CODEPAGE                    mappedCodePage;
NOTER_CONNECTION_SETTINGS   connectionSettings;
NOTER_CREDENTIALS           credentials, tempCredentials;
NOTER_CREDENTIALS           *auxCredentials;
MAINSETTINGS                mainSettings;
NOTE_SUMMARY                *notes=NULL;
LIBRARIES                   libraries;

// Standard types
long int                    noteCount=0;
long int                    mainLastResult=0;
long int                    *minID, *maxID;
unsigned int                ctlRegs=0;
std::string                 *minLM, *maxLM;
bool                        check3DChanged, editsChanged, editsChanged2, useTestCredentials, firstOptions=false, codePageChanged, cpHover=false, cpClick=false;
char                        buffer[65536];

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
BOOL CALLBACK LibInfoDlgProc    (HWND, UINT, WPARAM, LPARAM);

//////////////////////////////////////
//
//  EDIT WINDOW
//
//////////////////////////////////////

// Editor window creation function
HWND createEditWindow(HWND hwnd, HINSTANCE hInstance, WINDOWMEMORY &winMem, NOTE *note) {
    EDITWINDOW *editWin = new EDITWINDOW;

    makeEditWindowTitle(editWin,note,false,mappedCodePage);
    
    editWin->hwnd=  CreateWindow(NOTER_EDITWINDOW, editWin->windowTitle.c_str(), WS_OVERLAPPEDWINDOW,
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
        
        editWin->hStaticSubject= CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_EDITWIN_TITLE), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                                0, 0, 600, 16, editWin->hwnd, (HMENU)IDC_EDIT_SUBJECT, hInstance, NULL);

        editWin->hEditBoxSubject=CreateWindow(WC_EDIT, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                                                0, 16, 600, 24, editWin->hwnd, (HMENU)IDE_EDIT_SUBJECT, hInstance, NULL);

        editWin->hStaticEntry=   CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_EDITWIN_ENTRY), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                                0, 40, 600, 16, editWin->hwnd, (HMENU)IDC_EDIT_ENTRY, hInstance, NULL);

        editWin->hEditBoxEntry=  CreateWindow(WC_EDIT, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
                                                0, 56, 600, 300, editWin->hwnd, (HMENU)IDE_EDIT_ENTRY, hInstance, NULL);

        if(editWin->note->id==0) {
            editWin->hButtonAddUp=  CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_ADD), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                                                    0, 356, 96, 21, editWin->hwnd, (HMENU)IDB_EDIT_ADDUP, hInstance, NULL);
        }
        else {
            editWin->hButtonAddUp=  CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_UPDATE), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                                                    0, 356, 96, 21, editWin->hwnd, (HMENU)IDB_EDIT_ADDUP, hInstance, NULL);
        }

        if(editWin->note->id==0) {
            editWin->hButtonProps=  CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_PROPERTIES), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                                                    96, 356, 96, 21, editWin->hwnd, (HMENU)IDB_EDIT_PROPERTIES, hInstance, NULL);
        }
        else {
            editWin->hButtonProps=  CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_PROPERTIES), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                    96, 356, 96, 21, editWin->hwnd, (HMENU)IDB_EDIT_PROPERTIES, hInstance, NULL);
        }

        editWin->hButtonClose=  CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_CLOSE), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                192, 356, 96, 21, editWin->hwnd, (HMENU)IDB_EDIT_CLOSE, hInstance, NULL);

        editWin->hStaticGrayBox=CreateWindow(WC_STATIC, NULL, WS_CHILD | WS_VISIBLE | SS_GRAYRECT,
                                                288, 356, 312, 21, editWin->hwnd, (HMENU)IDC_EDIT_GRAYBOX, hInstance, NULL);

        editWin->hStaticStatus= CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_INFO_OK), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                                0, 377, 600, 16, editWin->hwnd, (HMENU)IDC_EDIT_STATUS, hInstance, NULL);

        bool warningState=false;
        if(editWin->note->id>0) {
            SetWindowText(editWin->hEditBoxSubject, toCodePage(mappedCodePage,(char*)editWin->note->subject.c_str()).c_str());
            warningState=decodeWarningState();
            SetWindowText(editWin->hEditBoxEntry,   toCodePage(mappedCodePage,(char*)editWin->note->entry.c_str()).c_str());
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

// Main window class registration function
ATOM registerMainWindowClass(WNDCLASS *wc, HINSTANCE &hInstance) {
    wc->style=          0;
    wc->lpfnWndProc=    MainWndProc;
    wc->cbClsExtra=     0;
    wc->cbWndExtra=     0;
    wc->hInstance=      hInstance;
    wc->hIcon=          LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
    wc->hCursor=        LoadCursor(NULL, IDC_ARROW);
    wc->hbrBackground=  g_hBrushBtnFace;
    wc->lpszMenuName=   MAKEINTRESOURCE(IDR_MENU_MAIN);
    wc->lpszClassName=  NOTER_MAINWINDOW;
    return RegisterClass(wc);
}

// Editor window class registration function
ATOM registerEditWindowClass(WNDCLASS *wc, HINSTANCE &hInstance) {
    wc->style=          0;
    wc->lpfnWndProc=    EditWndProc;
    wc->cbClsExtra=     0;
    wc->cbWndExtra=     0;
    wc->hInstance=      hInstance;
    wc->hIcon=          LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EDITICON));
    wc->hCursor=        LoadCursor(NULL, IDC_ARROW);
    wc->hbrBackground=  g_hBrushBtnFace;
    wc->lpszMenuName=   MAKEINTRESOURCE(IDR_MENU_EDIT);
    wc->lpszClassName=  NOTER_EDITWINDOW;
    return RegisterClass(wc);
}

// Procedure for freeing global resources
void freeGlobalResources(void) {
    WSACleanup();
    DeleteObject(g_hBrushBtnFace);
    DeleteObject(g_hBrushActCapt);
    DeleteObject(g_hBrushWindow);
    DeleteObject(g_hBrushWinText);
    DeleteObject(hUnderlinedFont);
    unloadCodePage(hCodePageLib,hCodePageDefinition);
    return;
}

// Inline procedure for handling selection changes
void inline onSelectionChange(HWND hwnd, unsigned int &count, unsigned int *&selection) {
    count=(unsigned int)SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETSELCOUNT, 0, 0);
    if(count==0) {
        SetWindowText(GetDlgItem(hwnd,IDC_NOTEID),getStringFromTable(IDS_STRING_NOT_CHOSEN));
        SetWindowText(GetDlgItem(hwnd,IDC_NOTELASTMOD),getStringFromTable(IDS_STRING_NOT_CHOSEN));
        EnableWindow(GetDlgItem(hwnd,IDB_OPEN),     false);
        EnableWindow(GetDlgItem(hwnd,IDB_DELETE),   false);
    }
    else {
        if(IsWindowEnabled(GetDlgItem(hwnd,IDB_EXIT))) {
            if(count==1) {
                getSelection(GetDlgItem(hwnd,ID_LISTBOX),selection);
                SetWindowText(GetDlgItem(hwnd,IDC_NOTEID),IntToStr(notes[selection[0]].id).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_NOTELASTMOD),notes[selection[0]].lastModified.c_str());
            }
            else {
                if(mainSettings.showMultiIDnLM) {
                    getSelection(GetDlgItem(hwnd,ID_LISTBOX),selection);
                    minID=&notes[selection[0]].id;
                    maxID=&notes[selection[0]].id;
                    minLM=&notes[selection[0]].lastModified;
                    maxLM=&notes[selection[0]].lastModified;
                    for(unsigned int x=1; x<count; ++x) {
                        if(notes[selection[x]].id<*minID) {
                            minID=&notes[selection[x]].id;
                        }
                        if(notes[selection[x]].id>*maxID) {
                            maxID=&notes[selection[x]].id;
                        }
                        if(notes[selection[x]].lastModified<*minLM) {
                            minLM=&notes[selection[x]].lastModified;
                        }
                        if(notes[selection[x]].lastModified>*maxLM) {
                            maxLM=&notes[selection[x]].lastModified;
                        }
                    }
                    SetWindowText(GetDlgItem(hwnd,IDC_NOTEID),(IntToStr(*maxID)+" - "+IntToStr(*minID)+" => "+IntToStr(count)).c_str());
                    SetWindowText(GetDlgItem(hwnd,IDC_NOTELASTMOD),((*maxLM)+" - "+(*minLM)).c_str());
                }
                else {
                    SetWindowText(GetDlgItem(hwnd,IDC_NOTEID),getStringFromTable(IDS_STRING_MULTIPLE_CHOSEN));
                    SetWindowText(GetDlgItem(hwnd,IDC_NOTELASTMOD),getStringFromTable(IDS_STRING_MULTIPLE_CHOSEN));
                }
            }
            EnableWindow(GetDlgItem(hwnd,IDB_OPEN),     true);
            EnableWindow(GetDlgItem(hwnd,IDB_DELETE),   true);
        }
    }
    if(mainLastResult!=0) {
        mainLastResult=0;
        SetWindowText(GetDlgItem(hwnd,IDC_STATUS),(char*)noter_getAnswerString(mainLastResult).c_str());
    }
    return;
}

//////////////////////////////////////
//
//  MAIN PROGRAM
//
//////////////////////////////////////

// Main program function (WinMain)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    storeStringTableInstance(           hInstance);
    storeConnectionSettingsReference(   &connectionSettings);
    storeCredentialsReference(          &credentials);
    storeGlobalHWNDReference(           &g_hwnd);
    storeWindowMemoryReference(         &winMem);

    WNDCLASS wc = { 0 };
    WNDCLASS wc2= { 0 };

    HWND     hwnd, temp;
    HWND     hButtonDownload, hButtonCreate, hButtonOpen, hButtonExit, hButtonDelete;
    HWND     hListBox;
    HWND     hStaticNoteIDLabel, hStaticLastModLabel, hStaticNoteID, hStaticLastMod, hStaticStatus, hStaticGrayBox;

    HACCEL   hAccel;
    MSG      msg;

    // make global "reference" to the message variable
    g_Msg=&msg;
    
    GetModuleFileName(hInstance,buffer,32767);
    listAvailableLibs(buffer,libraries);
    std::string iniFile=getDefaultIniFile(buffer);
    connectionSettings=getConnectionSettings((char*)iniFile.c_str());
    credentials= getCredentials( (char*)iniFile.c_str());
    mainSettings=getMainSettings((char*)iniFile.c_str(),&libraries);

    if(wsInit()==SOCKET_ERROR) {
        MessageBox(NULL,getStringFromTable(IDS_STRING_MSG_WINSOCK_ERROR),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONSTOP | MB_OK);
        freeGlobalResources();
        return FALSE;
    }

    if(!loadAndPrepareCodePage(mainSettings,libraries,hCodePageLib,hCodePageDefinition,rawCodePage,mappedCodePage)) {
        MessageBox(NULL,getStringFromTable(IDS_STRING_MSG_CODEPAGE_ERROR),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONSTOP | MB_OK);
        freeGlobalResources();
        return FALSE;
    }

    if((!registerMainWindowClass(&wc, hInstance)) || (!registerEditWindowClass(&wc2, hInstance))) {
        MessageBox(NULL,getStringFromTable(IDS_STRING_MSG_WNDCLASS_ERROR),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONSTOP | MB_OK);
        freeGlobalResources();
        return FALSE;
    }
    
    hwnd=   CreateWindow(NOTER_MAINWINDOW, getStringFromTable(IDS_APPNAME), WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    if(hwnd==NULL) {
        MessageBox(NULL,getStringFromTable(IDS_STRING_MSG_WND_CREATE_ERROR),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONSTOP | MB_OK);
        freeGlobalResources();
        return FALSE;
    }
    else {
        g_hwnd=hwnd;
    }

    hAccel=LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));
    if(!hAccel) {
        MessageBox(NULL,getStringFromTable(IDS_STRING_MSG_ACCELERATORS_ERROR),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONSTOP | MB_OK);
        freeGlobalResources();
        return FALSE;
    }

    if(mainSettings.savePosSizes) {
        if((mainSettings.mainWindowX!=CW_USEDEFAULT) && (mainSettings.mainWindowY!=CW_USEDEFAULT) && (mainSettings.mainWindowSizeX!=CW_USEDEFAULT) && (mainSettings.mainWindowSizeY!=CW_USEDEFAULT)) {
            SetWindowPos(hwnd,NULL,mainSettings.mainWindowX,mainSettings.mainWindowY,mainSettings.mainWindowSizeX,mainSettings.mainWindowSizeY,SWP_NOZORDER);
        }
    }

    hButtonDownload=CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_DOWNLOAD), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                    0, 0, 80, 21, hwnd, (HMENU)IDB_DOWNLOAD, hInstance, NULL);

    hButtonCreate=  CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_CREATE), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                    80, 0, 80, 21, hwnd, (HMENU)IDB_CREATE, hInstance, NULL);

    hButtonOpen=    CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_OPEN), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                                    160, 0, 80, 21, hwnd, (HMENU)IDB_OPEN, hInstance, NULL);

    hButtonDelete=  CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_DELETE), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                                    240, 0, 80, 21, hwnd, (HMENU)IDB_DELETE, hInstance, NULL);

    hButtonExit=    CreateWindow(WC_BUTTON, getStringFromTable(IDS_STRING_EXIT), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                    320, 0, 80, 21, hwnd, (HMENU)IDB_EXIT, hInstance, NULL);

    hStaticGrayBox= CreateWindow(WC_STATIC, NULL, WS_CHILD | WS_VISIBLE | SS_GRAYRECT,
                                    400, 0, 200, 21, hwnd, (HMENU)IDC_GRAYBOX, hInstance, NULL);

    hListBox =      CreateWindow(WC_LISTBOX, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_TABSTOP | ES_AUTOVSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_EXTENDEDSEL,
                                    0, 21, 600, 300, hwnd, (HMENU)ID_LISTBOX, hInstance, NULL);

    if(mainSettings.use3DLists) {
        SetWindowPos(hListBox,NULL,0,22,600,298,SWP_NOZORDER);
    }
    else {
        SetWindowPos(hListBox,NULL,0,21,600,300,SWP_NOZORDER);
    }

    hStaticNoteIDLabel= CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_ID), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        8, 329, 128, 16, hwnd, (HMENU)IDC_SID, hInstance, NULL);
                           
    hStaticLastModLabel=CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_LAST_CHANGED), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        8, 346, 128, 16, hwnd, (HMENU)IDC_LASTCHANGED, hInstance, NULL);
                           
    hStaticNoteID=      CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_NOT_CHOSEN), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        137, 329, 454, 16, hwnd, (HMENU)IDC_NOTEID, hInstance, NULL);
                           
    hStaticLastMod=     CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_NOT_CHOSEN), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        137, 346, 454, 16, hwnd, (HMENU)IDC_NOTELASTMOD, hInstance, NULL);
                           
    hStaticStatus=      CreateWindow(WC_STATIC, getStringFromTable(IDS_STRING_INFO_OK), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        0, 370, 600, 16, hwnd, (HMENU)IDC_STATUS, hInstance, NULL);

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
                SendMessage(hwnd, WM_COMMAND, IDB_DOWNLOAD, ID_FILE_RELOAD);
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

// Main window message processing function
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Local variables
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
    // Switch section
    switch(msg) {
        case WM_CTLCOLOR:
            switch(HIWORD(lParam)) {
                case CTLCOLOR_STATIC:
                    if((LOWORD(lParam)!=GetDlgItem(hwnd,IDC_STATUS)) && (LOWORD(lParam)!=GetDlgItem(hwnd,IDC_GRAYBOX))) {
                        SetBkMode((HDC)wParam, TRANSPARENT);
                        return g_hBrushBtnFace;
                    }
                    break;
                case CTLCOLOR_BTN:
                    SetBkMode((HDC)wParam, TRANSPARENT);
                    break;
            }
            break;
        case WM_INITMENU:
            count=(unsigned int)SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETSELCOUNT, 0, 0);
            EnableMenuItem(GetMenu(hwnd),ID_FILE_RELOAD,        IsWindowEnabled(GetDlgItem(hwnd,IDB_DOWNLOAD))                                                          ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_FILE_OPEN,          IsWindowEnabled(GetDlgItem(hwnd,IDB_OPEN))                                                              ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_FILE_EXIT,          IsWindowEnabled(GetDlgItem(hwnd,IDB_EXIT))                                                              ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_FILE_DELETE,        IsWindowEnabled(GetDlgItem(hwnd,IDB_DELETE))                                                            ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_FILE_IMPORT,        (noter_connectionSettingsAvailable(connectionSettings) && IsWindowEnabled(GetDlgItem(hwnd,IDB_EXIT)))   ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_FILE_EXPORT,        ((count>0) && IsWindowEnabled(GetDlgItem(hwnd,IDB_EXIT)))                                               ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_EDIT_SELECTALL,     (count!=noteCount)                                                                                      ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_OPTIONS_CONNECTION, IsWindowEnabled(GetDlgItem(hwnd,IDB_EXIT))                                                              ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(GetMenu(hwnd),ID_OPTIONS_CREDENTIALS,(noter_connectionSettingsAvailable(connectionSettings) && IsWindowEnabled(GetDlgItem(hwnd,IDB_EXIT)))   ? MF_ENABLED : MF_GRAYED);
            break;
        case WM_COMMAND:
            switch(wParam) {
                case ID_ACC_TAB:
                    SetFocus(GetNextDlgTabItem(hwnd,GetFocus(),false));
                    break;
                case ID_ACC_CTRLN:
                    SendMessage(hwnd, WM_COMMAND, IDB_CREATE, 0);
                    break;
                case ID_ACC_ENTER:
                    SendMessage(hwnd, WM_COMMAND, IDB_OPEN, 0);
                    break;
                case ID_ACC_F5:
                    SendMessage(hwnd, WM_COMMAND, IDB_DOWNLOAD, ID_ACC_F5);
                    break;
                case ID_ACC_DEL:
                    SendMessage(hwnd, WM_COMMAND, IDB_DELETE, 0);
                    break;
                case ID_ACC_CTRLI:
                    SendMessage(hwnd, WM_COMMAND, ID_FILE_IMPORT, 0);
                    break;
                case ID_ACC_CTRLE:
                    SendMessage(hwnd, WM_COMMAND, ID_FILE_EXPORT, 0);
                    break;
                case ID_ACC_ALTF4:
                    SendMessage(hwnd, WM_COMMAND, IDB_EXIT, 0);
                    break;
                case ID_ACC_CTRLA:
                    SendMessage(hwnd, WM_COMMAND, ID_EDIT_SELECTALL, 0);
                    break;
                case ID_ACC_F1:
                    SendMessage(hwnd, WM_COMMAND, ID_HELP_HELP, 0);
                    break;
                case ID_FILE_NEW:
                    SendMessage(hwnd, WM_COMMAND, IDB_CREATE, 0);
                    break;
                case ID_FILE_OPEN:
                    SendMessage(hwnd, WM_COMMAND, IDB_OPEN, 0);
                    break;
                case ID_FILE_RELOAD:
                    SendMessage(hwnd, WM_COMMAND, IDB_DOWNLOAD, ID_FILE_RELOAD);
                    break;
                case ID_FILE_DELETE:
                    SendMessage(hwnd, WM_COMMAND, IDB_DELETE, 0);
                    break;
                case ID_FILE_IMPORT:
                    if(IsWindowEnabled(GetDlgItem(hwnd,IDB_EXIT))) {
                        // TODO: Import section
                    }
                    break;
                case ID_FILE_EXPORT:
                    if((SendMessage(GetDlgItem(hwnd,ID_LISTBOX),LB_GETSELCOUNT,0,0)>0) && IsWindowEnabled(GetDlgItem(hwnd,IDB_EXIT))) {
                        MakeDialogBox(hwnd,IDD_EXPORT,NotesExpDlgProc);
                    }
                    break;
                case ID_FILE_EXIT:
                    SendMessage(hwnd, WM_COMMAND, IDB_EXIT, 0);
                    break;
                case ID_EDIT_SELECTALL:
                    if(SendMessage(GetDlgItem(hwnd,ID_LISTBOX),LB_GETSELCOUNT,0,0)!=noteCount) {
                        SendMessage(GetDlgItem(hwnd,ID_LISTBOX),LB_SETSEL,TRUE,-1);
                        onSelectionChange(hwnd,count,selection);
                    }
                    break;
                case ID_OPTIONS_PREFERENCES:
                    if((MakeDialogBox(hwnd,IDD_PREFERENCES,PreferencesDlgProc)==IDOK) && (codePageChanged)) {
                        if(MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WANT_RELOAD),getStringFromTable(IDS_APPNAME,1),MB_ICONQUESTION | MB_YESNO)==IDYES) {
                            SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_SETSEL, FALSE, -1);
                            SendMessage(hwnd, WM_COMMAND, IDB_DOWNLOAD, ID_FILE_RELOAD);
                        }
                    }
                    break;
                case ID_OPTIONS_CONNECTION:
                    if((MakeDialogBox(hwnd,IDD_CONNECTION,ConnSettDlgProc)==IDOK) && (editsChanged)) {
                        if((!firstOptions) && (noter_credentialsAvailable(credentials)) && (MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WANT_RELOAD),getStringFromTable(IDS_APPNAME,1),MB_ICONQUESTION | MB_YESNO)==IDYES)) {
                            SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_SETSEL, FALSE, -1);
                            SendMessage(hwnd, WM_COMMAND, IDB_DOWNLOAD, ID_FILE_RELOAD);
                        }
                    }
                    break;
                case ID_OPTIONS_CREDENTIALS:
                    if((MakeDialogBox(hwnd,IDD_CREDENTIALS,CredsSettDlgProc)==IDOK) && (editsChanged || firstOptions)) {
                        if(SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCOUNT, 0, 0)>0) {
                            SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_SETSEL, FALSE, -1);
                            SendMessage(hwnd, WM_COMMAND, IDB_DOWNLOAD, ID_FILE_RELOAD);
                        }
                        else {
                            if(MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WANT_RELOAD),getStringFromTable(IDS_APPNAME,1),MB_ICONQUESTION | MB_YESNO)==IDYES) {
                                SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_SETSEL, FALSE, -1);
                                SendMessage(hwnd, WM_COMMAND, IDB_DOWNLOAD, ID_FILE_RELOAD);
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
                    MakeDialogBox(hwnd,IDD_APPINFO,AboutDlgProc);
                    break;
                // Refresh button
                case IDB_DOWNLOAD:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDB_DOWNLOAD))) {
                        break;
                    }
                    main_LockAllButtons(hwnd);
                    SetWindowText(GetDlgItem(hwnd,IDC_STATUS),getStringFromTable(IDS_STRING_LOADING_NOTE_LIST));
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
                        SetWindowText(GetDlgItem(hwnd,IDC_NOTEID),IntToStr(notes[selection[0]].id).c_str());
                        SetWindowText(GetDlgItem(hwnd,IDC_NOTELASTMOD),notes[selection[0]].lastModified.c_str());
                    }
                    else {
                        SetWindowText(GetDlgItem(hwnd,IDC_NOTEID),getStringFromTable(IDS_STRING_NOT_CHOSEN));
                        SetWindowText(GetDlgItem(hwnd,IDC_NOTELASTMOD),getStringFromTable(IDS_STRING_NOT_CHOSEN));
                        EnableWindow(GetDlgItem(hwnd,IDB_OPEN),     false);
                        EnableWindow(GetDlgItem(hwnd,IDB_DELETE),   false);
                    }
                    if(noteCount>=0) {
                        mainLastResult=INFO_LIST_SUCCESSFUL;
                        if(lParam!=0) {
                            tempString=noter_getAnswerString(mainLastResult)+(std::string)getStringFromTable(IDS_STRING_SPACED_COUNT)+IntToStr(noteCount)+".";
                            compressionRatio=getCompressionRatio();
                            if(compressionRatio!=100) {
                                tempString=tempString+(std::string)getStringFromTable(IDS_STRING_SPACED_COMPRESSION)+IntToStr(getCompressionRatio())+"%.";
                            }
                            SetWindowText(GetDlgItem(hwnd,IDC_STATUS),(char*)tempString.c_str());
                        } else {
                            SetWindowText(GetDlgItem(hwnd,IDC_STATUS),getStringFromTable(IDS_STRING_INFO_OK));
                        }
                    }
                    else {
                        mainLastResult=noteCount;
                        SetWindowText(GetDlgItem(hwnd,IDC_STATUS),(char*)noter_getAnswerString(mainLastResult).c_str());
                        EnableWindow(GetDlgItem(hwnd,IDB_OPEN),     false);
                        EnableWindow(GetDlgItem(hwnd,IDB_DELETE),   false);
                        noteCount=0;
                    }
                    main_UnlockAllButtons(hwnd);
                    if(count==0) {
                        EnableWindow(GetDlgItem(hwnd,IDB_OPEN),     false);
                        EnableWindow(GetDlgItem(hwnd,IDB_DELETE),   false);
                    }
                    freeSelectionBuffer(selection);
                    break;
                // Create button
                case IDB_CREATE:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDB_CREATE))) {
                        break;
                    }
                    if(mainLastResult!=0) {
                        mainLastResult=0;
                    }
                    SetWindowText(GetDlgItem(hwnd,IDC_STATUS),getStringFromTable(IDS_STRING_CREATING_EDIT_WINDOW));
                    if(createEditWindow(hwnd,GetWindowWord(hwnd,GWW_HINSTANCE),winMem,NULL)==NULL) {
                        SetWindowText(GetDlgItem(hwnd,IDC_STATUS),getStringFromTable(IDS_STRING_EDITWIN_CREATE_ERROR));
                        mainLastResult=ERROR_PROGRAM_FAILURE;
                    }
                    else {
                        SetWindowText(GetDlgItem(hwnd,IDC_STATUS),(char*)noter_getAnswerString(INFO_OK).c_str());
                    }
                    break;
                // Open button
                case IDB_OPEN:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDB_OPEN))) {
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
                            SetWindowText(GetDlgItem(hwnd,IDC_STATUS),getStringFromTable(IDS_STRING_DOWNLOADING_NOTE));
                            index=selection[x];
                            if(index>=0) {
                                NOTE *note=new NOTE;
                                result=noter_getNote(connectionSettings,credentials,notes[index].id,buffer,*note);
                                if(result>=0) {
                                    HWND tempHwnd=createEditWindow(hwnd,GetWindowWord(hwnd,GWW_HINSTANCE),winMem,note);
                                    if(tempHwnd!=NULL) {
                                        tempString=noter_getAnswerString(result)+(std::string)getStringFromTable(IDS_STRING_SPACED_LAST_MOD_DATE)+toCodePage(mappedCodePage,(char*)note->lastModified.c_str())+".";
                                        compressionRatio=getCompressionRatio();
                                        if(compressionRatio!=100) {
                                            tempString=tempString+(std::string)getStringFromTable(IDS_STRING_SPACED_COMPRESSION)+IntToStr(getCompressionRatio())+"%.";
                                        }
                                        SetWindowText(GetDlgItem(tempHwnd,IDC_EDIT_STATUS),(char*)tempString.c_str());
                                        winMem[tempHwnd]->lastResult=result;
                                        SetWindowText(GetDlgItem(hwnd,IDC_STATUS),(char*)noter_getAnswerString(INFO_OK).c_str());
                                    }
                                    else {
                                        SetWindowText(GetDlgItem(hwnd,IDC_STATUS),getStringFromTable(IDS_STRING_EDITWIN_CREATE_ERROR));
                                        mainLastResult=ERROR_PROGRAM_FAILURE;
                                        ++errorCount;
                                    }
                                }
                                else {
                                    mainLastResult=result;
                                    SetWindowText(GetDlgItem(hwnd,IDC_STATUS),(char*)noter_getAnswerString(mainLastResult).c_str());
                                    ++errorCount;
                                }
                            }
                        }
                        main_UnlockAllButtons(hwnd);
                        freeSelectionBuffer(selection);
                        if(errorCount>1) {
                            SetWindowText(GetDlgItem(hwnd,IDC_STATUS),getStringFromTable(IDS_STRING_NOT_ALL_NOTES_LOADED));
                        }
                    }
                    break;
                // Close button
                case IDB_EXIT:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDB_EXIT))) {
                        break;
                    }
                    if(mainLastResult!=0) {
                        mainLastResult=0;
                        SetWindowText(GetDlgItem(hwnd,IDC_STATUS),(char*)noter_getAnswerString(mainLastResult).c_str());
                    }
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
                // Remove button
                case IDB_DELETE:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDB_DELETE))) {
                        break;
                    }
                    if(mainLastResult!=0) {
                        mainLastResult=0;
                        SetWindowText(GetDlgItem(hwnd,IDC_STATUS),(char*)noter_getAnswerString(mainLastResult).c_str());
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
                            SetWindowText(GetDlgItem(hwnd,IDC_STATUS),getStringFromTable(IDS_STRING_REMOVING_NOTE));
                            SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_SETSEL, FALSE, selection[x]);
                            mainLastResult=noter_deleteNote(connectionSettings,credentials,notes[selection[x]].id,buffer);
                            if(mainLastResult==INFO_NOTE_DELETED) {
                                selection[x]=0;
                                if(!atLeastOneDeleted) {
                                    atLeastOneDeleted=true;
                                }
                            }
                            else {
                                selection[x]=(unsigned int)notes[selection[x]].id;
                                ++errorCount;
                            }
                        }
                        main_UnlockAllButtons(hwnd);
                        if(atLeastOneDeleted) {
                            SendMessage(hwnd,WM_COMMAND,IDB_DOWNLOAD,0);
                        }
                        if((errorCount==0) || (errorCount==count)) {
                            SetWindowText(GetDlgItem(hwnd,IDC_STATUS),(char*)noter_getAnswerString(mainLastResult).c_str());
                            if(errorCount>0) {
                                selectIndexes(GetDlgItem(hwnd,ID_LISTBOX),selection,count,notes,(unsigned int)noteCount);
                            }
                        }
                        else {
                            SetWindowText(GetDlgItem(hwnd,IDC_STATUS),getStringFromTable(IDS_STRING_NOT_ALL_NOTES_REMOVED));
                            selectIndexes(GetDlgItem(hwnd,ID_LISTBOX),selection,count,notes,(unsigned int)noteCount);
                        }
                        if(SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETSELCOUNT, 0, 0)>0) {
                            EnableWindow(GetDlgItem(hwnd,IDB_OPEN),     true);
                            EnableWindow(GetDlgItem(hwnd,IDB_DELETE),   true);
                        }
                    }
                    freeSelectionBuffer(selection);
                    break;
                case ID_LISTBOX:
                    switch(HIWORD(lParam)) {
                        case LBN_DBLCLK:
                            SendMessage(hwnd, WM_COMMAND, IDB_OPEN, 0);
                            break;
                        case LBN_SELCHANGE:
                            onSelectionChange(hwnd,count,selection);
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
                SetWindowPos(GetDlgItem(hwnd,ID_LISTBOX),   NULL,0,0,(int)width,        (int)(height-88),   SWP_NOZORDER | SWP_NOMOVE);
            }
            else {
                SetWindowPos(GetDlgItem(hwnd,ID_LISTBOX),   NULL,0,0,(int)width,        (int)(height-86),   SWP_NOZORDER | SWP_NOMOVE);
            }
            SetWindowPos(GetDlgItem(hwnd,IDC_GRAYBOX),      NULL,0,0,(int)(width-400),  21,                 SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,IDC_SID),          NULL,8,  (int)(height-57),  0,0,                SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,IDC_LASTCHANGED),  NULL,8,  (int)(height-40),  0,0,                SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,IDC_NOTEID),       NULL,137,(int)(height-57),  (int)(width-146),16,SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd,IDC_NOTELASTMOD),  NULL,137,(int)(height-40),  (int)(width-146),16,SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd,IDC_STATUS),       NULL,0,  (int)(height-16),  (int)width,16,      SWP_NOZORDER);
            break;
        case WM_GETMINMAXINFO:
            lpMMI=(MINMAXINFO*)lParam;
            lpMMI->ptMinTrackSize.x=480;
            lpMMI->ptMinTrackSize.y=320;
            break;
        case WM_CLOSE:
            if(IsWindowEnabled(GetDlgItem(hwnd,IDB_EXIT))) {
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

// Editor window message processing function
LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Local variables
    unsigned long int width;
    unsigned long int height;
    unsigned long int sel;
    unsigned int result;
    int x, y, size_x, size_y;
    unsigned int state;
    MINMAXINFO *lpMMI;
    // Switch section
    switch(msg) {
        case WM_CTLCOLOR:
            switch(HIWORD(lParam)) {
                case CTLCOLOR_STATIC:
                    if((LOWORD(lParam)!=GetDlgItem(hwnd,IDC_EDIT_GRAYBOX)) && (LOWORD(lParam)!=GetDlgItem(hwnd,IDC_EDIT_STATUS))) {
                        SetBkMode((HDC)wParam, TRANSPARENT);
                        return g_hBrushBtnFace;
                    }
                    break;
                case CTLCOLOR_BTN:
                    SetBkMode((HDC)wParam, TRANSPARENT);
                    break;
            }
            break;
        case WM_INITMENU:
            if(winMem[hwnd]->note->id==0) {
                EnableMenuItem(GetMenu(hwnd),   ID_EW_FILE_PROPERTIES,  MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EW_FILE_TONEWNOTE,   MF_GRAYED);
                ModifyMenu(GetMenu(hwnd),       ID_EW_FILE_ADDUP,       MF_BYCOMMAND | MF_STRING,ID_EW_FILE_ADDUP,getStringFromTable(IDS_STRING_MENU_ADD));
                EnableMenuItem(GetMenu(hwnd),   ID_EW_FILE_ADDUP,       IsWindowEnabled(GetDlgItem(hwnd,IDB_EDIT_ADDUP))        ? MF_ENABLED : MF_GRAYED);
            }
            else {
                EnableMenuItem(GetMenu(hwnd),   ID_EW_FILE_PROPERTIES,  IsWindowEnabled(GetDlgItem(hwnd,IDB_EDIT_PROPERTIES))   ? MF_ENABLED : MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EW_FILE_TONEWNOTE,   MF_ENABLED);
                ModifyMenu(GetMenu(hwnd),       ID_EW_FILE_ADDUP,       MF_BYCOMMAND | MF_STRING,ID_EW_FILE_ADDUP,getStringFromTable(IDS_STRING_MENU_UPDATE));
                EnableMenuItem(GetMenu(hwnd),   ID_EW_FILE_ADDUP,       IsWindowEnabled(GetDlgItem(hwnd,IDB_EDIT_ADDUP))        ? MF_ENABLED : MF_GRAYED);
            }
            if((GetFocus()==GetDlgItem(hwnd,IDE_EDIT_SUBJECT)) || (GetFocus()==GetDlgItem(hwnd,IDE_EDIT_ENTRY))) {
                EnableMenuItem(GetMenu(hwnd),   ID_EW_EDIT_UNDO,        SendMessage(GetFocus(), EM_CANUNDO, 0, 0)               ? MF_ENABLED : MF_GRAYED);
                sel=SendMessage(GetFocus(),     EM_GETSEL,              0, 0);
                EnableMenuItem(GetMenu(hwnd),   ID_EW_EDIT_CUT,         (HIWORD(sel)!=LOWORD(sel)) ? MF_ENABLED : MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EW_EDIT_COPY,        (HIWORD(sel)!=LOWORD(sel)) ? MF_ENABLED : MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EW_EDIT_CLEAR,       (HIWORD(sel)!=LOWORD(sel)) ? MF_ENABLED : MF_GRAYED);
                if(OpenClipboard(GetFocus())) {
                    EnableMenuItem(GetMenu(hwnd),ID_EW_EDIT_PASTE,      (IsClipboardFormatAvailable(CF_TEXT) || IsClipboardFormatAvailable(CF_OEMTEXT)) ? MF_ENABLED : MF_GRAYED);
                    CloseClipboard();
                }
                else {
                    EnableMenuItem(GetMenu(hwnd),ID_EW_EDIT_PASTE,      MF_GRAYED);
                }
                EnableMenuItem(GetMenu(hwnd),   ID_EW_EDIT_SELECTALL,   ((GetWindowTextLength(GetFocus())>0) && (GetWindowTextLength(GetFocus())>(HIWORD(sel)-LOWORD(sel)))) ? MF_ENABLED : MF_GRAYED);
            }
            else {
                EnableMenuItem(GetMenu(hwnd),   ID_EW_EDIT_UNDO,        MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EW_EDIT_CUT,         MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EW_EDIT_COPY,        MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EW_EDIT_PASTE,       MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EW_EDIT_CLEAR,       MF_GRAYED);
                EnableMenuItem(GetMenu(hwnd),   ID_EW_EDIT_SELECTALL,   MF_GRAYED);
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
                    if(GetFocus()==GetDlgItem(hwnd,IDE_EDIT_ENTRY)) {
                        processMessages(g_Msg);
                    }
                    else {
                        SetFocus(GetNextDlgTabItem(hwnd,GetFocus(),false));
                    }
                    break;
                case ID_ACC_CTRLS:
                    SendMessage(hwnd, WM_COMMAND, ID_EW_FILE_ADDUP, 0);
                    break;
                case ID_ACC_CTRLD:
                    if(winMem[hwnd]->note->id!=0) {
                        SendMessage(hwnd, WM_COMMAND, ID_EW_FILE_TONEWNOTE, 0);
                    }
                    break;
                case ID_ACC_CTRLA:
                    SendMessage(hwnd, WM_COMMAND, ID_EW_EDIT_SELECTALL, 0);
                    break;
                case ID_ACC_ALTF4:
                    SendMessage(hwnd, WM_COMMAND, IDB_EDIT_CLOSE, 0);
                    break;
                case ID_ACC_F1:
                    SendMessage(hwnd, WM_COMMAND, ID_HELP_HELP, 0);
                    break;
                case ID_EW_FILE_ADDUP:
                    if(IsWindowEnabled(GetDlgItem(hwnd,IDB_EDIT_ADDUP))) {
                        SendMessage(hwnd, WM_COMMAND, IDB_EDIT_ADDUP, 0);
                    }
                    break;
                case ID_EW_FILE_PROPERTIES:
                    if(IsWindowEnabled(GetDlgItem(hwnd,IDB_EDIT_PROPERTIES))) {
                        SendMessage(hwnd, WM_COMMAND, IDB_EDIT_PROPERTIES, 0);
                    }
                    break;
                case ID_EW_FILE_TONEWNOTE:
                    if(winMem[hwnd]->lastResult!=0) {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    winMem[hwnd]->note->id=0;
                    EnableWindow(GetDlgItem(hwnd,IDB_EDIT_ADDUP),       true);
                    EnableWindow(GetDlgItem(hwnd,IDB_EDIT_PROPERTIES),  false);
                    SetWindowText(GetDlgItem(hwnd,IDB_EDIT_ADDUP),getStringFromTable(IDS_STRING_ADD));
                    winMem[hwnd]->subjectChanged=   true;
                    winMem[hwnd]->entryChanged=     true;
                    makeEditWindowTitle(winMem[hwnd],NULL,true,mappedCodePage);
                    SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),getStringFromTable(IDS_STRING_TO_NEW_NOTE));
                    winMem[hwnd]->lastResult=1024;
                    break;
                case ID_FILE_EXIT:
                    SendMessage(hwnd, WM_COMMAND, IDB_EDIT_CLOSE, 0);
                    break;
                case ID_EW_EDIT_UNDO:
                    if((GetFocus()==GetDlgItem(hwnd,IDE_EDIT_SUBJECT)) || (GetFocus()==GetDlgItem(hwnd,IDE_EDIT_ENTRY))) {
                        if(winMem[hwnd]->lastResult!=0) {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),WM_UNDO,0,0);
                    }
                    break;
                case ID_EW_EDIT_CUT:
                    if((GetFocus()==GetDlgItem(hwnd,IDE_EDIT_SUBJECT)) || (GetFocus()==GetDlgItem(hwnd,IDE_EDIT_ENTRY))) {
                        if(winMem[hwnd]->lastResult!=0) {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),WM_CUT,0,0);
                    }
                    break;
                case ID_EW_EDIT_COPY:
                    if((GetFocus()==GetDlgItem(hwnd,IDE_EDIT_SUBJECT)) || (GetFocus()==GetDlgItem(hwnd,IDE_EDIT_ENTRY))) {
                        if(winMem[hwnd]->lastResult!=0) {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),WM_COPY,0,0);
                    }
                    break;
                case ID_EW_EDIT_PASTE:
                    if((GetFocus()==GetDlgItem(hwnd,IDE_EDIT_SUBJECT)) || (GetFocus()==GetDlgItem(hwnd,IDE_EDIT_ENTRY))) {
                        if(winMem[hwnd]->lastResult!=0) {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),WM_PASTE,0,0);
                    }
                    break;
                case ID_EW_EDIT_CLEAR:
                    if((GetFocus()==GetDlgItem(hwnd,IDE_EDIT_SUBJECT)) || (GetFocus()==GetDlgItem(hwnd,IDE_EDIT_ENTRY))) {
                        if(winMem[hwnd]->lastResult!=0) {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),WM_CLEAR,0,0);
                    }
                    break;
                case ID_EW_EDIT_SELECTALL:
                    if((GetFocus()==GetDlgItem(hwnd,IDE_EDIT_SUBJECT)) || (GetFocus()==GetDlgItem(hwnd,IDE_EDIT_ENTRY))) {
                        if(winMem[hwnd]->lastResult!=0) {
                            winMem[hwnd]->lastResult=0;
                            SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        }
                        SendMessage(GetFocus(),EM_SETSEL,0,65535);
                    }
                    break;
                case ID_HELP_HELP:
                    if(winMem[hwnd]->lastResult!=0) {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    WinHelp(g_hwnd,getStringFromTable(IDS_HELPFILE),HELP_CONTENTS,0);
                    break;
                case ID_HELP_HOWTO:
                    if(winMem[hwnd]->lastResult!=0) {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    WinHelp(g_hwnd,"",HELP_HELPONHELP,0);
                    break;
                case ID_HELP_ABOUT:
                    if(winMem[hwnd]->lastResult!=0) {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    MakeDialogBox(hwnd,IDD_APPINFO,AboutDlgProc);
                    break;
                case IDE_EDIT_SUBJECT:
                    switch(HIWORD(lParam)) {
                        case EN_CHANGE:
                            if(winMem[hwnd]!=NULL) {
                                if(!winMem[hwnd]->subjectChanged) {
                                    winMem[hwnd]->subjectChanged=true;
                                    if(winMem[hwnd]->note->id==0) {
                                        if(winMem[hwnd]->entryChanged) {
                                            EnableWindow(GetDlgItem(hwnd,IDB_EDIT_ADDUP),true);
                                        }
                                    }
                                    else {
                                        EnableWindow(GetDlgItem(hwnd,IDB_EDIT_ADDUP),true);
                                    }
                                }
                                if(winMem[hwnd]->lastResult!=0) {
                                    winMem[hwnd]->lastResult=0;
                                    SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                                }
                            }
                            break;
                    }
                    break;
                case IDE_EDIT_ENTRY:
                    switch(HIWORD(lParam)) {
                        case EN_CHANGE:
                            if(winMem[hwnd]!=NULL) {
                                if(!winMem[hwnd]->entryChanged) {
                                    winMem[hwnd]->entryChanged=true;
                                    if(winMem[hwnd]->note->id==0) {
                                        if(winMem[hwnd]->subjectChanged) {
                                            EnableWindow(GetDlgItem(hwnd,IDB_EDIT_ADDUP),true);
                                        }
                                    }
                                    else {
                                        EnableWindow(GetDlgItem(hwnd,IDB_EDIT_ADDUP),true);
                                    }
                                }
                                if(winMem[hwnd]->lastResult!=0) {
                                    winMem[hwnd]->lastResult=0;
                                    SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                                }
                            }
                            break;
                    }
                    break;
                case IDB_EDIT_ADDUP:
                    bool subEncErr=false, entEncErr=false;
                    if((!IsWindowEnabled(GetDlgItem(g_hwnd,IDB_EXIT))) || (!IsWindowEnabled(GetDlgItem(hwnd,IDB_EDIT_ADDUP)))) {
                        break;
                    }
                    if(winMem[hwnd]->lastResult!=0) {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    GetWindowText(GetDlgItem(hwnd,IDE_EDIT_SUBJECT),buffer,65535);
                    winMem[hwnd]->note->subject=fromCodePage(rawCodePage,buffer);
                    subEncErr=encodeWarningState();
                    GetWindowText(GetDlgItem(hwnd,IDE_EDIT_ENTRY),  buffer,65535);
                    winMem[hwnd]->note->entry=fromCodePage(rawCodePage,buffer);
                    entEncErr=encodeWarningState();
                    if(winMem[hwnd]->note->id==0) {
                        edit_LockAllButtons(g_hwnd,hwnd);
                        winMem[hwnd]->lastResult=noter_addNote(connectionSettings,credentials,*winMem[hwnd]->note,buffer);
                        edit_UnlockAllButtons(hwnd);
                        SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        if(winMem[hwnd]->lastResult>=0) {
                            if(mainSettings.autoRefresh) {
                                SendMessage(g_hwnd,WM_COMMAND,IDB_DOWNLOAD,0);
                            }
                            SetWindowText(GetDlgItem(hwnd,IDB_EDIT_ADDUP),getStringFromTable(IDS_STRING_UPDATE));
                            winMem[hwnd]->subjectChanged=false;
                            winMem[hwnd]->entryChanged=  false;
                            makeEditWindowTitle(winMem[hwnd],winMem[hwnd]->note,true,mappedCodePage);
                        }
                        else {
                            EnableWindow(GetDlgItem(hwnd,IDB_EDIT_ADDUP),true);
                        }
                    }
                    else {
                        edit_LockAllButtons(g_hwnd,hwnd);
                        winMem[hwnd]->lastResult=noter_updateNote(connectionSettings,credentials,*winMem[hwnd]->note,buffer);
                        edit_UnlockAllButtons(hwnd);
                        SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                        if(winMem[hwnd]->lastResult>=0) {
                            if(mainSettings.autoRefresh) {
                                SendMessage(g_hwnd,WM_COMMAND,IDB_DOWNLOAD,0);
                            }
                            winMem[hwnd]->subjectChanged=false;
                            winMem[hwnd]->entryChanged=  false;
                            makeEditWindowTitle(winMem[hwnd],winMem[hwnd]->note,true,mappedCodePage);
                        }
                        else {
                            EnableWindow(GetDlgItem(hwnd,IDB_EDIT_ADDUP),true);
                        }
                    }
                    if(subEncErr || entEncErr) {
                        std::string preparedMessage=getStringFromTable(IDS_STRING_SOME_CHARACTERS);
                        if(subEncErr) {
                            preparedMessage+=getStringFromTable(IDS_STRING_OF_SUBJECT);
                        }
                        if(subEncErr && entEncErr) {
                            preparedMessage+=getStringFromTable(IDS_STRING_BOTH_SPACED_AND);
                        }
                        if(entEncErr) {
                            preparedMessage+=getStringFromTable(IDS_STRING_OF_ENTRY);
                        }
                        preparedMessage+=getStringFromTable(IDS_STRING_COULD_NOT_CONVERT_LOSS);
                        MessageBox(hwnd,preparedMessage.c_str(),getStringFromTable(IDS_APPNAME,1),MB_ICONINFORMATION);
                    }
                    break;
                case IDB_EDIT_PROPERTIES:
                    if((!IsWindowEnabled(GetDlgItem(g_hwnd,IDB_EXIT))) || (!IsWindowEnabled(GetDlgItem(hwnd,IDB_EDIT_PROPERTIES)))) {
                        break;
                    }
                    if(winMem[hwnd]->lastResult!=0) {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    MakeDialogBox(hwnd,IDD_NOTEINFO,NotePropDlgProc);
                    break;
                case IDB_EDIT_CLOSE:
                    if((!IsWindowEnabled(GetDlgItem(g_hwnd,IDB_EXIT))) || (!IsWindowEnabled(GetDlgItem(hwnd,IDB_EDIT_CLOSE)))) {
                        break;
                    }
                    if(winMem[hwnd]->lastResult!=0) {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,IDC_EDIT_STATUS),(char*)noter_getAnswerString(winMem[hwnd]->lastResult).c_str());
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
            SetWindowPos(GetDlgItem(hwnd,IDC_EDIT_SUBJECT),     NULL,0,0,(int)width,16,                         SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,IDE_EDIT_SUBJECT),     NULL,0,0,(int)width,24,                         SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,IDC_EDIT_ENTRY),       NULL,0,0,(int)width,16,                         SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,IDE_EDIT_ENTRY),       NULL,0,0,(int)width,(int)(height-93),           SWP_NOZORDER | SWP_NOMOVE);
            SetWindowPos(GetDlgItem(hwnd,IDB_EDIT_ADDUP),       NULL,0,  (int)(height-37),0,0,                  SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,IDB_EDIT_PROPERTIES),  NULL,96, (int)(height-37),0,0,                  SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,IDB_EDIT_CLOSE),       NULL,192,(int)(height-37),0,0,                  SWP_NOZORDER | SWP_NOSIZE);
            SetWindowPos(GetDlgItem(hwnd,IDC_EDIT_GRAYBOX),     NULL,288,(int)(height-37),(int)(width-288),21,  SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd,IDC_EDIT_STATUS),      NULL,0,  (int)(height-16),(int)width,16,        SWP_NOZORDER);
            break;
        case WM_GETMINMAXINFO:
            lpMMI=(MINMAXINFO*)lParam;
            lpMMI->ptMinTrackSize.x=480;
            lpMMI->ptMinTrackSize.y=320;
            break;
        case WM_CLOSE:
            if((!IsWindowEnabled(GetDlgItem(g_hwnd,IDB_EXIT))) || (!IsWindowEnabled(GetDlgItem(hwnd,IDB_EDIT_CLOSE)))) {
                break;
            }
            if((winMem[hwnd]!=NULL) && (winMem[hwnd]->subjectChanged || winMem[hwnd]->entryChanged)) {
                result=MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WANT_CHANGES_SAVED),getStringFromTable(IDS_APPNAME,1),MB_ICONEXCLAMATION | MB_YESNOCANCEL);
            }
            else {
                result=IDNO;
            }
            if(result==IDYES) {
                SendMessage(hwnd,WM_COMMAND,IDB_EDIT_ADDUP,0);
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

// Note properties dialog message processing function
BOOL CALLBACK NotePropDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Local variables
    NOTE tempNote;
    long int result;
    bool addUpTest=false;
    // Switch section
    switch(msg) {
        case WM_INITDIALOG:
            addUpTest=IsWindowEnabled(GetDlgItem(GetParent(hwnd),IDB_EDIT_ADDUP));
            properties_LockAllButtons(g_hwnd,hwnd);
            result=noter_getNote(connectionSettings,credentials,winMem[GetParent(hwnd)]->note->id,buffer,tempNote);
            properties_UnlockAllButtons(hwnd);
            EnableWindow(GetDlgItem(GetParent(hwnd),IDB_EDIT_ADDUP),addUpTest);
            if(result>=0) {
                SetWindowText(GetDlgItem(hwnd,IDC_NOTENAMESTATIC),          (char*)toCodePage(mappedCodePage,(char*)tempNote.subject.c_str()).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_NOTEIDSTATIC),            (char*)IntToStr(tempNote.id).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_NOTEADDDATESTATIC),       (char*)toCodePage(mappedCodePage,(char*)tempNote.dateAdded.c_str()).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_NOTEADDEDUSINGSTATIC),    (char*)toCodePage(mappedCodePage,(char*)tempNote.userAgent.c_str()).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_NOTELASTMODSTATIC),       (char*)toCodePage(mappedCodePage,(char*)tempNote.lastModified.c_str()).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_NOTELASTMODUSINGSTATIC),  (char*)toCodePage(mappedCodePage,(char*)tempNote.lastUserAgent.c_str()).c_str());
                EnableWindow(GetDlgItem(hwnd,IDC_LOCKBUTTON),               !tempNote.locked);
                EnableWindow(GetDlgItem(hwnd,IDC_UNLOCKBUTTON),              tempNote.locked);
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
                case IDC_LOCKBUTTON:
                    result=noter_lockNote(connectionSettings,credentials,winMem[GetParent(hwnd)]->note->id,buffer);
                    if(result>=0) {
                        winMem[GetParent(hwnd)]->note->locked=true;
                        tempNote.locked=true;
                        EnableWindow(GetDlgItem(hwnd,IDC_LOCKBUTTON),   false);
                        EnableWindow(GetDlgItem(hwnd,IDC_UNLOCKBUTTON), true);
                    }
                    else {
                        MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),getStringFromTable(IDS_APPNAME,1),MB_ICONHAND | MB_OK);
                    }
                    break;
                case IDC_UNLOCKBUTTON:
                    result=noter_unlockNote(connectionSettings,credentials,winMem[GetParent(hwnd)]->note->id,buffer);
                    if(result>=0) {
                        winMem[GetParent(hwnd)]->note->locked=false;
                        tempNote.locked=false;
                        EnableWindow(GetDlgItem(hwnd,IDC_LOCKBUTTON),   true);
                        EnableWindow(GetDlgItem(hwnd,IDC_UNLOCKBUTTON), false);
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

// Program information dialog message processing function
BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Switch section
    switch(msg) {
        case WM_INITDIALOG:
            SetWindowText(GetDlgItem(hwnd,IDC_APPNAMESTATIC),getStringFromTable(IDS_APPNAME));
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

// Preferences dialog message processing function
BOOL CALLBACK PreferencesDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Local variables
    bool enabled;
    std::string iniFile;
    LIB_ITER lIt;
    unsigned int counter, counter2, selectedIndex, selectedIndex2;
    POINT mousePosition;
    RECT tempRect;
    HDC hdc;
    // Switch section
    switch(msg) {
        case WM_INITDIALOG:
            check3DChanged=false;
            SetWindowText(GetDlgItem(hwnd,IDC_BUILDINFOSTATIC),((std::string)(getStringFromTable(IDS_STRING_BUILD_DATE))+__DATE__+", "+__TIME__).c_str());
            SendMessage(GetDlgItem(hwnd, IDC_MAINWINDISPCOMBO), CB_ADDSTRING, 0, (LPARAM)getStringFromTable(IDS_STRING_NORMAL_WINDOW));
            SendMessage(GetDlgItem(hwnd, IDC_MAINWINDISPCOMBO), CB_ADDSTRING, 0, (LPARAM)getStringFromTable(IDS_STRING_MINIMIZED_WINDOW));
            SendMessage(GetDlgItem(hwnd, IDC_MAINWINDISPCOMBO), CB_ADDSTRING, 0, (LPARAM)getStringFromTable(IDS_STRING_MAXIMIZED_WINDOW));
            SendMessage(GetDlgItem(hwnd, IDC_EDITWINDISPCOMBO), CB_ADDSTRING, 0, (LPARAM)getStringFromTable(IDS_STRING_NORMAL_WINDOW));
            SendMessage(GetDlgItem(hwnd, IDC_EDITWINDISPCOMBO), CB_ADDSTRING, 0, (LPARAM)getStringFromTable(IDS_STRING_MINIMIZED_WINDOW));
            SendMessage(GetDlgItem(hwnd, IDC_EDITWINDISPCOMBO), CB_ADDSTRING, 0, (LPARAM)getStringFromTable(IDS_STRING_MAXIMIZED_WINDOW));
            SendMessage(GetDlgItem(hwnd, IDC_LANGUAGECOMBO),    CB_ADDSTRING, 0, (LPARAM)getStringFromTable(IDS_STRING_BUILTIN_LANGUAGE));
            // begin temporary part
            SendMessage(GetDlgItem(hwnd, IDC_LANGUAGECOMBO),    CB_SETCURSEL, 0, 0);
            // end temporary part
            counter=0;
            counter2=0;
            selectedIndex=0;
            selectedIndex2=0;
            for(lIt=libraries.begin(); lIt!=libraries.end(); ++lIt) {
                if(lIt->type==LIB_CODEPAGE) {
                    SendMessage(GetDlgItem(hwnd, IDC_CODEPAGECOMBO), CB_ADDSTRING, 0, (LPARAM)(char*)((lIt->relatedInfo)+" ["+(lIt->filename)+"]").c_str());
                    if(lIt->filename==mainSettings.selectedCodePage) {
                        selectedIndex=counter;
                    }
                    ++counter;
                }
                else if(lIt->type==LIB_STRINGTABLE) {
                    SendMessage(GetDlgItem(hwnd, IDC_LANGUAGECOMBO), CB_ADDSTRING, 0, (LPARAM)(char*)((lIt->relatedInfo)+" ["+(lIt->filename)+"]").c_str());
                    // part to do
                    ++counter2;
                }
            }
            SendMessage(GetDlgItem(hwnd, IDC_CODEPAGECOMBO),    CB_SETCURSEL, selectedIndex, 0);
            SendMessage(GetDlgItem(hwnd, IDC_MAINWINDISPCOMBO), CB_SETCURSEL, mainSettings.mainWindowStyle, 0);
            SendMessage(GetDlgItem(hwnd, IDC_EDITWINDISPCOMBO), CB_SETCURSEL, mainSettings.editWindowStyle, 0);
            SendMessage(GetDlgItem(hwnd, IDC_MAINWINDISPCOMBO), WM_PAINT,     0, 0);
            SendMessage(GetDlgItem(hwnd, IDC_EDITWINDISPCOMBO), WM_PAINT,     0, 0);
            SendMessage(GetDlgItem(hwnd, IDC_LANGUAGECOMBO),    WM_PAINT,     0, 0);
            SendMessage(GetDlgItem(hwnd, IDC_CODEPAGECOMBO),    WM_PAINT,     0, 0);
            if(mainSettings.mainWindowSystem) {
                CheckRadioButton(hwnd, IDC_SYSDISPRADIO,        IDC_MAINDISPASRADIO, IDC_SYSDISPRADIO);
                EnableWindow(GetDlgItem(hwnd,IDC_MAINWINDISPCOMBO),false);
            }
            else {
                CheckRadioButton(hwnd, IDC_SYSDISPRADIO,        IDC_MAINDISPASRADIO, IDC_MAINDISPASRADIO);
                EnableWindow(GetDlgItem(hwnd,IDC_MAINWINDISPCOMBO),true);
            }
            if(mainSettings.editWindowSystem) {
                CheckRadioButton(hwnd, IDC_EDITDISPASMAINRADIO, IDC_EDITDISPASRADIO, IDC_EDITDISPASMAINRADIO);
                EnableWindow(GetDlgItem(hwnd,IDC_EDITWINDISPCOMBO),false);
            }
            else {
                CheckRadioButton(hwnd, IDC_EDITDISPASMAINRADIO, IDC_EDITDISPASRADIO, IDC_EDITDISPASRADIO);
                EnableWindow(GetDlgItem(hwnd,IDC_EDITWINDISPCOMBO),true);
            }
            CheckDlgButton(hwnd, IDC_AUTORELOADCHECK,       mainSettings.autoReload     ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_REFRESHONADDCHECK,     mainSettings.autoRefresh    ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_SAVEWINPOSCHECK,       mainSettings.savePosSizes   ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_USE3DCONTROLSCHECK,    mainSettings.use3DControls  ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_BUTTONSCHECK,          mainSettings.use3DButtons   ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_LISTSCHECK,            mainSettings.use3DLists     ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_EDITSCHECK,            mainSettings.use3DEdits     ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_COMBOSCHECK,           mainSettings.use3DCombos    ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_DIALOGSCHECK,          mainSettings.use3DDialogs   ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_MULTIIDSNLMCHECK,      mainSettings.showMultiIDnLM ? BST_CHECKED : BST_UNCHECKED);
            enabled=IsDlgButtonChecked(hwnd,                IDC_USE3DCONTROLSCHECK)==1;
            EnableWindow(GetDlgItem(hwnd,IDC_BUTTONSCHECK), enabled);
            EnableWindow(GetDlgItem(hwnd,IDC_LISTSCHECK),   enabled);
            EnableWindow(GetDlgItem(hwnd,IDC_EDITSCHECK),   enabled);
            EnableWindow(GetDlgItem(hwnd,IDC_COMBOSCHECK),  enabled);
            EnableWindow(GetDlgItem(hwnd,IDC_DIALOGSCHECK), enabled);
            break;
        case WM_COMMAND:
            switch(wParam) {
                case IDOK:
                    GetModuleFileName(GetWindowWord(hwnd,GWW_HINSTANCE),buffer,32767);
                    iniFile=getDefaultIniFile(buffer);
                    mainSettings.mainWindowSystem=IsDlgButtonChecked(hwnd, IDC_SYSDISPRADIO)==1;
                    mainSettings.editWindowSystem=IsDlgButtonChecked(hwnd, IDC_EDITDISPASMAINRADIO)==1;
                    mainSettings.mainWindowStyle=(unsigned int)SendMessage(GetDlgItem(hwnd,IDC_MAINWINDISPCOMBO), CB_GETCURSEL, 0, 0);
                    mainSettings.editWindowStyle=(unsigned int)SendMessage(GetDlgItem(hwnd,IDC_EDITWINDISPCOMBO), CB_GETCURSEL, 0, 0);
                    mainSettings.autoReload= IsDlgButtonChecked(hwnd, IDC_AUTORELOADCHECK)==1;
                    mainSettings.autoRefresh=IsDlgButtonChecked(hwnd, IDC_REFRESHONADDCHECK)==1;
                    if((mainSettings.savePosSizes==false) && (IsDlgButtonChecked(hwnd, IDC_SAVEWINPOSCHECK))) {
                        mainSettings.mainWindowX=       CW_USEDEFAULT;
                        mainSettings.mainWindowY=       CW_USEDEFAULT;
                        mainSettings.mainWindowSizeX=   CW_USEDEFAULT;
                        mainSettings.mainWindowSizeY=   CW_USEDEFAULT;
                        mainSettings.editWindowX=       CW_USEDEFAULT;
                        mainSettings.editWindowY=       CW_USEDEFAULT;
                        mainSettings.editWindowSizeX=   CW_USEDEFAULT;
                        mainSettings.editWindowSizeY=   CW_USEDEFAULT;
                    }
                    mainSettings.savePosSizes=  IsDlgButtonChecked(hwnd, IDC_SAVEWINPOSCHECK)==1;
                    mainSettings.use3DControls= IsDlgButtonChecked(hwnd, IDC_USE3DCONTROLSCHECK)==1;
                    mainSettings.use3DButtons=  IsDlgButtonChecked(hwnd, IDC_BUTTONSCHECK)==1;
                    mainSettings.use3DLists=    IsDlgButtonChecked(hwnd, IDC_LISTSCHECK)==1;
                    mainSettings.use3DEdits=    IsDlgButtonChecked(hwnd, IDC_EDITSCHECK)==1;
                    mainSettings.use3DCombos=   IsDlgButtonChecked(hwnd, IDC_COMBOSCHECK)==1;
                    mainSettings.use3DDialogs=  IsDlgButtonChecked(hwnd, IDC_DIALOGSCHECK)==1;
                    mainSettings.showMultiIDnLM=IsDlgButtonChecked(hwnd, IDC_MULTIIDSNLMCHECK)==1;
                    counter=0;
                    counter2=0;
                    selectedIndex2=(unsigned int)SendMessage(GetDlgItem(hwnd,IDC_LANGUAGECOMBO), CB_GETCURSEL, 0, 0);
                    selectedIndex= (unsigned int)SendMessage(GetDlgItem(hwnd,IDC_CODEPAGECOMBO), CB_GETCURSEL, 0, 0);
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
                case IDC_SYSDISPRADIO:
                    CheckRadioButton(hwnd, IDC_SYSDISPRADIO,        IDC_MAINDISPASRADIO, IDC_SYSDISPRADIO);
                    EnableWindow(GetDlgItem(hwnd,IDC_MAINWINDISPCOMBO),false);
                    break;
                case IDC_MAINDISPASRADIO:
                    CheckRadioButton(hwnd, IDC_SYSDISPRADIO,        IDC_MAINDISPASRADIO, IDC_MAINDISPASRADIO);
                    EnableWindow(GetDlgItem(hwnd,IDC_MAINWINDISPCOMBO),true);
                    break;
                case IDC_EDITDISPASMAINRADIO:
                    CheckRadioButton(hwnd, IDC_EDITDISPASMAINRADIO, IDC_EDITDISPASRADIO, IDC_EDITDISPASMAINRADIO);
                    EnableWindow(GetDlgItem(hwnd,IDC_EDITWINDISPCOMBO),false);
                    break;
                case IDC_EDITDISPASRADIO:
                    CheckRadioButton(hwnd, IDC_EDITDISPASMAINRADIO, IDC_EDITDISPASRADIO, IDC_EDITDISPASRADIO);
                    EnableWindow(GetDlgItem(hwnd,IDC_EDITWINDISPCOMBO),true);
                    break;
                case IDC_USE3DCONTROLSCHECK:
                    check3DChanged=true;
                    enabled=IsDlgButtonChecked(hwnd, IDC_USE3DCONTROLSCHECK)==1;
                    EnableWindow(GetDlgItem(hwnd,IDC_BUTTONSCHECK), enabled);
                    EnableWindow(GetDlgItem(hwnd,IDC_LISTSCHECK),   enabled);
                    EnableWindow(GetDlgItem(hwnd,IDC_EDITSCHECK),   enabled);
                    EnableWindow(GetDlgItem(hwnd,IDC_COMBOSCHECK),  enabled);
                    EnableWindow(GetDlgItem(hwnd,IDC_DIALOGSCHECK), enabled);
                    break;
                case IDC_BUTTONSCHECK:
                case IDC_LISTSCHECK:
                case IDC_EDITSCHECK:
                case IDC_COMBOSCHECK:
                case IDC_DIALOGSCHECK:
                    check3DChanged=true;
                    break;
            }
            break;
        case WM_LBUTTONDOWN:
            GetWindowRect(GetDlgItem(hwnd,IDC_CODEPAGESTATIC),&tempRect);
            GetCursorPos(&mousePosition);
            if(PtInRect(&tempRect,mousePosition)) {
                if(!cpClick) {
                    cpClick=true;
                }
            }
            break;
        case WM_LBUTTONUP:
            GetWindowRect(GetDlgItem(hwnd,IDC_CODEPAGESTATIC),&tempRect);
            GetCursorPos(&mousePosition);
            if(PtInRect(&tempRect,mousePosition)) {
                if(cpClick) {
                    MakeDialogBoxParam(hwnd,IDD_LIBINFO,LibInfoDlgProc,(LPARAM)hCodePageLib);
                    cpHover=false;
                    InvalidateRect(GetDlgItem(hwnd,IDC_CODEPAGESTATIC),NULL,TRUE);
                    SendMessage(GetDlgItem(hwnd,IDC_CODEPAGESTATIC),WM_SETFONT,(WPARAM)(HFONT)GetStockObject(SYSTEM_FONT),0);
                }
            }
            if(cpClick) {
                cpClick=false;
            }
            break;
        case WM_MOUSEMOVE:
            GetWindowRect(GetDlgItem(hwnd,IDC_CODEPAGESTATIC),&tempRect);
            GetCursorPos(&mousePosition);
            if(PtInRect(&tempRect,mousePosition)) {
                if(!cpHover) {
                    cpHover=true;
                    InvalidateRect(GetDlgItem(hwnd,IDC_CODEPAGESTATIC),NULL,TRUE);
                    SendMessage(GetDlgItem(hwnd,IDC_CODEPAGESTATIC),WM_SETFONT,(WPARAM)hUnderlinedFont,0);
                }
            }
            else {
                if(cpHover) {
                    cpHover=false;
                    InvalidateRect(GetDlgItem(hwnd,IDC_CODEPAGESTATIC),NULL,TRUE);
                    SendMessage(GetDlgItem(hwnd,IDC_CODEPAGESTATIC),WM_SETFONT,(WPARAM)(HFONT)GetStockObject(SYSTEM_FONT),0);
                }
            }
            break;
        case WM_CTLCOLOR:
            if(HIWORD(lParam)==CTLCOLOR_STATIC) {
                if(LOWORD(lParam)==GetDlgItem(hwnd,IDC_CODEPAGESTATIC)) {
                    hdc=(HDC)wParam;
                    if(cpHover) {
                        SetTextColor(hdc,RGB(0,0,255));
                    }
                    else {
                        SetTextColor(hdc,getBrushColor(g_hBrushWinText));
                    }
                    SetBkMode(hdc,TRANSPARENT);
                    if(Ctl3dEnabled() && mainSettings.use3DControls && mainSettings.use3DDialogs) {
                        return (LRESULT)g_hBrushBtnFace;
                    }
                    else {
                        return (LRESULT)g_hBrushWindow;
                    }
                }
            }
            break;
        case WM_CLOSE:
            EndDialog(hwnd,IDCANCEL);
            break;
        case WM_DESTROY:
            cpHover=false;
            cpClick=false;
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

// Connection settings dialog message processing function
BOOL CALLBACK ConnSettDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Local variables
    NOTER_SERVER_INFO serverInfo;
    NOTER_CONNECTION_SETTINGS tempConnectionSettings;
    char* ipAddress=NULL;
    std::string iniFile, serverAddress;
    unsigned int port;
    // Switch section
    switch(msg) {
        case WM_INITDIALOG:
            if(noter_connectionSettingsAvailable(connectionSettings)) {
                SetWindowText(GetDlgItem(hwnd,IDC_ADDRESSEDIT), (char*)connectionSettings.serverAddress.c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_PORTEDIT),    (char*)IntToStr(connectionSettings.port).c_str());
                SetWindowText(GetDlgItem(hwnd,IDC_SHAREEDIT),   (char*)connectionSettings.share.c_str());
                CheckDlgButton(hwnd,IDC_COMPRESSIONCHECK,connectionSettings.requestCompression ? BST_CHECKED : BST_UNCHECKED);
                connection_LockAllButtons(hwnd);
                serverInfo=noter_getServerInfo(connectionSettings,buffer);
                connection_UnlockAllButtons(hwnd);
                if(serverInfo.version==MATCH_VERSION) {
                    SetWindowText(GetDlgItem(hwnd,IDC_SERVERNAMESTATIC),    (char*)toCodePage(mappedCodePage,(char*)serverInfo.name.c_str()).c_str());
                    SetWindowText(GetDlgItem(hwnd,IDC_SERVERTIMEZONESTATIC),(char*)toCodePage(mappedCodePage,(char*)serverInfo.timezone.c_str()).c_str());
                    SetWindowText(GetDlgItem(hwnd,IDC_SERVERVERSIONSTATIC), (char*)toCodePage(mappedCodePage,(char*)serverInfo.version.c_str()).c_str());
                }
            }
            else {
                EnableWindow(GetDlgItem(hwnd,IDOK),             false);
                EnableWindow(GetDlgItem(hwnd,IDC_TESTBUTTON),   false);
            }
            editsChanged=false;
            break;
        case WM_COMMAND:
            switch(wParam) {
                case IDOK:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDOK))) {
                        break;
                    }
                    GetWindowText(GetDlgItem(hwnd,IDC_PORTEDIT),buffer,65535);
                    trimChar(buffer);
                    if(checkIfInt(buffer)) {
                        if(editsChanged) {
                            GetWindowText(GetDlgItem(hwnd,IDC_ADDRESSEDIT), buffer,65535);
                            trimChar(buffer);
                            if(buffer[0]==0) {
                                MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_SERVER_ADDRESS),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                            }
                            else {
                                serverAddress=buffer;
                                GetWindowText(GetDlgItem(hwnd,IDC_PORTEDIT),    buffer,65535);
                                trimChar(buffer);
                                if(buffer[0]==0) {
                                    MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_PORT_NUMBER),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                                }
                                else {
                                    port=StrToInt(buffer);
                                    GetWindowText(GetDlgItem(hwnd,IDC_SHAREEDIT),   buffer,65535);
                                    trimChar(buffer);
                                    if(buffer[0]==0) {
                                        MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_SHARE),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                                    }
                                    else {
                                        connectionSettings.serverAddress=serverAddress;
                                        connectionSettings.port=port;
                                        connectionSettings.share=buffer;
                                        connectionSettings.requestCompression=IsDlgButtonChecked(hwnd, IDC_COMPRESSIONCHECK)==1;
                                        GetModuleFileName(GetWindowWord(hwnd,GWW_HINSTANCE),buffer,32767);
                                        iniFile=getDefaultIniFile(buffer);
                                        saveConnectionSettings(connectionSettings,(char*)iniFile.c_str());
                                        EndDialog(hwnd,IDOK);
                                    }
                                }
                            }
                        }
                        else {
                            EndDialog(hwnd,IDOK);
                        }
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
                case IDC_COMPRESSIONCHECK:
                    editsChanged=true;  // yeah, a bit ugly...
                    break;
                case IDC_TESTBUTTON:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDC_TESTBUTTON))) {
                        break;
                    }
                    GetWindowText(GetDlgItem(hwnd,IDC_PORTEDIT),buffer,65535);
                    trimChar(buffer);
                    if(checkIfInt(buffer)) {
                        GetWindowText(GetDlgItem(hwnd,IDC_ADDRESSEDIT), buffer,65535);
                        trimChar(buffer);
                        if(buffer[0]==0) {
                            MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_SERVER_ADDRESS),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                        }
                        else {
                            tempConnectionSettings.serverAddress=buffer;
                            GetWindowText(GetDlgItem(hwnd,IDC_PORTEDIT),    buffer,65535);
                            trimChar(buffer);
                            if(buffer[0]==0) {
                                MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_PORT_NUMBER),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                            }
                            else {
                                tempConnectionSettings.port=StrToInt(buffer);
                                GetWindowText(GetDlgItem(hwnd,IDC_SHAREEDIT),   buffer,65535);
                                trimChar(buffer);
                                if(buffer[0]==0) {
                                    MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_SHARE),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                                }
                                else {
                                    tempConnectionSettings.share=buffer;
                                    connection_LockAllButtons(hwnd);
                                    serverInfo=noter_getServerInfo(tempConnectionSettings,buffer);
                                    connection_UnlockAllButtons(hwnd);
                                    if(noter_checkServerVersion(serverInfo)) {
                                        SetWindowText(GetDlgItem(hwnd,IDC_SERVERNAMESTATIC),    (char*)toCodePage(mappedCodePage,(char*)serverInfo.name.c_str()).c_str());
                                        SetWindowText(GetDlgItem(hwnd,IDC_SERVERTIMEZONESTATIC),(char*)toCodePage(mappedCodePage,(char*)serverInfo.timezone.c_str()).c_str());
                                        SetWindowText(GetDlgItem(hwnd,IDC_SERVERVERSIONSTATIC), (char*)toCodePage(mappedCodePage,(char*)serverInfo.version.c_str()).c_str());
                                        MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_CONN_ESTABLISHED),getStringFromTable(IDS_STRING_INFORMATION,1),MB_ICONINFORMATION | MB_OK);
                                    }
                                    else {
                                        SetWindowText(GetDlgItem(hwnd,IDC_SERVERNAMESTATIC),    getStringFromTable(IDS_STRING_NOT_CONNECTED));
                                        SetWindowText(GetDlgItem(hwnd,IDC_SERVERTIMEZONESTATIC),getStringFromTable(IDS_STRING_NOT_CONNECTED));
                                        SetWindowText(GetDlgItem(hwnd,IDC_SERVERVERSIONSTATIC), getStringFromTable(IDS_STRING_NOT_CONNECTED));
                                        MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_CONNECTION_ERROR),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                                    }
                                }
                            }
                        }
                    }
                    else {
                        MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_PORT_NUMBER),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                    }
                    break;
                case IDC_ADDRESSEDIT:
                case IDC_PORTEDIT:
                case IDC_SHAREEDIT:
                    switch(HIWORD(lParam)) {
                        case EN_CHANGE:
                            if(GetWindowTextLength(GetDlgItem(hwnd,wParam))==0) {
                                EnableWindow(GetDlgItem(hwnd,IDOK),             false);
                                EnableWindow(GetDlgItem(hwnd,IDC_TESTBUTTON),   false);
                            }
                            else {
                                if((GetWindowTextLength(GetDlgItem(hwnd,IDC_ADDRESSEDIT))>0) && (GetWindowTextLength(GetDlgItem(hwnd,IDC_PORTEDIT))>0) && (GetWindowTextLength(GetDlgItem(hwnd,IDC_SHAREEDIT))>0)) {
                                    EnableWindow(GetDlgItem(hwnd,IDOK),             true);
                                    EnableWindow(GetDlgItem(hwnd,IDC_TESTBUTTON),   true);
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

// Credentials settings dialog message processing procedure
BOOL CALLBACK CredsSettDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Local variables
    USER_INFO userInfo;
    long int result;
    std::string tempString, iniFile, username;
    // Switch section
    switch(msg) {
        case WM_INITDIALOG:
            if(noter_connectionSettingsAvailable(connectionSettings)) {
                if(noter_credentialsAvailable(credentials)) {
                    SetWindowText(GetDlgItem(hwnd,IDC_USERNAMEEDIT),(char*)credentials.username.c_str());
                    SetWindowText(GetDlgItem(hwnd,IDC_PASSWORDEDIT),(char*)credentials.password.c_str());
                    credentials_LockAllButtons(hwnd);
                    result=noter_getUserInfo(connectionSettings,credentials,buffer,userInfo);
                    credentials_UnlockAllButtons(hwnd);
                    if(result>=0) {
                        SetWindowText(GetDlgItem(hwnd,IDC_USERIDSTATIC),            (char*)IntToStr(userInfo.ID).c_str());
                        SetWindowText(GetDlgItem(hwnd,IDC_USERREGDATESTATIC),       (char*)userInfo.dateRegistered.c_str());
                        SetWindowText(GetDlgItem(hwnd,IDC_USERREGUSINGSTATIC),      (char*)userInfo.userAgent.c_str());
                        SetWindowText(GetDlgItem(hwnd,IDC_USERLASTMODSTATIC),       (char*)userInfo.lastChanged.c_str());
                        SetWindowText(GetDlgItem(hwnd,IDC_USERLASTMODUSINGSTATIC),  (char*)userInfo.lastUserAgent.c_str());
                        EnableWindow(GetDlgItem(hwnd,IDC_ACCDELETEBUTTON),          true);
                        EnableWindow(GetDlgItem(hwnd,IDC_PASSCHANGEBUTTON),         true);
                    }
                    else {
                        EnableWindow(GetDlgItem(hwnd,IDC_ACCDELETEBUTTON),  false);
                        EnableWindow(GetDlgItem(hwnd,IDC_PASSCHANGEBUTTON), false);
                    }
                }
                else {
                    EnableWindow(GetDlgItem(hwnd,IDOK),                 false);
                    EnableWindow(GetDlgItem(hwnd,IDC_TESTBUTTON2),      false);
                    EnableWindow(GetDlgItem(hwnd,IDC_ACCDELETEBUTTON),  false);
                    EnableWindow(GetDlgItem(hwnd,IDC_PASSCHANGEBUTTON), false);
                }
            }
            else {
                EnableWindow(GetDlgItem(hwnd,IDOK),                 false);
                EnableWindow(GetDlgItem(hwnd,IDC_REGISTERBUTTON),   false);
                EnableWindow(GetDlgItem(hwnd,IDC_TESTBUTTON2),      false);
                EnableWindow(GetDlgItem(hwnd,IDC_ACCDELETEBUTTON),  false);
                EnableWindow(GetDlgItem(hwnd,IDC_PASSCHANGEBUTTON), false);
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
                        GetWindowText(GetDlgItem(hwnd,IDC_USERNAMEEDIT),buffer,65535);
                        trimChar(buffer);
                        if(buffer[0]==0) {
                            MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_USERNAME),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                        }
                        else {
                            username=buffer;
                            GetWindowText(GetDlgItem(hwnd,IDC_PASSWORDEDIT),buffer,65535);
                            trimChar(buffer);
                            if(buffer[0]==0) {
                                MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_PASSWORD),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                            }
                            else {
                                credentials.username=username;
                                credentials.password=buffer;
                                GetModuleFileName(GetWindowWord(hwnd,GWW_HINSTANCE),buffer,32767);
                                iniFile=getDefaultIniFile(buffer);
                                saveCredentials(credentials,(char*)iniFile.c_str());
                                EnableWindow(GetDlgItem(GetParent(hwnd),IDB_DOWNLOAD),true);
                                EndDialog(hwnd,IDOK);
                            }
                        }
                    }
                    else {
                        EndDialog(hwnd,IDOK);
                    }
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
                case IDC_REGISTERBUTTON:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDC_REGISTERBUTTON))) {
                        break;
                    }
                    auxCredentials=&tempCredentials;
                    if(MakeDialogBox(hwnd,IDD_REGISTRATION,UserRegDlgProc)==IDOK) {
                        if(editsChanged2) {
                            SetWindowText(GetDlgItem(hwnd,IDC_USERNAMEEDIT),(char*)tempCredentials.username.c_str());
                            SetWindowText(GetDlgItem(hwnd,IDC_PASSWORDEDIT),(char*)tempCredentials.password.c_str());
                            useTestCredentials=true;
                            SendMessage(hwnd, WM_COMMAND, IDC_TESTBUTTON2, IDC_REGISTERBUTTON);
                        }
                    }
                    break;
                case IDC_TESTBUTTON2:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDC_TESTBUTTON2))) {
                        break;
                    }
                    if(noter_connectionSettingsAvailable(connectionSettings)) {
                        GetWindowText(GetDlgItem(hwnd,IDC_USERNAMEEDIT),buffer,65535);
                        trimChar(buffer);
                        if(buffer[0]==0) {
                            MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_USERNAME),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                        }
                        else {
                            tempCredentials.username=buffer;
                            GetWindowText(GetDlgItem(hwnd,IDC_PASSWORDEDIT),buffer,65535);
                            trimChar(buffer);
                            if(buffer[0]==0) {
                                MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_PASSWORD),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                            }
                            else {
                                tempCredentials.password=buffer;
                                if(noter_credentialsAvailable(tempCredentials)) {
                                    credentials_LockAllButtons(hwnd);
                                    result=noter_getUserInfo(connectionSettings,tempCredentials,buffer,userInfo);
                                    credentials_UnlockAllButtons(hwnd);
                                    if(result>=0) {
                                        SetWindowText(GetDlgItem(hwnd,IDC_USERIDSTATIC),            (char*)IntToStr(userInfo.ID).c_str());
                                        SetWindowText(GetDlgItem(hwnd,IDC_USERREGDATESTATIC),       (char*)userInfo.dateRegistered.c_str());
                                        SetWindowText(GetDlgItem(hwnd,IDC_USERREGUSINGSTATIC),      (char*)userInfo.userAgent.c_str());
                                        SetWindowText(GetDlgItem(hwnd,IDC_USERLASTMODSTATIC),       (char*)userInfo.lastChanged.c_str());
                                        SetWindowText(GetDlgItem(hwnd,IDC_USERLASTMODUSINGSTATIC),  (char*)userInfo.lastUserAgent.c_str());
                                        if(editsChanged) {
                                            useTestCredentials=true;
                                        }
                                        EnableWindow(GetDlgItem(hwnd,IDC_ACCDELETEBUTTON), true);
                                        EnableWindow(GetDlgItem(hwnd,IDC_PASSCHANGEBUTTON),true);
                                        if(lParam!=IDC_REGISTERBUTTON) {
                                            MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_LOGIN_SUCCESSFUL),getStringFromTable(IDS_STRING_INFORMATION,1),MB_ICONINFORMATION | MB_OK);
                                        }
                                    }
                                    else {
                                        SetWindowText(GetDlgItem(hwnd,IDC_USERIDSTATIC),            getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                        SetWindowText(GetDlgItem(hwnd,IDC_USERREGDATESTATIC),       getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                        SetWindowText(GetDlgItem(hwnd,IDC_USERREGUSINGSTATIC),      getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                        SetWindowText(GetDlgItem(hwnd,IDC_USERLASTMODSTATIC),       getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                        SetWindowText(GetDlgItem(hwnd,IDC_USERLASTMODUSINGSTATIC),  getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                        EnableWindow( GetDlgItem(hwnd,IDC_ACCDELETEBUTTON),         false);
                                        EnableWindow( GetDlgItem(hwnd,IDC_PASSCHANGEBUTTON),        false);
                                        MessageBox(hwnd,(char*)noter_getAnswerString(result).c_str(),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                                    }
                                }
                                else {
                                    MessageBox(hwnd,getStringFromTable(IDS_STRING_NO_CREDENTIALS),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                                }
                            }
                        }
                    }
                    else {
                        MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_NO_CONN_SETTINGS),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                    }
                    break;
                case IDC_ACCDELETEBUTTON:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDC_ACCDELETEBUTTON))) {
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
                            if(MakeDialogBox(hwnd,IDD_ACCDELETE,PassConfirmDlgProc)==IDOK) {
                                if(useTestCredentials) {
                                    SetWindowText(GetDlgItem(hwnd,IDC_USERNAMEEDIT),            "");
                                    SetWindowText(GetDlgItem(hwnd,IDC_PASSWORDEDIT),            "");
                                    SetWindowText(GetDlgItem(hwnd,IDC_USERIDSTATIC),            getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                    SetWindowText(GetDlgItem(hwnd,IDC_USERREGDATESTATIC),       getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                    SetWindowText(GetDlgItem(hwnd,IDC_USERREGUSINGSTATIC),      getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                    SetWindowText(GetDlgItem(hwnd,IDC_USERLASTMODSTATIC),       getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                    SetWindowText(GetDlgItem(hwnd,IDC_USERLASTMODUSINGSTATIC),  getStringFromTable(IDS_STRING_NOT_LOGGED_IN));
                                    EnableWindow( GetDlgItem(hwnd,IDC_ACCDELETEBUTTON),         false);
                                    EnableWindow( GetDlgItem(hwnd,IDC_PASSCHANGEBUTTON),        false);
                                }
                                else {
                                    credentials.username="";
                                    credentials.password="";
                                    GetModuleFileName(GetWindowWord(hwnd,GWW_HINSTANCE),buffer,32767);
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
                case IDC_PASSCHANGEBUTTON:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDC_PASSCHANGEBUTTON))) {
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
                        if(MakeDialogBox(hwnd,IDD_PASSCHANGE,PassChangeDlgProc)==IDOK) {
                            SetWindowText(GetDlgItem(hwnd,IDC_PASSWORDEDIT),(char*)tempCredentials.password.c_str());
                            useTestCredentials=true;
                            SendMessage(hwnd, WM_COMMAND, IDC_TESTBUTTON2, IDC_REGISTERBUTTON);
                        }
                    }
                    else {
                        MessageBox(hwnd,getStringFromTable(IDS_STRING_NO_CREDENTIALS),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                    }
                    break;
                case IDC_USERNAMEEDIT:
                case IDC_PASSWORDEDIT:
                    switch(HIWORD(lParam)) {
                        case EN_CHANGE:
                            editsChanged=true;
                            if(GetWindowTextLength(GetDlgItem(hwnd,wParam))==0) {
                                EnableWindow(GetDlgItem(hwnd,IDOK),             false);
                                EnableWindow(GetDlgItem(hwnd,IDC_TESTBUTTON2),  false);
                            }
                            else {
                                if(GetWindowTextLength(GetDlgItem(hwnd,IDC_USERNAMEEDIT))>0 && GetWindowTextLength(GetDlgItem(hwnd,IDC_PASSWORDEDIT))>0) {
                                    EnableWindow(GetDlgItem(hwnd,IDOK),             true);
                                    EnableWindow(GetDlgItem(hwnd,IDC_TESTBUTTON2),  true);
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

// User registration dialog message processing function
BOOL CALLBACK UserRegDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Local variables
    NOTER_CREDENTIALS tempCredentials;
    std::string tempUserName, tempPassword, tempSecPassword, tempString;
    long int result;
    // Switch section
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
                    GetWindowText(GetDlgItem(hwnd,IDC_REGUSERNAMEEDIT),buffer,65535);
                    trimChar(buffer);
                    if(buffer[0]==0) {
                        MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_USERNAME),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                    }
                    else {
                        tempUserName=buffer;
                        GetWindowText(GetDlgItem(hwnd,IDC_REGPASSWORDEDIT),buffer,65535);
                        trimChar(buffer);
                        if(buffer[0]==0) {
                            MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_PASSWORD),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                        }
                        else {
                            tempPassword=buffer;
                            GetWindowText(GetDlgItem(hwnd,IDC_REGPSREPEATEDIT),buffer,65535);
                            trimChar(buffer);
                            if(buffer[0]==0) {
                                MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_PASSWORD),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                            }
                            else {
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
                            }
                        }
                    }
                    break;
                case IDCANCEL:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDCANCEL))) {
                        break;
                    }
                    editsChanged2=false;
                    EndDialog(hwnd,IDCANCEL);
                    break;
                case IDC_REGUSERNAMEEDIT:
                case IDC_REGPASSWORDEDIT:
                case IDC_REGPSREPEATEDIT:
                    switch(HIWORD(lParam)) {
                        case EN_CHANGE:
                            editsChanged2=true;
                            if(GetWindowTextLength(GetDlgItem(hwnd,wParam))==0) {
                                EnableWindow(GetDlgItem(hwnd,IDOK),false);
                            }
                            else {
                                if(GetWindowTextLength(GetDlgItem(hwnd,IDC_REGUSERNAMEEDIT))>0 && GetWindowTextLength(GetDlgItem(hwnd,IDC_REGPASSWORDEDIT))>0 && GetWindowTextLength(GetDlgItem(hwnd,IDC_REGPSREPEATEDIT))>0) {
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
//  PASSWORD CONFIRMATION DIALOG PROCEDURE (USER REMOVAL)
//
//////////////////////////////////////

// Password confirmation dialog message processing function (user removal)
BOOL CALLBACK PassConfirmDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Local variables
    long int result;
    std::string tempSecPassword, tempString;
    // Switch section
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
                    GetWindowText(GetDlgItem(hwnd,IDC_ACCREMPASSWORDEDIT),buffer,65535);
                    trimChar(buffer);
                    if(buffer[0]==0) {
                        MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_PASSWORD),getStringFromTable(IDS_STRING_ERROR,1),MB_ICONEXCLAMATION | MB_OK);
                    }
                    else {
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
                    }
                    break;
                case IDCANCEL:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDCANCEL))) {
                        break;
                    }
                    EndDialog(hwnd,IDCANCEL);
                    break;
                case IDC_ACCREMPASSWORDEDIT:
                    switch(HIWORD(lParam)) {
                        case EN_CHANGE:
                            EnableWindow(GetDlgItem(hwnd,IDOK),(GetWindowTextLength(GetDlgItem(hwnd,wParam))!=0));
                            break;
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

// Password change dialog message processing function
BOOL CALLBACK PassChangeDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Local variables
    std::string tempOldPassword, tempNewPassword, tempSecNewPassword;
    long int result;
    // Switch section
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
                    GetWindowText(GetDlgItem(hwnd,IDC_OLDPASSWORDEDIT),buffer,65535);
                    trimChar(buffer);
                    if(buffer[0]==0) {
                        MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_PASSWORD),getStringFromTable(IDS_APPNAME,1),MB_ICONEXCLAMATION | MB_OK);
                    }
                    else {
                        tempOldPassword=buffer;
                        GetWindowText(GetDlgItem(hwnd,IDC_NEWPASSWORDEDIT),buffer,65535);
                        trimChar(buffer);
                        if(buffer[0]==0) {
                            MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_PASSWORD),getStringFromTable(IDS_APPNAME,1),MB_ICONEXCLAMATION | MB_OK);
                        }
                        else {
                            tempNewPassword=buffer;
                            GetWindowText(GetDlgItem(hwnd,IDC_NEWPSREPEATEDIT),buffer,65535);
                            trimChar(buffer);
                            if(buffer[0]==0) {
                                MessageBox(hwnd,getStringFromTable(IDS_STRING_MSG_WRONG_PASSWORD),getStringFromTable(IDS_APPNAME,1),MB_ICONEXCLAMATION | MB_OK);
                            }
                            else {
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
                            }
                        }
                    }
                    break;
                case IDCANCEL:
                    if(!IsWindowEnabled(GetDlgItem(hwnd,IDCANCEL))) {
                        break;
                    }
                    editsChanged2=false;
                    EndDialog(hwnd,IDCANCEL);
                    break;
                case IDC_OLDPASSWORDEDIT:
                case IDC_NEWPASSWORDEDIT:
                case IDC_NEWPSREPEATEDIT:
                    switch(HIWORD(lParam)) {
                        case EN_CHANGE:
                            editsChanged2=true;
                            if(GetWindowTextLength(GetDlgItem(hwnd,wParam))==0) {
                                EnableWindow(GetDlgItem(hwnd,IDOK),false);
                            }
                            else {
                                if(GetWindowTextLength(GetDlgItem(hwnd,IDC_OLDPASSWORDEDIT))>0 && GetWindowTextLength(GetDlgItem(hwnd,IDC_NEWPASSWORDEDIT))>0 && GetWindowTextLength(GetDlgItem(hwnd,IDC_NEWPSREPEATEDIT))>0) {
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

// Place left for import dialog procedure

//////////////////////////////////////
//
//  NOTES EXPORT DIALOG PROCEDURE
//
//////////////////////////////////////

// Notes export dialog message processing function
BOOL CALLBACK NotesExpDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Switch section
    switch(msg) {
        case WM_INITDIALOG:
            setProgress(hwnd,       IDC_PROGRESSBARSTATIC,      IDC_PROGRESSBARBORDERSTATIC,0);
            CheckDlgButton(hwnd,    IDC_FIRSTLINESUBJECTCHECK,  BST_CHECKED);
            CheckDlgButton(hwnd,    IDC_ONELINEGAPCHECK,        BST_CHECKED);
            CheckDlgButton(hwnd,    IDC_ADDINFOATENDCHECK,      BST_CHECKED);
            CheckDlgButton(hwnd,    IDC_SEPARATEINFOCHECK,      BST_CHECKED);
            CheckRadioButton(hwnd,  IDC_NUMASIDSRADIO,          IDC_NUMSTARTSFROMRADIO, IDC_NUMASIDSRADIO);
            EnableWindow(GetDlgItem(hwnd,IDC_NUMSTARTSFROMEDIT),            false);
            EnableWindow(GetDlgItem(hwnd,IDC_ADDPREFIXEDIT),                false);
            EnableWindow(GetDlgItem(hwnd,IDC_ADDPREFIXCHECK),               false);
            EnableWindow(GetDlgItem(hwnd,IDC_SEPARATENOTESINFILESCHECK),    false);
            if(SendMessage(GetDlgItem(GetParent(hwnd),ID_LISTBOX),LB_GETSELCOUNT, 0, 0)>1) {
                CheckRadioButton(hwnd, IDC_EXPORTTOONEFILERADIO, IDC_EXPORTTOMANYFILESRADIO, IDC_EXPORTTOMANYFILESRADIO);
                CheckDlgButton(hwnd, IDC_IGNOREFILENAMECHECK, BST_CHECKED);
            }
            else {
                CheckRadioButton(hwnd, IDC_EXPORTTOONEFILERADIO, IDC_EXPORTTOMANYFILESRADIO, IDC_EXPORTTOONEFILERADIO);
                EnableWindow(GetDlgItem(hwnd,IDC_IGNOREFILENAMECHECK),      false);
                EnableWindow(GetDlgItem(hwnd,IDC_EXPORTTOMANYFILESRADIO),   false);
                EnableWindow(GetDlgItem(hwnd,IDC_NUMASIDSRADIO),            false);
                EnableWindow(GetDlgItem(hwnd,IDC_NUMSTARTSFROMRADIO),       false);
                EnableWindow(GetDlgItem(hwnd,IDC_CONTINUEONERRORSCHECK),    false);
            }
            break;
        case WM_CTLCOLOR:
            switch(HIWORD(lParam)) {
                case CTLCOLOR_STATIC:
                    if(LOWORD(lParam)==GetDlgItem(hwnd,IDC_PROGRESSBARSTATIC)) {
                        return g_hBrushActCapt;
                    }
                    break;
                case CTLCOLOR_EDIT:
                    SetBkMode((HDC)wParam, TRANSPARENT);
                    return g_hBrushWindow;
            }
            break;
        case WM_COMMAND:
            switch(wParam) {
                case IDC_EXPORTTOONEFILERADIO:
                    EnableWindow(GetDlgItem(hwnd,IDC_NUMASIDSRADIO),            false);
                    EnableWindow(GetDlgItem(hwnd,IDC_NUMSTARTSFROMRADIO),       false);
                    EnableWindow(GetDlgItem(hwnd,IDC_ADDPREFIXCHECK),           false);
                    EnableWindow(GetDlgItem(hwnd,IDC_NUMSTARTSFROMEDIT),        false);
                    EnableWindow(GetDlgItem(hwnd,IDC_ADDPREFIXEDIT),            false);
                    if(SendMessage(GetDlgItem(GetParent(hwnd),ID_LISTBOX),LB_GETSELCOUNT, 0, 0)>1) {
                        EnableWindow(GetDlgItem(hwnd,IDC_SEPARATENOTESINFILESCHECK),true);
                    }
                    else {
                        EnableWindow(GetDlgItem(hwnd,IDC_SEPARATENOTESINFILESCHECK),false);
                    }
                    CheckRadioButton(hwnd, IDC_EXPORTTOONEFILERADIO, IDC_EXPORTTOMANYFILESRADIO, IDC_EXPORTTOONEFILERADIO);
                    break;
                case IDC_EXPORTTOMANYFILESRADIO:
                    EnableWindow(GetDlgItem(hwnd,IDC_NUMASIDSRADIO),            true);
                    EnableWindow(GetDlgItem(hwnd,IDC_NUMSTARTSFROMRADIO),       true);
                    if(IsDlgButtonChecked(hwnd,IDC_NUMSTARTSFROMRADIO)) {
                        EnableWindow(GetDlgItem(hwnd,IDC_NUMSTARTSFROMEDIT),    true);
                        EnableWindow(GetDlgItem(hwnd,IDC_ADDPREFIXCHECK),       true);
                        EnableWindow(GetDlgItem(hwnd,IDC_ADDPREFIXEDIT),IsDlgButtonChecked(hwnd,IDC_ADDPREFIXCHECK));
                    }
                    else {
                        EnableWindow(GetDlgItem(hwnd,IDC_NUMSTARTSFROMEDIT),    false);
                        EnableWindow(GetDlgItem(hwnd,IDC_ADDPREFIXCHECK),       false);
                    }
                    EnableWindow(GetDlgItem(hwnd,IDC_SEPARATENOTESINFILESCHECK),false);
                    CheckRadioButton(hwnd, IDC_EXPORTTOONEFILERADIO, IDC_EXPORTTOMANYFILESRADIO, IDC_EXPORTTOMANYFILESRADIO);
                    break;
                case IDC_NUMASIDSRADIO:
                    CheckRadioButton(hwnd, IDC_NUMASIDSRADIO, IDC_NUMSTARTSFROMRADIO, IDC_NUMASIDSRADIO);
                    EnableWindow(GetDlgItem(hwnd,IDC_NUMSTARTSFROMEDIT),        false);
                    EnableWindow(GetDlgItem(hwnd,IDC_ADDPREFIXCHECK),           false);
                    EnableWindow(GetDlgItem(hwnd,IDC_ADDPREFIXEDIT),            false);
                    break;
                case IDC_NUMSTARTSFROMRADIO:
                    CheckRadioButton(hwnd, IDC_NUMASIDSRADIO, IDC_NUMSTARTSFROMRADIO, IDC_NUMSTARTSFROMRADIO);
                    EnableWindow(GetDlgItem(hwnd,IDC_NUMSTARTSFROMEDIT),        true);
                    EnableWindow(GetDlgItem(hwnd,IDC_ADDPREFIXCHECK),           true);
                    EnableWindow(GetDlgItem(hwnd,IDC_ADDPREFIXEDIT),IsDlgButtonChecked(hwnd,IDC_ADDPREFIXCHECK));
                    break;
                case IDC_ADDPREFIXCHECK:
                    EnableWindow(GetDlgItem(hwnd,IDC_ADDPREFIXEDIT),IsDlgButtonChecked(hwnd,IDC_ADDPREFIXCHECK));
                    break;
                case IDC_FIRSTLINESUBJECTCHECK:
                    EnableWindow(GetDlgItem(hwnd,IDC_ONELINEGAPCHECK),IsDlgButtonChecked(hwnd,IDC_FIRSTLINESUBJECTCHECK));
                    break;
                case IDC_ADDINFOATENDCHECK:
                    EnableWindow(GetDlgItem(hwnd,IDC_SEPARATEINFOCHECK),IsDlgButtonChecked(hwnd,IDC_ADDINFOATENDCHECK));
                    break;
                case IDC_BROWSEBUTTON:
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

//////////////////////////////////////
//
//  LIBRARY INFORMATION DIALOG PROCEDURE
//
//////////////////////////////////////

// Library information dialog message processing function
BOOL CALLBACK LibInfoDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static char test[64];
    // Switch section
    switch(msg) {
        case WM_INITDIALOG:
            test[0]=0x00;
            LoadString((HINSTANCE)lParam,IDS_LIBTYPE,test,64);
            if(((std::string)test)==STR_CODEPAGE) {
                SetWindowText(GetDlgItem(hwnd,IDC_LIBTYPEDATA),getStringFromTable(IDS_STRING_LIB_CODEPAGE_DEFINITION));
                LoadString((HINSTANCE)lParam,IDS_CPNAME,test,64);
                SetWindowText(GetDlgItem(hwnd,IDC_LIBLINE1DATA),test);
                LoadString((HINSTANCE)lParam,IDS_USEDWORD,test,64);
                SetWindowText(GetDlgItem(hwnd,IDC_LIBLINE2DATA),(((std::string)test=="0")?getStringFromTable(IDS_STRING_LIB_NO):getStringFromTable(IDS_STRING_LIB_YES)));
            }
            else if (((std::string)test)==STR_STRINGTABLE) {
                SetWindowText(GetDlgItem(hwnd,IDC_LIBTYPEDATA),getStringFromTable(IDS_STRING_LIB_LANGUAGE_PACK));
                // TODO: get more info
            }
            else {
                SetWindowText(GetDlgItem(hwnd,IDC_LIBTYPEDATA),getStringFromTable(IDS_STRING_LIB_UNKNOWN));
            }
            LoadString((HINSTANCE)lParam,IDS_REVDATE,test,64);
            SetWindowText(GetDlgItem(hwnd,IDC_LIBLINE3DATA),test);
            LoadString((HINSTANCE)lParam,IDS_AUTHOR,test,64);
            SetWindowText(GetDlgItem(hwnd,IDC_LIBLINE4DATA),test);
            break;
        case WM_COMMAND:
            switch(wParam) {
                case IDOK:
                    EndDialog(hwnd,IDOK);
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
