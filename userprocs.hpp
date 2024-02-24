#ifndef USERPROCS_H
#define USERPROCS_H

#include <map>
#include "jsonhelper.hpp"

typedef struct USER_INFO {
    long int ID;
    std::string username;
    std::string dateRegistered;
    std::string userAgent;
    std::string lastChanged;
    std::string lastUserAgent;
} USERINFO;

bool indexUser(NAMEDESCRIPTOR&);
USER_INFO getUserInfo(NAMEDESCRIPTOR&);

#endif
