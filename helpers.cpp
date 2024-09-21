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
        editWin->windowTitle = (std::string)getStringFromTable(IDS_STRING_NEW_NOTE);
    }
    else {
        editWin->windowTitle = toCodePage(codePage,(char*)note->subject.c_str())+" - ";
    }
    editWin->windowTitle = editWin->windowTitle + (std::string)getStringFromTable(IDS_APPNAME,1);
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

unsigned int getSelection(HWND listHwnd, unsigned int *&selected) {
    unsigned long int selCount=SendMessage(listHwnd, LB_GETSELCOUNT, 0, 0);
    unsigned int selSize=0;
    if((selCount<=65535) && (selCount>0)) {
        selected=new unsigned int[selCount];
        if(selected==NULL) {
            selSize=0;
        }
        else {
            selSize=selCount;
        }
    }
    unsigned int itemsInBuffer=SendMessage(listHwnd, LB_GETSELITEMS, selSize, (LPARAM)selected);
    if(selCount==0) {
        return 0;
    }
    else if(selCount>itemsInBuffer) {
        return 0;
    }
    else {
        return itemsInBuffer;
    }
}

void setSelection(HWND listHwnd, unsigned int *&selected, unsigned int itemsInBuffer) {
    for(long int x=itemsInBuffer-1; x>=0; --x) {
        SendMessage(listHwnd, LB_SETSEL, TRUE, selected[x]);
    }
    return;
}

void selectIndexes(HWND listHwnd, unsigned int *&nowId, unsigned int itemsInBuffer, NOTE_SUMMARY *&notes, unsigned int noteCount) {
    for(unsigned int y=0; y<itemsInBuffer; ++y) {
        for(unsigned int x=0; x<noteCount; ++x) {
            if(nowId[y]==notes[x].id) {
                SendMessage(listHwnd, LB_SETSEL, TRUE, x);
                break;
            }
        }
    }
    return;
}

void freeSelectionBuffer(unsigned int *&selected) {
    delete[] selected;
    return;
}
