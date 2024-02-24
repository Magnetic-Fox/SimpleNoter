#ifndef HELPERS_H
#define HELPERS_H

#include <windows.h>
#include "definitions.hpp"
#include "resources.h"
#include "codepages.hpp"
#include "constants.hpp"

bool checkIfInt(char*);
unsigned int getState(HWND);
void deleteWindow(WINDOWMEMORY&, HWND);
void makeEditWindowTitle(EDITWINDOW*, NOTE*, bool, CODEPAGE&);
void inline lockExitButton(HWND);
void inline unlockExitButton(HWND);
void inline lockRefreshButton(HWND);
void inline unlockRefreshButton(HWND);
void inline lockOpenButton(HWND);
void inline unlockOpenButton(HWND);
void inline lockDeleteButton(HWND);
void inline unlockDeleteButton(HWND);
long int inline MakeDialogBox(HWND, unsigned int, void*);

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

long int inline MakeDialogBox(HWND hwnd, unsigned int type, void* procedure) {
    long int result;
    HANDLE instHandle=(HINSTANCE)GetWindowWord(hwnd,GWW_HINSTANCE);
    FARPROC proc=MakeProcInstance((FARPROC)procedure, instHandle);
    result=DialogBox(instHandle, MAKEINTRESOURCE(type), hwnd, (DLGPROC)proc);
    FreeProcInstance(proc);
    return result;
}

#endif
