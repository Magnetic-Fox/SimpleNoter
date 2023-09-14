#include "inihandling.hpp"

NOTER_CREDENTIALS getCredentials(char* iniFile)
{
    NOTER_CREDENTIALS credentials;
    char username[256];
    char password[256];
    GetPrivateProfileString("Credentials","Username",NULL,username,256,iniFile);
    GetPrivateProfileString("Credentials","Password",NULL,password,256,iniFile);
    credentials=noter_prepareCredentials(username,password);
    return credentials;
}

NOTER_CONNECTION_SETTINGS getConnectionSettings(char* iniFile)
{
    NOTER_CONNECTION_SETTINGS connectionSettings;
    char ipAddress[256];
    unsigned int port;
    char share[256];
    GetPrivateProfileString("Server","IP",NULL,ipAddress,256,iniFile);
    port=GetPrivateProfileInt("Server","Port",0,iniFile);
    GetPrivateProfileString("Server","Share",NULL,share,256,iniFile);
    connectionSettings=noter_prepareConnectionSettings(ipAddress,port,share,(char*)makeDefaultUserAgent().c_str());
    return connectionSettings;
}
