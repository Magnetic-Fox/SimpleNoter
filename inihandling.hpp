#ifndef INIHANDLING_H
#define INIHANDLING_H

#include <string>
#include <windows.h>

#include "base64.hpp"
#include "noterapi.hpp"
#include "additional.hpp"
#include "definitions.hpp"
#include "constants.hpp"
#include "conversion.hpp"
#include "simplecrypto.hpp"

NOTER_CREDENTIALS getCredentials(char*);
void saveCredentials(NOTER_CREDENTIALS&, char*);
NOTER_CONNECTION_SETTINGS getConnectionSettings(char*);
void saveConnectionSettings(NOTER_CONNECTION_SETTINGS&, char*);
MAINSETTINGS getMainSettings(char*);
void saveMainSettings(MAINSETTINGS&, char*);
std::string inline getDefaultIniFile(char*);

std::string inline getDefaultIniFile(char* input)
{
    return getCurrentDirectory(input)+INI_FILE;
}

#endif
