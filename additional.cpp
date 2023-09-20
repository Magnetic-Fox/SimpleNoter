#include "additional.hpp"

std::string winVersionString(void)
{
    std::string temp="Windows ";
    char conv[8];

    DWORD dwVersion = 0; 
    DWORD dwMajorVersion = 0;
    DWORD dwMinorVersion = 0; 
    DWORD dwBuild = 0;

    dwVersion=GetVersion();

    dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

    ltoa(dwMajorVersion,conv,10);
    temp=temp+conv+'.';
    ltoa(dwMinorVersion,conv,10);
    temp=temp+conv;

    if(dwVersion<0x80000000)
    {              
        dwBuild = (DWORD)(HIWORD(dwVersion));
        ltoa(dwBuild,conv,10);
        temp=temp+'.'+conv;
    }

    return temp;
}

std::string makeUserAgent(char* userAgent)
{
    std::string temp=userAgent;
    temp=temp+" ("+winVersionString()+")";
    return temp;
}

std::string getCurrentDirectory(char* input)
{
    std::string currentDirectory="";
    unsigned int pos=0;
    for(int x=strlen(input)-1; x>=0; --x)
    {
        if(input[x]=='\\')
        {
            pos=x;
            break;
        }
    }
    for(unsigned int x=0; x<pos+1; ++x)
    {
        currentDirectory=currentDirectory+input[x];
    }
    return currentDirectory;
}
