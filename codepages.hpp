#ifndef CODEPAGES_H
#define CODEPAGES_H

#include <string>
#include <map>
#include <string.h>
#include <windows.h>
#include "libutil.hpp"
#include "definitions.hpp"
#include "utf8_encode.hpp"
#include "utf8_decode.hpp"

typedef std::map<int, int> CODEPAGE;
typedef const int* RAWCODEPAGE;

std::string fromCodePage(RAWCODEPAGE, char*);
std::string toCodePage(CODEPAGE&, char*);
void prepareCodePage(CODEPAGE&, RAWCODEPAGE);
bool encodeWarningState(void);
bool decodeWarningState(void);
bool loadCodePage(char*, HINSTANCE&, HGLOBAL&, RAWCODEPAGE&);
void unloadCodePage(HINSTANCE&, HGLOBAL&);
bool loadAndPrepareCodePage(MAINSETTINGS&, LIBRARIES&, HINSTANCE&, HGLOBAL&, RAWCODEPAGE&, CODEPAGE&);

#endif
