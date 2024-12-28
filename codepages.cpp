#include "codepages.hpp"

static long int the_index=  0;
static long int the_length= 0;
static long int the_char=   0;
static long int the_byte=   0;
static char     *the_input;

// Neutral defaults (7-bit characters only), but changeable
static std::string encodeError= ".";
static std::string decodeError= "?";
static std::string utfError=    "~";

static bool encodeWarning=  false;
static bool decodeWarning=  false;

std::string fromCodePage(RAWCODEPAGE nmCodePage, char* input) {
    std::string temp="";
    encodeWarning=false;
    for(unsigned long int x=0; x<strlen(input); ++x) {
        if(input[x]<0x80) {
            temp+=input[x];
        }
        else {
            char xtest[8];
            int xout=utf8_encode(xtest,nmCodePage[input[x]-0x80]);
            if(xout==0) {
                temp+=encodeError;
                encodeWarning=true;
            }
            else {
                for(unsigned int y=0; y<xout; ++y) {
                    temp+=xtest[y];
                }
            }
        }
    }
    return temp;
}

std::string toCodePage(CODEPAGE &codepage, char* input) {
    std::string temp="";
    utf8_decode_init(input,strlen(input));
    decodeWarning=false;
    while(true) {
        long int one=utf8_decode_next();
        if(one<0) {
            if(one==UTF8_ERROR) {
                temp+=utfError;
            }
            break;
        }
        else {
            if(one<0x80) {
                temp+=(char)one;
            }
            else {
                char test=(char)codepage[(unsigned int)one];
                if(test<0x20) {
                    if(!((test==0x0D) || (test==0x0A))) {
                        temp+=decodeError;
                        decodeWarning=true;
                        continue;
                    }
                }
                temp+=test;
            }
        }
    }
    return temp;
}

void prepareCodePage(CODEPAGE &codepage, RAWCODEPAGE cpdef) {
    codepage.clear();
    for(unsigned int x=0; x<128; ++x) {
        codepage[cpdef[x]]=0x80+x;
    }
    return;
}

bool encodeWarningState(void) {
    return encodeWarning;
}

bool decodeWarningState(void) {
    return decodeWarning;
}

bool loadCodePage(char *libName, HINSTANCE &hCodePageLib, HGLOBAL &hCodePageDefinition, RAWCODEPAGE &rawCodePage) {
    char testString[9];
    char testString2[17];
    hCodePageLib=LoadLibrary(libName);
    if(hCodePageLib < 32) {
        return false;
    }
    else {
        if(LoadString(hCodePageLib,IDS_LIBTYPE,testString,9)) {
            if(((std::string)testString)=="CODEPAGE") {
                if(LoadString(hCodePageLib,IDS_USEDWORD,testString,9)) {
                    // non-DWORD code page definition is the only allowed in this version (might be changed in the future)
                    if(((std::string)testString)=="0") {
                        hCodePageDefinition=LoadResource(hCodePageLib,FindResource(hCodePageLib,MAKEINTRESOURCE(IDR_CODEPAGE),RT_RCDATA));
                        if(hCodePageDefinition==NULL) {
                            return false;
                        }
                        else {
                            // Load code page definition
                            rawCodePage=(int*)LockResource(hCodePageDefinition);
                            // And now load optional error strings (change neutral defaults to something another)
                            if(LoadString(hCodePageLib,IDS_ENCODEERROR,testString2,17)) {
                                // Make it UTF-8 string immediately (to avoid any further errors)
                                encodeError=fromCodePage(rawCodePage,testString2);
                            }
                            if(LoadString(hCodePageLib,IDS_DECODEERROR,testString2,17)) {
                                decodeError=testString2;
                            }
                            if(LoadString(hCodePageLib,IDS_UTFERROR,testString2,17)) {
                                utfError=testString2;
                            }
                            // Finish successfully
                            return true;
                        }
                    }
                    else {
                        return false;
                    }
                }
                else {
                    return false;
                }
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
    }
}

void unloadCodePage(HINSTANCE &hCodePageLib, HGLOBAL &hCodePageDefinition) {
    if(hCodePageDefinition!=NULL) {
        UnlockResource(hCodePageDefinition);
    }
    if(hCodePageDefinition!=NULL) {
        FreeResource(hCodePageDefinition);
    }
    if(hCodePageLib!=NULL) {
        FreeLibrary(hCodePageLib);
    }
    return;
}

bool loadAndPrepareCodePage(MAINSETTINGS &mainSettings, LIBRARIES &libraries, HINSTANCE &hCodePageLib, HGLOBAL &hCodePageDefinition, RAWCODEPAGE &rawCodePage, CODEPAGE &mappedCodePage) {
    std::string foundCodePage="";
    if(!loadCodePage((char*)mainSettings.selectedCodePage.c_str(),hCodePageLib,hCodePageDefinition,rawCodePage)) {
        foundCodePage=findAnyCodePage(libraries);
        if(loadCodePage((char*)foundCodePage.c_str(),hCodePageLib,hCodePageDefinition,rawCodePage)) {
            mainSettings.selectedCodePage=foundCodePage;
        }
        else {
            return false;
        }
    }
    prepareCodePage(mappedCodePage,rawCodePage);
    return true;
}
