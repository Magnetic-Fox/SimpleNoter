#ifndef HELPERS_H
#define HELPERS_H

#include <windows.h>
#include "definitions.hpp"
#include "resources.h"
#include "codepages.hpp"
#include "constants.hpp"
#include "additional.hpp"
#include "noterapi.hpp"

static NOTER_CONNECTION_SETTINGS *conn=NULL;
static NOTER_CREDENTIALS *creds=NULL;
static HWND *gHW=NULL;
static WINDOWMEMORY *wMem=NULL;

bool checkIfInt(char*);
unsigned int getState(HWND);
void deleteWindow(WINDOWMEMORY&, HWND);
void makeEditWindowTitle(EDITWINDOW*, NOTE*, bool, CODEPAGE&);
void getWindowCoordinates(HWND, int&, int&, int&, int&, unsigned int&);
void inline lockExitButton(HWND);
void inline unlockExitButton(HWND);
void inline lockRefreshButton(HWND);
void inline unlockRefreshButton(HWND);
void inline lockOpenButton(HWND);
void inline unlockOpenButton(HWND);
void inline lockDeleteButton(HWND);
void inline unlockDeleteButton(HWND);
void inline main_LockAllButtons(HWND);
void inline main_UnlockAllButtons(HWND);
void inline edit_LockAllButtons(HWND, HWND);
void inline edit_UnlockAllButtons(HWND);
void inline properties_LockAllButtons(HWND, HWND);
void inline properties_UnlockAllButtons(HWND);
void inline connection_LockAllButtons(HWND);
void inline connection_UnlockAllButtons(HWND);
void inline credentials_LockAllButtons(HWND);
void inline credentials_UnlockAllButtons(HWND);
void inline userEdit_LockAllButtons(HWND);
void inline userEdit_UnlockAllButtons(HWND);
long int inline MakeDialogBox(HWND, unsigned int, void*);
void inline processMessages(MSG*);
void inline storeConnectionSettingsReference(NOTER_CONNECTION_SETTINGS*);
void inline storeCredentialsReference(NOTER_CREDENTIALS*);
void inline storeGlobalHWNDReference(HWND*);
void inline storeWindowMemoryReference(WINDOWMEMORY*);

void inline lockExitButton(HWND hwnd) {
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON4),false);
    return;
}

void inline unlockExitButton(HWND hwnd) {
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON4),true);
    return;
}

void inline lockRefreshButton(HWND hwnd) {
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON1),false);
    return;
}

void inline unlockRefreshButton(HWND hwnd) {
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON1),true);
    return;
}

void inline lockOpenButton(HWND hwnd) {
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON3),false);
    return;
}

void inline unlockOpenButton(HWND hwnd) {
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON3),true);
    return;
}

void inline lockDeleteButton(HWND hwnd) {
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON5),false);
    return;
}

void inline unlockDeleteButton(HWND hwnd) {
    EnableWindow(GetDlgItem(hwnd,ID_BUTTON5),true);
    return;
}

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
    if(noter_connectionSettingsAvailable(*conn) && noter_credentialsAvailable(*creds)) {
        unlockRefreshButton(hwnd);
        if(SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0)>=0) {
            unlockOpenButton(hwnd);
            unlockDeleteButton(hwnd);
        }
    }
    return;
}

void inline edit_LockAllButtons(HWND g_hwnd, HWND hwnd) {
    main_LockAllButtons(g_hwnd);
    EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),false);
    EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON2),false);
    return;
}

void inline edit_UnlockAllButtons(HWND hwnd) {
    main_UnlockAllButtons(*gHW);
    if(((*wMem)[hwnd])->note->id!=0) {
        EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON2),true);
    }
    return;
}

void inline properties_LockAllButtons(HWND g_hwnd, HWND hwnd) {
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON1),false);
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON2),false);
    edit_LockAllButtons(g_hwnd, GetParent(hwnd));
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
    if(noter_connectionSettingsAvailable(*conn)) {
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

long int inline MakeDialogBox(HWND hwnd, unsigned int type, void* procedure) {
    long int result;
    HANDLE instHandle=(HINSTANCE)GetWindowWord(hwnd,GWW_HINSTANCE);
    FARPROC proc=MakeProcInstance((FARPROC)procedure, instHandle);
    result=DialogBox(instHandle, MAKEINTRESOURCE(type), hwnd, (DLGPROC)proc);
    FreeProcInstance(proc);
    return result;
}

void inline processMessages(MSG *msg) {
    TranslateMessage(msg);
    DispatchMessage(msg);
    return;
}

void inline storeConnectionSettingsReference(NOTER_CONNECTION_SETTINGS *in_conn) {
    conn=in_conn;
    return;
}

void inline storeCredentialsReference(NOTER_CREDENTIALS *in_creds) {
    creds=in_creds;
    return;
}

void inline storeGlobalHWNDReference(HWND *g_hwnd) {
    gHW=g_hwnd;
    return;
}

void inline storeWindowMemoryReference(WINDOWMEMORY *winMem) {
    wMem=winMem;
    return;
}

#endif
