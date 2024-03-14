#ifndef JSONHELPER_H
#define JSONHELPER_H

#include <string>
#include <map>

#include "json.h"

typedef std::map<std::string,json_value*> NAMEDESCRIPTOR;
typedef std::map<std::string,int> NUMBERDESCRIPTOR;

void indexNames(json_value*, std::string, NAMEDESCRIPTOR&);
void indexNumbers(json_value*, std::string, NUMBERDESCRIPTOR&);
char* getArrayString(json_value*, unsigned long int);
json_value* getArrayElement(json_value*, unsigned long int);
long int getInteger(json_value*, unsigned long int);
char* getString(json_value*, unsigned long int);
long int getSingleInteger(json_value*);
char* getSingleString(json_value*);
bool jsonLooksValid(char*&, unsigned int&);

#endif
