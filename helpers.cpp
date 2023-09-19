#include "helpers.hpp"

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

void deleteWindow(WINDOWMEMORY &winMem, HWND hwnd)
{
    delete winMem[hwnd]->note;
    delete winMem[hwnd];
    winMem.erase(hwnd);
    return;
}

void makeEditWindowTitle(EDITWINDOW *editWin, NOTE *note, bool set, CODEPAGE &codePage)
{
    if(note==NULL)
    {
        editWin->windowTitle = "~ Nowa notatka ~ - ";
    }
    else
    {
        editWin->windowTitle = toCodePage(codePage,(char*)note->subject.c_str())+" - ";
    }
    editWin->windowTitle = editWin->windowTitle + APPNAME;
    if(set)
    {
        SetWindowText(editWin->hwnd,(char*)editWin->windowTitle.c_str());
    }
    return;
}
