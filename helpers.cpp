#include "helpers.hpp"

bool checkIfInt(char* input)
{
    unsigned long int x=0;
    while(input[x]!=0x00)
    {
        if((input[x]<0x30) || (input[x]>0x39))
        {
            return false;
        }
        ++x;
    }
    return true;
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
