#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <string>
#include "noteprocs.hpp"
#include "libdefs.hpp"

typedef struct editWindow {
    HWND hwnd;
    HWND hStaticSubject, hStaticEntry, hStaticGrayBox, hStaticStatus;
    HWND hEditBoxSubject, hEditBoxEntry;
    HWND hButtonAddUp, hButtonProps, hButtonClose;
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
    bool autoReload, autoRefresh, savePosSizes, use3DControls, use3DButtons, use3DLists, use3DEdits, use3DCombos, use3DDialogs, showMultiIDnLM;
    std::string selectedCodePage;
} MAINSETTINGS;

#endif
