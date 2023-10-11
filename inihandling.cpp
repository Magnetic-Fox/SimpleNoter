#include "inihandling.hpp"

NOTER_CREDENTIALS getCredentials(char* iniFile)
{
    NOTER_CREDENTIALS credentials;
    char username[256];
    char password[256];
    GetPrivateProfileString("Credentials","Username","",username,256,iniFile);  // Yup, for Win3.11 placing "" here is better choice than just NULL (however, under WinXP NULL worked as it should).
    GetPrivateProfileString("Credentials","Password","",password,256,iniFile);  // Otherwise noter_prepareCredentials procedure produces garbage in credential settings (which is a bit strange).
    credentials=noter_prepareCredentials((char*)base64_decode(username).c_str(),(char*)makeSecureString_B64(false,password,(char*)base64_decode(username).c_str()).c_str());
    return credentials;
}

void saveCredentials(NOTER_CREDENTIALS &credentials, char* iniFile)
{
    WritePrivateProfileString("Credentials","Username",(char*)base64_encode((char*)credentials.username.c_str()).c_str(),iniFile);
    WritePrivateProfileString("Credentials","Password",(char*)makeSecureString_B64(true,(char*)credentials.password.c_str(),(char*)credentials.username.c_str()).c_str(),iniFile);
    return;
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

void saveConnectionSettings(NOTER_CONNECTION_SETTINGS &connectionSettings, char* iniFile)
{
    WritePrivateProfileString("Server","IP",(char*)connectionSettings.ipAddress.c_str(),iniFile);
    WritePrivateProfileString("Server","Port",(char*)IntToStr(connectionSettings.port).c_str(),iniFile);
    WritePrivateProfileString("Server","Share",(char*)connectionSettings.share.c_str(),iniFile);
    return;
}

MAINSETTINGS getMainSettings(char* iniFile)
{
    MAINSETTINGS mainSettings;
    mainSettings.mainWindowSystem=(GetPrivateProfileInt("Settings","MainWindowSystem",1,iniFile)==1);
    mainSettings.editWindowSystem=(GetPrivateProfileInt("Settings","EditWindowSystem",1,iniFile)==1);
    mainSettings.mainWindowStyle=GetPrivateProfileInt("Settings","MainWindowStyle",0,iniFile);
    mainSettings.editWindowStyle=GetPrivateProfileInt("Settings","EditWindowStyle",0,iniFile);
    mainSettings.autoReload=(GetPrivateProfileInt("Settings","AutoReload",0,iniFile)==1);
    mainSettings.autoRefresh=(GetPrivateProfileInt("Settings","AutoRefresh",0,iniFile)==1);
    mainSettings.savePosSizes=(GetPrivateProfileInt("Settings","SavePosSizes",0,iniFile)==1);
    mainSettings.use3DControls=(GetPrivateProfileInt("Settings","Use3DControls",0,iniFile)==1);
    mainSettings.use3DButtons=(GetPrivateProfileInt("Settings","Use3DButtons",0,iniFile)==1);
    mainSettings.use3DLists=(GetPrivateProfileInt("Settings","Use3DLists",0,iniFile)==1);
    mainSettings.use3DEdits=(GetPrivateProfileInt("Settings","Use3DEdits",0,iniFile)==1);
    mainSettings.use3DCombos=(GetPrivateProfileInt("Settings","Use3DCombos",0,iniFile)==1);
    mainSettings.use3DDialogs=(GetPrivateProfileInt("Settings","Use3DDialogs",0,iniFile)==1);
    return mainSettings;
}

void saveMainSettings(MAINSETTINGS &mainSettings, char* iniFile)
{
    WritePrivateProfileString("Settings","MainWindowSystem",(char*)IntToStr(mainSettings.mainWindowSystem).c_str(),iniFile);
    WritePrivateProfileString("Settings","EditWindowSystem",(char*)IntToStr(mainSettings.editWindowSystem).c_str(),iniFile);
    WritePrivateProfileString("Settings","MainWindowStyle",(char*)IntToStr(mainSettings.mainWindowStyle).c_str(),iniFile);
    WritePrivateProfileString("Settings","EditWindowStyle",(char*)IntToStr(mainSettings.editWindowStyle).c_str(),iniFile);
    WritePrivateProfileString("Settings","AutoReload",(char*)IntToStr(mainSettings.autoReload).c_str(),iniFile);
    WritePrivateProfileString("Settings","AutoRefresh",(char*)IntToStr(mainSettings.autoRefresh).c_str(),iniFile);
    WritePrivateProfileString("Settings","SavePosSizes",(char*)IntToStr(mainSettings.savePosSizes).c_str(),iniFile);
    WritePrivateProfileString("Settings","Use3DControls",(char*)IntToStr(mainSettings.use3DControls).c_str(),iniFile);
    WritePrivateProfileString("Settings","Use3DButtons",(char*)IntToStr(mainSettings.use3DButtons).c_str(),iniFile);
    WritePrivateProfileString("Settings","Use3DLists",(char*)IntToStr(mainSettings.use3DLists).c_str(),iniFile);
    WritePrivateProfileString("Settings","Use3DEdits",(char*)IntToStr(mainSettings.use3DEdits).c_str(),iniFile);
    WritePrivateProfileString("Settings","Use3DCombos",(char*)IntToStr(mainSettings.use3DCombos).c_str(),iniFile);
    WritePrivateProfileString("Settings","Use3DDialogs",(char*)IntToStr(mainSettings.use3DDialogs).c_str(),iniFile);
    return;
}
