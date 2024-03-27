#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <string>
#include "noteprocs.hpp"

#define IDS_LIBTYPE                     1
#define IDS_CPNAME                      2
#define IDS_USEDWORD                    3
#define IDS_REVDATE                     4
#define IDS_AUTHOR                      5

#define IDR_CODEPAGE                    100

#define ID_BUTTON1                      1400
#define ID_BUTTON2                      1401
#define ID_BUTTON3                      1402
#define ID_BUTTON4                      1403
#define ID_BUTTON5                      1404
#define ID_LISTBOX                      1500
#define ID_STATIC1                      1600
#define ID_STATIC2                      1601
#define ID_STATIC3                      1602
#define ID_STATIC4                      1603
#define ID_STATIC5                      1604
#define ID_STATIC6                      1605

#define ID_EDIT_BUTTON1                 2400
#define ID_EDIT_BUTTON2                 2401
#define ID_EDIT_BUTTON3                 2402
#define ID_EDIT_EDITBOX1                2500
#define ID_EDIT_EDITBOX2                2501
#define ID_EDIT_STATIC1                 2600
#define ID_EDIT_STATIC2                 2601
#define ID_EDIT_STATIC3                 2602
#define ID_EDIT_STATIC4                 2603

typedef struct editWindow {
    HWND hwnd;
    HWND hStatic, hStatic2, hStatic3, hStatic4;
    HWND hEditBox, hEditBox2;
    HWND hButton, hButton2, hButton3;
    std::string windowTitle;
    bool subjectChanged, entryChanged;
    long int lastResult;
    NOTE *note;
} EDITWINDOW;

typedef std::map<HWND,EDITWINDOW*> WINDOWMEMORY;

typedef struct mainSettings {
    bool mainWindowSystem, editWindowSystem;
    unsigned int mainWindowStyle, editWindowStyle;
    int mainWindowX, mainWindowY, mainWindowSizeX, mainWindowSizeY, editWindowX, editWindowY, editWindowSizeX, editWindowSizeY;
    bool autoReload, autoRefresh, savePosSizes, use3DControls, use3DButtons, use3DLists, use3DEdits, use3DCombos, use3DDialogs;
} MAINSETTINGS;

#endif
