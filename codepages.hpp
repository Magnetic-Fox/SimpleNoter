#ifndef CODEPAGES_H
#define CODEPAGES_H

#include <string>
#include <map>
#include <string.h>
#include "utf8_encode.hpp"
#include "utf8_decode.hpp"

typedef std::map<int, int> CODEPAGE;
typedef const int* RAWCODEPAGE;

std::string fromCodePage(RAWCODEPAGE, char*);
std::string toCodePage(CODEPAGE&, char*);
void prepareCodePage(CODEPAGE&, RAWCODEPAGE);

bool decodeWarningState(void);

#endif
