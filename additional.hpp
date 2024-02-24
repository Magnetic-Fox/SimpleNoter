#ifndef ADDITIONAL_H
#define ADDITIONAL_H

#include <string>
#include "string.h"
#include <stdlib.h>
#include <windows.h>
#include "constants.hpp"

std::string winVersionString(void);
std::string makeUserAgent(char*);
std::string getCurrentDirectory(char*);
std::string inline makeDefaultUserAgent(void);

std::string inline makeDefaultUserAgent(void) {
    return makeUserAgent(USER_AGENT);
}

#endif
