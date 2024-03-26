#include "helpers.hpp"

bool checkIfInt(char* input) {
    unsigned long int x=0;
    while(input[x]!=0x00) {
        if((input[x]<0x30) || (input[x]>0x39)) {
            return false;
        }
        ++x;
    }
    return true;
}

unsigned int getState(HWND hwnd) {
    WINDOWPLACEMENT wp = { 0 };
    wp.length=sizeof(WINDOWPLACEMENT);
    if(GetWindowPlacement(hwnd,&wp)) {
        return wp.showCmd;
    }
    else {
        return 0;
    }
}

void deleteWindow(WINDOWMEMORY &winMem, HWND hwnd) {
    delete winMem[hwnd]->note;
    delete winMem[hwnd];
    winMem.erase(hwnd);
    return;
}

void makeEditWindowTitle(EDITWINDOW *editWin, NOTE *note, bool set, CODEPAGE &codePage) {
    if(note==NULL) {
        editWin->windowTitle = STRING_NEW_NOTE;
    }
    else {
        editWin->windowTitle = toCodePage(codePage,(char*)note->subject.c_str())+" - ";
    }
    editWin->windowTitle = editWin->windowTitle + APPNAME;
    if(set) {
        SetWindowText(editWin->hwnd,(char*)editWin->windowTitle.c_str());
    }
    return;
}

void getWindowCoordinates(HWND hwnd, int &x, int &y, int &size_x, int &size_y, unsigned int &state) {
    RECT tempRect;
    GetWindowRect(hwnd,&tempRect);
    x=tempRect.left;
    y=tempRect.top;
    size_x=(tempRect.right-tempRect.left);
    size_y=(tempRect.bottom-tempRect.top);
    state=getState(hwnd);
    return;
}
