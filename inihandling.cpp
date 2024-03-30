#include "inihandling.hpp"

NOTER_CREDENTIALS getCredentials(char* iniFile) {
    NOTER_CREDENTIALS credentials;
    char username[256];
    char password[256];
    GetPrivateProfileString("Credentials","Username","",username,256,iniFile);  // Yup, for Win3.11 placing "" here is better choice than just NULL (however, under WinXP NULL worked as it should).
    GetPrivateProfileString("Credentials","Password","",password,256,iniFile);  // Otherwise noter_prepareCredentials procedure produces garbage in credential settings (which is a bit strange).
    credentials=noter_prepareCredentials((char*)base64_decode(username).c_str(),(char*)makeSecureString_B64(false,password,(char*)base64_decode(username).c_str()).c_str());
    return credentials;
}

void saveCredentials(NOTER_CREDENTIALS &credentials, char* iniFile) {
    WritePrivateProfileString("Credentials","Username",(char*)base64_encode((char*)credentials.username.c_str()).c_str(),iniFile);
    WritePrivateProfileString("Credentials","Password",(char*)makeSecureString_B64(true,(char*)credentials.password.c_str(),(char*)credentials.username.c_str()).c_str(),iniFile);
    return;
}

NOTER_CONNECTION_SETTINGS getConnectionSettings(char* iniFile) {
    NOTER_CONNECTION_SETTINGS connectionSettings;
    char serverAddress[256];
    unsigned int port;
    char share[256];
    bool requestCompression;
    GetPrivateProfileString("Server","Address","",serverAddress,256,iniFile);   // quick, forgotten change
    port=GetPrivateProfileInt("Server","Port",0,iniFile);
    GetPrivateProfileString("Server","Share","",share,256,iniFile);             // quick, forgotten change
    requestCompression=(GetPrivateProfileInt("Server","RequestCompression",0,iniFile)==1);
    connectionSettings=noter_prepareConnectionSettings(serverAddress,port,share,(char*)makeDefaultUserAgent().c_str(),requestCompression);
    return connectionSettings;
}

void saveConnectionSettings(NOTER_CONNECTION_SETTINGS &connectionSettings, char* iniFile) {
    WritePrivateProfileString("Server","Address",(char*)connectionSettings.serverAddress.c_str(),iniFile);
    WritePrivateProfileString("Server","Port",(char*)IntToStr(connectionSettings.port).c_str(),iniFile);
    WritePrivateProfileString("Server","Share",(char*)connectionSettings.share.c_str(),iniFile);
    WritePrivateProfileString("Server","RequestCompression",(char*)IntToStr(connectionSettings.requestCompression).c_str(),iniFile);
    return;
}

MAINSETTINGS getMainSettings(char* iniFile, LIBRARIES *libraries) {
    MAINSETTINGS mainSettings;
    char temp[256];

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

    mainSettings.mainWindowX=GetPrivateProfileInt("WindowSettings","MainWindowX",CW_USEDEFAULT,iniFile);
    mainSettings.mainWindowY=GetPrivateProfileInt("WindowSettings","MainWindowY",CW_USEDEFAULT,iniFile);
    mainSettings.mainWindowSizeX=GetPrivateProfileInt("WindowSettings","MainWindowSizeX",CW_USEDEFAULT,iniFile);
    mainSettings.mainWindowSizeY=GetPrivateProfileInt("WindowSettings","MainWindowSizeY",CW_USEDEFAULT,iniFile);
    mainSettings.editWindowX=GetPrivateProfileInt("WindowSettings","EditWindowX",CW_USEDEFAULT,iniFile);
    mainSettings.editWindowY=GetPrivateProfileInt("WindowSettings","EditWindowY",CW_USEDEFAULT,iniFile);
    mainSettings.editWindowSizeX=GetPrivateProfileInt("WindowSettings","EditWindowSizeX",CW_USEDEFAULT,iniFile);
    mainSettings.editWindowSizeY=GetPrivateProfileInt("WindowSettings","EditWindowSizeY",CW_USEDEFAULT,iniFile);

    if(libraries==NULL) {
        GetPrivateProfileString("Regional","CodePage","",temp,256,iniFile);
    }
    else {
        GetPrivateProfileString("Regional","CodePage",(char*)findAnyCodePage(*libraries).c_str(),temp,256,iniFile);
    }

    mainSettings.selectedCodePage=(std::string)temp;
    
    return mainSettings;
}

void saveMainSettings(MAINSETTINGS &mainSettings, char* iniFile) {  // really main, without window coordinates
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
    WritePrivateProfileString("Regional","CodePage",(char*)mainSettings.selectedCodePage.c_str(),iniFile);
    return;
}

void saveWindowCoordinatesSettings(MAINSETTINGS &mainSettings, char* iniFile) {
    WritePrivateProfileString("WindowSettings","MainWindowX",(char*)IntToStr(mainSettings.mainWindowX).c_str(),iniFile);
    WritePrivateProfileString("WindowSettings","MainWindowY",(char*)IntToStr(mainSettings.mainWindowY).c_str(),iniFile);
    WritePrivateProfileString("WindowSettings","MainWindowSizeX",(char*)IntToStr(mainSettings.mainWindowSizeX).c_str(),iniFile);
    WritePrivateProfileString("WindowSettings","MainWindowSizeY",(char*)IntToStr(mainSettings.mainWindowSizeY).c_str(),iniFile);
    WritePrivateProfileString("WindowSettings","EditWindowX",(char*)IntToStr(mainSettings.editWindowX).c_str(),iniFile);
    WritePrivateProfileString("WindowSettings","EditWindowY",(char*)IntToStr(mainSettings.editWindowY).c_str(),iniFile);
    WritePrivateProfileString("WindowSettings","EditWindowSizeX",(char*)IntToStr(mainSettings.editWindowSizeX).c_str(),iniFile);
    WritePrivateProfileString("WindowSettings","EditWindowSizeY",(char*)IntToStr(mainSettings.editWindowSizeY).c_str(),iniFile);
    WritePrivateProfileString("Settings","MainWindowStyle",(char*)IntToStr(mainSettings.mainWindowStyle).c_str(),iniFile);
    WritePrivateProfileString("Settings","EditWindowStyle",(char*)IntToStr(mainSettings.editWindowStyle).c_str(),iniFile);    
    return;
}
