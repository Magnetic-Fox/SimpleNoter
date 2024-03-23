#ifndef REQUESTS_H
#define REQUESTS_H

#include <map>
#include <string>
#include <string.h>
#include <ctype.h>
#include <winsock.h>
#include <stdlib.h>
#include "wsprocs.hpp"

typedef std::map<std::string,std::string> HEADERS;

int getResponseCode(HEADERS&);
std::string prepareRequest(char*, char*, char*, char*, unsigned int, char*, char*);
void interpretHeaderLine(HEADERS&, std::string, bool);
std::string URLencode(char*);
std::string prepareContent(char*, char*, char*, char*, char*, unsigned long int, char*, bool);
unsigned int makeRequest(char*, unsigned int, char*, char*, char*, char*, char*, HEADERS&, char*);

#endif
