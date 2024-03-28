#ifndef LIBUTIL_H
#define LIBUTIL_H

#include <windows.h>
#include <direct.h>
#include <list>
#include <string>

#include "additional.hpp"
#include "libdefs.hpp"

#define LIB_UNKNOWN     0
#define LIB_CODEPAGE    1
#define LIB_STRINGTABLE 2

typedef struct LIB_FILE_INFO {
    unsigned short int type;
    std::string filename, relatedInfo;
} LIB_FILE_INFO;

typedef std::list<LIB_FILE_INFO> LIBRARIES;
typedef std::list<LIB_FILE_INFO>::iterator LIB_ITER;

unsigned short int getLibType(HINSTANCE);
std::string getCodePageInfo(HINSTANCE);
bool listAvailableLibs(char*, LIBRARIES&);
std::string findAnyCodePage(LIBRARIES&);

#endif
