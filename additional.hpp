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
#define MAX_TEMP_SIZE   256

std::string winVersionString(void);
std::string makeUserAgent(char*);
std::string getCurrentDirectory(char*);
std::string getCodePage(void);
std::string getLangName(void);
std::string getString(HINSTANCE, UINT);
char* getStringChar(HINSTANCE, UINT, char*, unsigned int);
std::string inline makeDefaultUserAgent(void);
void storeProgramInstance(HINSTANCE);

std::string inline makeDefaultUserAgent(void) {
    return makeUserAgent((char*)getString((HINSTANCE)NULL,IDS_USER_AGENT).c_str());
}

#endif
