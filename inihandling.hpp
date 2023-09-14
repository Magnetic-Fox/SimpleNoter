#ifndef INIHANDLING_H
#define INIHANDLING_H

#include <string>
#include <windows.h>

#include "noterapi.hpp"
#include "additional.hpp"
#include "constants.hpp"

NOTER_CREDENTIALS getCredentials(char*);
NOTER_CONNECTION_SETTINGS getConnectionSettings(char*);
std::string inline getDefaultIniFile(char*);

std::string inline getDefaultIniFile(char* input)
{
    return getCurrentDirectory(input)+INI_FILE;
}

#endif
