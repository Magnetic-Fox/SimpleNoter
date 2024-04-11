#ifndef ADDITIONAL_H
#define ADDITIONAL_H

#include <string>
#include "string.h"
#include <stdlib.h>
#include <windows.h>
#include <olenls.h>
#include "constants.hpp"

#include "resources.h"

// hope more temporary space is not needed...
#define MAX_TEMP_SIZE       256
#define MAX_BUFFER_COUNT    2

std::string winVersionString(void);
std::string makeUserAgent(char*);
std::string getCurrentDirectory(char*);
std::string getCodePage(void);
std::string getLangName(void);
std::string inline makeDefaultUserAgent(void);
void storeStringTableInstance(HINSTANCE);
char* getStringFromTable(UINT stringID, unsigned short int whichBuffer = 0);

std::string inline makeDefaultUserAgent(void) {
    std::string temp=getStringFromTable(IDS_USER_AGENT);
    return makeUserAgent((char*)temp.c_str());
}

#endif
