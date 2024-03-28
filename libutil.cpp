#include "libutil.hpp"

unsigned short int getLibType(HINSTANCE hLib) {
    char testString[12];
    if(hLib < 32) {
        return LIB_UNKNOWN;
    }
    else {
        if(LoadString(hLib,IDS_LIBTYPE,testString,12)) {
            if(((std::string)testString)=="CODEPAGE") {
                return LIB_CODEPAGE;
            }
            else if(((std::string)testString)=="STRINGTABLE") {
                return LIB_STRINGTABLE;
            }
            else {
                return LIB_UNKNOWN;
            }
        }
        else {
            return LIB_UNKNOWN;
        }
    }
}

std::string getCodePageInfo(HINSTANCE hLib) {
    char testString[12];
    if(hLib < 32) {
        return "";
    }
    else {
        if(LoadString(hLib,IDS_CPNAME,testString,12)) {
            return (std::string)testString;
        }
        else {
            return "";
        }
    }
}

bool listAvailableLibs(char* appPath, LIBRARIES &libraries) {
    HINSTANCE hLib=NULL;
    bool anyFound=false;
    std::string searchString=getCurrentDirectory(appPath)+"*.DLL";
    DIR *directory=opendir(searchString.c_str());
    std::string temp="";
    LIB_FILE_INFO fileInfo;
    libraries.clear();
    while(directory=readdir(directory)) {
        hLib=LoadLibrary(directory->d_name);
        switch(getLibType(hLib)) {
            case LIB_CODEPAGE:
                fileInfo.type=LIB_CODEPAGE;
                fileInfo.filename=directory->d_name;
                temp=getCodePageInfo(hLib);
                if(temp!="") {
                    fileInfo.relatedInfo=temp;
                    libraries.push_back(fileInfo);
                    if(!anyFound) {
                        anyFound=true;
                    }
                }
                break;
            case LIB_STRINGTABLE:
                fileInfo.type=LIB_STRINGTABLE;
                fileInfo.filename=directory->d_name;
                // TODO: all the rest like above
                break;
            default:
                break;
        }
        FreeLibrary(hLib);
    }
    return anyFound;
}

std::string findAnyCodePage(LIBRARIES &libraries) {
    LIB_ITER it;
    std::string selectedFile="";
    std::string defaultCodePage=getCodePage();
    for(it=libraries.begin(); it!=libraries.end(); ++it) {
        if(it->type==LIB_CODEPAGE) {
            if(selectedFile=="") {
                selectedFile=it->filename;
            }
            if(it->relatedInfo==defaultCodePage) {
                selectedFile=it->filename;
                return selectedFile;    // default system code page found - it is the best result
            }
        }
    }
    return selectedFile;
}
