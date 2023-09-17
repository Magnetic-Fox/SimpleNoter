#include <windows.h>
#include <ctl3d.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>

#include "resources.h"

#include "cp1250.hpp"
#include "wsprocs.hpp"
#include "noterapi.hpp"
#include "codepages.hpp"
#include "inihandling.hpp"

#define APPNAME             "Simple Noter v0.4"

#define ID_BUTTON1          400
#define ID_BUTTON2          401
#define ID_BUTTON3          402
#define ID_BUTTON4          403
#define ID_BUTTON5          404
#define ID_LISTBOX          500
#define ID_STATIC1          600
#define ID_STATIC2          601
#define ID_STATIC3          602
#define ID_STATIC4          603
#define ID_STATIC5          604
#define ID_STATIC6          605

#define ID_EDIT_BUTTON1     1400
#define ID_EDIT_BUTTON2     1401
#define ID_EDIT_BUTTON3     1402
#define ID_EDIT_EDITBOX1    1500
#define ID_EDIT_EDITBOX2    1501
#define ID_EDIT_STATIC1     1600
#define ID_EDIT_STATIC2     1601
#define ID_EDIT_STATIC3     1602
#define ID_EDIT_STATIC4     1603

LPSTR editWindowClass = "SimpleNoterEdit";

typedef struct editWindow
{
    HWND hwnd;
    HWND hStatic, hStatic2, hStatic3, hStatic4;
    HWND hEditBox, hEditBox2;
    HWND hButton, hButton2, hButton3;
    std::string windowTitle;
    bool subjectChanged, entryChanged;
    int lastResult;
    NOTE *note;
} EDITWINDOW;

typedef std::map<HWND,EDITWINDOW*> WINDOWMEMORY;

HBRUSH g_hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
WINDOWMEMORY winMem;

char buffer[65536];
CODEPAGE m_cp1250;
NOTER_CONNECTION_SETTINGS connectionSettings;
NOTER_CREDENTIALS credentials;
NOTE_SUMMARY *notes=NULL;
long int noteCount=0;
long int mainLastResult=0;
std::string tempString;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc2(HWND, UINT, WPARAM, LPARAM);

HWND g_hwnd;

std::string getAnswerString(int answerCode)
{
    switch(answerCode)
    {
        case ERROR_WRONG_RESPONSE:
            return "Nie uda�o si� po��czy� z serwerem.";
        case ERROR_SERVICE_DISABLED:
            return "Us�uga tymczasowo niedost�pna.";
        case ERROR_INTERNAL_SERVER_ERROR:
            return "B��d wewn�trzny serwera.";
        case ERROR_NOTE_ALREADY_UNLOCKED:
            return "Notatka ju� odblokowana.";
        case ERROR_NOTE_ALREADY_LOCKED:
            return "Notatka ju� zablokowana.";
        case ERROR_NOTE_LOCKED:
            return "Notatka jest zablokowana.";
        case ERROR_USER_REMOVAL_FAILURE:
            return "Usuni�cie u�ytkownika nie powiod�o si�.";
        case ERROR_USER_NOT_EXISTS:
            return "Dany u�ytkownik nie istnieje.";
        case ERROR_NOTE_NOT_EXISTS:
            return "Dana notatka nie istnieje.";
        case ERROR_NO_NECESSARY_INFORMATION:
            return "Brak wymaganych informacji (temat lub tre��).";
        case ERROR_USER_DEACTIVATED:
            return "U�ytkownik zablokowany.";
        case ERROR_LOGIN_INCORRECT:
            return "Nieprawid�owe dane logowania.";
        case ERROR_UNKNOWN_ACTION:
            return "Nieprawid�owe polecenie.";
        case ERROR_NO_CREDENTIALS:
            return "Brak danych logowania.";
        case ERROR_USER_EXISTS:
            return "Podana nazwa u�ytkownika jest ju� zaj�ta.";
        case ERROR_NO_USABLE_INFORMATION:
            return "Brak u�ytecznych informacji w ��daniu.";
        case ERROR_INVALID_METHOD:
            return "Nieobs�ugiwane ��danie.";
        case INFO_OK:
            return "Wszystko w porz�dku.";
        case INFO_USER_CREATED:
            return "U�ytkownik zosta� zarejestrowany.";
        case INFO_USER_UPDATED:
            return "Dane u�ytkownika zosta�y zaktualizowane.";
        case INFO_USER_REMOVED:
            return "U�ytkownik zosta� usuni�ty.";
        case INFO_LIST_SUCCESSFUL:
            return "Lista notatek zosta�a za�adowana.";
        case INFO_NOTE_RETRIEVED:
            return "Notatka zosta�a za�adowana.";
        case INFO_NOTE_CREATED:
            return "Notatka zosta�a utworzona.";
        case INFO_NOTE_UPDATED:
            return "Notatka zosta�a zaktualizowana.";
        case INFO_NOTE_DELETED:
            return "Notatka zosta�a usuni�ta.";
        case INFO_USER_INFO_RETRIEVED:
            return "Informacje o u�ytkowniku zosta�y za�adowane.";
        case INFO_NOTE_LOCKED:
            return "Notatka zosta�a zablokowana.";
        case INFO_NOTE_UNLOCKED:
            return "Notatka zosta�a odblokowana.";
        default:
            return "Nieznany kod odpowiedzi.";
    }
}

