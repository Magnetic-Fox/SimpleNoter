#ifndef SIMPLECRYPTO_H
#define SIMPLECRYPTO_H

#include <string>
#include <string.h>
#include "base64.hpp"

std::string secureString(bool, char*, char*);
std::string makeSecureString_B64(bool, char*, char*);

#endif
