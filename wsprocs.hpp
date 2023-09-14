#ifndef WSPROCS_H
#define WSPROCS_H

#include <winsock.h>

#define MAKEWORD(a,b)   ((WORD)(((BYTE)(a))|(((WORD)((BYTE)(b)))<<8)))

int wsInit(void);
char* getHostIP(char*/*, DWORD* */);

#endif