void ShowInteger(long int integer)
{
    char test[20];
    ltoa(integer,test,10);
    MessageBox(0,test,"ShowInteger",MB_OK);
    return;
}

std::string IntToStr(long int input)
{
    std::string temp;
    char test[32];
    ltoa(input,test,10);
    temp=test;
    return temp;
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
    editWin->windowTitle = APPNAME;
    if(note==NULL)
    {
        editWin->windowTitle = editWin->windowTitle +" [Nowa notatka]";
    }
    else
    {
        editWin->windowTitle = editWin->windowTitle+" ["+toCodePage(m_cp1250,(char*)note->subject.c_str())+"]";
    }
    
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
            editWin->note->lastUserAgent;
        }
        else
        {
            editWin->note=note;
        }
        
        editWin->hStatic = CreateWindow("STATIC", "Tytu�:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        0, 0, 600, 16, editWin->hwnd, (HMENU)ID_EDIT_STATIC1, hInstance, NULL);

        editWin->hEditBox= CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                                        0, 16, 600, 24, editWin->hwnd, (HMENU)ID_EDIT_EDITBOX1, hInstance, NULL);

        editWin->hStatic2= CreateWindow("STATIC", "Tre��:", WS_CHILD | WS_VISIBLE | SS_LEFT,
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
            editWin->hButton2 =CreateWindow("BUTTON", "W�a�ciwo�ci", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                                            96, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON2, hInstance, NULL);
        }
        else
        {
            editWin->hButton2 =CreateWindow("BUTTON", "W�a�ciwo�ci", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                            96, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON2, hInstance, NULL);
        }

        editWin->hButton3 =CreateWindow("BUTTON", "Zamknij", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                        192, 356, 96, 21, editWin->hwnd, (HMENU)ID_EDIT_BUTTON3, hInstance, NULL);

        editWin->hStatic3 =CreateWindow("STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_GRAYRECT,
                                        288, 356, 312, 21, editWin->hwnd, (HMENU)ID_EDIT_STATIC3, hInstance, NULL);

        editWin->hStatic4= CreateWindow("STATIC", "Wszystko w porz�dku.", WS_CHILD | WS_VISIBLE | SS_LEFT,
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
        
        ShowWindow(editWin->hwnd,SW_SHOW);
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
//  G��WNA CZʌ� PROGRAMU
//
//////////////////////////////////////

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    char path[256];
    GetModuleFileName(hInstance,path,256);
    std::string iniFile=getDefaultIniFile(path);
    connectionSettings=getConnectionSettings((char*)iniFile.c_str());
    credentials=getCredentials((char*)iniFile.c_str());
    prepareCodePage(m_cp1250,cp1250);

    if(wsInit() == SOCKET_ERROR)
    {
        MessageBox(NULL,"Inicjalizacja sk�adnika WinSock nie powiod�a si�. Program zostanie zamkni�ty.","B��d",MB_ICONSTOP | MB_OK);
        return 1;
    }
    
    LPSTR mainWindowClass = "SimpleNoterMain";
    
    WNDCLASS wc = { 0 };
    WNDCLASS wc2= { 0 };

    HWND hwnd, hwnd2;
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
    wc2.hIcon = LoadIcon(hInstance, /*MAKEINTRESOURCE(IDI_ICON2)*/IDI_APPLICATION);
    wc2.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc2.hbrBackground = g_hBrush;
    wc2.lpszMenuName = /*MAKEINTRESOURCE(IDR_MENU2)*/NULL;
    wc2.lpszClassName = editWindowClass;

    if(!RegisterClass(&wc))
    {
        MessageBox(NULL,"Utworzenie klasy okna nie powiod�o si�. Program zostanie zamkni�ty.","B��d",MB_ICONSTOP | MB_OK);
        return 1;
    }

    if(!RegisterClass(&wc2))
    {
        MessageBox(NULL,"Utworzenie klasy okna nie powiod�o si�. Program zostanie zamkni�ty.","B��d",MB_ICONSTOP | MB_OK);
        return 1;
    }

    hwnd = CreateWindow(mainWindowClass, APPNAME, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        NULL, NULL, hInstance, NULL);
                        
    if(hwnd == NULL)
    {
        MessageBox(NULL,"Utworzenie okna nie powiod�o si�. Program zostanie zamkni�ty.","B��d",MB_ICONSTOP | MB_OK);
        return 1;
    }

    g_hwnd=hwnd;

    hAccel=LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));
    if(!hAccel)
    {
        MessageBox(NULL,"Nie uda�o si� za�adowa� akcelerator�w! Program zostanie zamkni�ty.","B��d",MB_ICONSTOP | MB_OK);
        return 1;
    }

    hButton =  CreateWindow("BUTTON", "Pobierz", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                            0, 0, 80, 21, hwnd, (HMENU)ID_BUTTON1, hInstance, NULL);

    hButton2 = CreateWindow("BUTTON", "Utw�rz", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                            80, 0, 80, 21, hwnd, (HMENU)ID_BUTTON2, hInstance, NULL);

    hButton3 = CreateWindow("BUTTON", "Otw�rz", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                            160, 0, 80, 21, hwnd, (HMENU)ID_BUTTON3, hInstance, NULL);

    hButton5 = CreateWindow("BUTTON", "Usu�", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
                            240, 0, 80, 21, hwnd, (HMENU)ID_BUTTON5, hInstance, NULL);

    hButton4 = CreateWindow("BUTTON", "Zako�cz", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
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
                           
    hStatic5 = CreateWindow("STATIC", "Wszystko w porz�dku.", WS_CHILD | WS_VISIBLE | SS_LEFT,
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
        MessageBox(hwnd,"Zarejestrowanie CTL3D nie powiod�o si�!","Ostrze�enie",MB_ICONEXCLAMATION);
    }

    ShowWindow(hwnd,nCmdShow);
    UpdateWindow(hwnd);

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
//  G��WNE OKNO
//
//////////////////////////////////////

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
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
        case WM_COMMAND:
            long int index;
            long int result;
            switch(wParam)
            {
                case ID_ACC_TAB:
                    SetFocus(GetNextDlgTabItem(hwnd,GetFocus(),false));
                    break;
                case ID_ACC_ALTF4:
                    SendMessage(hwnd, WM_COMMAND, ID_BUTTON4, 0);
                    break;
                case ID_FILE_EXIT:
                    SendMessage(hwnd, WM_COMMAND, ID_BUTTON4, 0);
                    break;
                case ID_BUTTON1:
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
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)getAnswerString(mainLastResult).c_str());
                        }
                    }
                    else
                    {
                        mainLastResult=noteCount;
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)getAnswerString(mainLastResult).c_str());
                    }
                    break;
                case ID_BUTTON2:
                    if(mainLastResult!=0)
                    {
                        mainLastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)getAnswerString(mainLastResult).c_str());
                    }
                    createEditWindow(hwnd,winMem,NULL);
                    break;
                case ID_BUTTON3:
                    if(mainLastResult!=0)
                    {
                        mainLastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)getAnswerString(mainLastResult).c_str());
                    }
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
                                SetWindowText(GetDlgItem(tempHwnd,ID_EDIT_STATIC4),(char*)getAnswerString(result).c_str());
                                winMem[tempHwnd]->lastResult=result;
                            }
                        }
                        else
                        {
                            mainLastResult=result;
                            SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)getAnswerString(mainLastResult).c_str());
                        }
                    }
                    break;
                case ID_BUTTON4:
                    if(mainLastResult!=0)
                    {
                        mainLastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)getAnswerString(mainLastResult).c_str());
                    }
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
                case ID_BUTTON5:
                    if(mainLastResult!=0)
                    {
                        mainLastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)getAnswerString(mainLastResult).c_str());
                    }
                    index=SendMessage(GetDlgItem(hwnd,ID_LISTBOX), LB_GETCURSEL, 0, 0);
                    tempString="Czy na pewno chcesz usun�� notatk� \"";
                    tempString=tempString+toCodePage(m_cp1250,(char*)notes[index].subject.c_str());
                    tempString=tempString+"\"?";
                    if(MessageBox(hwnd,(char*)tempString.c_str(),APPNAME,MB_ICONQUESTION | MB_YESNO)==IDYES)
                    {
                        mainLastResult=noter_deleteNote(connectionSettings,credentials,notes[index].id,buffer);
                        SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)getAnswerString(mainLastResult).c_str());
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
                                SetWindowText(GetDlgItem(hwnd,ID_STATIC5),(char*)getAnswerString(mainLastResult).c_str());
                            }
                            break;
                    }
                    break;
            }
            break;
        case WM_SIZE:
            unsigned long int width= LOWORD(lParam);
            unsigned long int height=HIWORD(lParam);

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
            break;
        case WM_DESTROY:
            if(Ctl3dEnabled() && (!Ctl3dUnregister(GetWindowWord(hwnd,GWW_HINSTANCE))))
            {
                MessageBox(0,"Wyrejestrowanie aplikacji z CTL3D nie powiod�o si�!","Ostrze�enie",MB_ICONEXCLAMATION);
            }
            WSACleanup();
            if(noteCount>0)
            {
                freeNoteList(notes);
            }
            DeleteObject(g_hBrush);
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
        case WM_COMMAND:
            switch(wParam)
            {
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
                                    SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)getAnswerString(winMem[hwnd]->lastResult).c_str());
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
                                    SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)getAnswerString(winMem[hwnd]->lastResult).c_str());
                                }
                            }
                            break;
                    }
                    break;
                case ID_EDIT_BUTTON1:
                    if(winMem[hwnd]->lastResult!=0)
                    {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    GetWindowText(GetDlgItem(hwnd,ID_EDIT_EDITBOX1),buffer,65535);
                    winMem[hwnd]->note->subject=fromCodePage(cp1250,buffer);
                    GetWindowText(GetDlgItem(hwnd,ID_EDIT_EDITBOX2),buffer,65535);
                    winMem[hwnd]->note->entry=fromCodePage(cp1250,buffer);
                    if(winMem[hwnd]->note->id==0)
                    {
                        winMem[hwnd]->lastResult=noter_addNote(connectionSettings,credentials,*winMem[hwnd]->note,buffer);
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)getAnswerString(winMem[hwnd]->lastResult).c_str());
                        if(winMem[hwnd]->lastResult>0)
                        {
                            SendMessage(g_hwnd,WM_COMMAND,ID_BUTTON1,0);
                            EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),false);
                            EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON2),true);
                            SetWindowText(GetDlgItem(hwnd,ID_EDIT_BUTTON1),"Aktualizuj");
                            winMem[hwnd]->subjectChanged=false;
                            winMem[hwnd]->entryChanged=false;
                        }
                    }
                    else
                    {
                        winMem[hwnd]->lastResult=noter_updateNote(connectionSettings,credentials,*winMem[hwnd]->note,buffer);
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)getAnswerString(winMem[hwnd]->lastResult).c_str());
                        if(winMem[hwnd]->lastResult>0)
                        {
                            SendMessage(g_hwnd,WM_COMMAND,ID_BUTTON1,0);
                            EnableWindow(GetDlgItem(hwnd,ID_EDIT_BUTTON1),false);
                            winMem[hwnd]->subjectChanged=false;
                            winMem[hwnd]->entryChanged=false;
                        }
                    }
                    break;
                case ID_EDIT_BUTTON2:
                    if(winMem[hwnd]->lastResult!=0)
                    {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    break;
                case ID_EDIT_BUTTON3:
                    if(winMem[hwnd]->lastResult!=0)
                    {
                        winMem[hwnd]->lastResult=0;
                        SetWindowText(GetDlgItem(hwnd,ID_EDIT_STATIC4),(char*)getAnswerString(winMem[hwnd]->lastResult).c_str());
                    }
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
            }   
            break;
        case WM_SIZE:
            unsigned long int width= LOWORD(lParam);
            unsigned long int height=HIWORD(lParam);

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
            unsigned int result;
            if((winMem[hwnd]!=NULL) && (winMem[hwnd]->subjectChanged || winMem[hwnd]->entryChanged))
            {
                GetWindowText(hwnd,buffer,32767);
                result=MessageBox(hwnd,"Czy chcesz zapisa� zmiany?",buffer,MB_ICONEXCLAMATION | MB_YESNOCANCEL);
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
