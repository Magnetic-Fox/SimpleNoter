#ifndef NOTERAPI_H
#define NOTERAPI_H

#include <string>
#include "requests.hpp"
#include "jsonhelper.hpp"
#include "noteprocs.hpp"
#include "userprocs.hpp"
#include "constants.hpp"

#define ERROR_WRONG_RESPONSE            -1024

#define ERROR_SERVICE_DISABLED          -768
#define ERROR_INTERNAL_SERVER_ERROR     -512
#define ERROR_NOTE_ALREADY_UNLOCKED     -14
#define ERROR_NOTE_ALREADY_LOCKED       -13
#define ERROR_NOTE_LOCKED               -12
#define ERROR_USER_REMOVAL_FAILURE      -11
#define ERROR_USER_NOT_EXISTS           -10
#define ERROR_NOTE_NOT_EXISTS           -9
#define ERROR_NO_NECESSARY_INFORMATION  -8
#define ERROR_USER_DEACTIVATED          -7
#define ERROR_LOGIN_INCORRECT           -6
#define ERROR_UNKNOWN_ACTION            -5
#define ERROR_NO_CREDENTIALS            -4
#define ERROR_USER_EXISTS               -3
#define ERROR_NO_USABLE_INFORMATION     -2
#define ERROR_INVALID_METHOD            -1

#define INFO_OK                         0

#define INFO_USER_CREATED               1
#define INFO_USER_UPDATED               2
#define INFO_USER_REMOVED               3
#define INFO_LIST_SUCCESSFUL            4
#define INFO_NOTE_RETRIEVED             5
#define INFO_NOTE_CREATED               6
#define INFO_NOTE_UPDATED               7
#define INFO_NOTE_DELETED               8
#define INFO_USER_INFO_RETRIEVED        9
#define INFO_NOTE_LOCKED                10
#define INFO_NOTE_UNLOCKED              11

typedef struct NOTER_CREDENTIALS
{
    std::string username, password;
} NOTER_CREDENTIALS;

typedef struct NOTER_CONNECTION_SETTINGS
{
    std::string ipAddress, share, userAgent;
    unsigned int port;
} NOTER_CONNECTION_SETTINGS;

typedef struct NOTER_SERVER_INFO
{
    std::string name, timezone, version;
} NOTER_SERVER_INFO;

typedef struct NOTER_NOTES
{
    NOTE_SUMMARY *notes;
    long int noteCount;
} NOTER_NOTES;

NOTER_CREDENTIALS noter_prepareCredentials(char*, char*);
NOTER_CONNECTION_SETTINGS noter_prepareConnectionSettings(char*, unsigned int, char*, char*);
bool noter_checkAndPrepareResponse(HEADERS&, char*&, unsigned int&, json_value*&, NAMEDESCRIPTOR&);
NOTER_SERVER_INFO noter_getServerInfo(NOTER_CONNECTION_SETTINGS&, char*);
long int noter_getNoteList(NOTER_CONNECTION_SETTINGS&, NOTER_CREDENTIALS&, char*, NOTE_SUMMARY*&);
long int noter_getNote(NOTER_CONNECTION_SETTINGS&, NOTER_CREDENTIALS&, unsigned long int, char*, NOTE&);
long int inline noter_getAnswerCode(NAMEDESCRIPTOR&);
bool inline noter_correctResponse(HEADERS&);
unsigned int inline noter_simplyMakeRequest(char*, NOTER_CONNECTION_SETTINGS&, NOTER_CREDENTIALS*, char*, char*, char*, unsigned long int, char*, HEADERS&, char*);
bool inline noter_checkServerVersion(NOTER_SERVER_INFO&);
long int noter_getUserInfo(NOTER_CONNECTION_SETTINGS&, NOTER_CREDENTIALS&, char*, USER_INFO&);
long int noter_lockNote(NOTER_CONNECTION_SETTINGS&, NOTER_CREDENTIALS&, unsigned long int, char*);
long int noter_unlockNote(NOTER_CONNECTION_SETTINGS&, NOTER_CREDENTIALS&, unsigned long int, char*);
long int noter_deleteNote(NOTER_CONNECTION_SETTINGS&, NOTER_CREDENTIALS&, unsigned long int, char*);
long int noter_addNote(NOTER_CONNECTION_SETTINGS&, NOTER_CREDENTIALS&, NOTE&, char*);
long int noter_updateNote(NOTER_CONNECTION_SETTINGS&, NOTER_CREDENTIALS&, NOTE&, char*);
long int noter_registerUser(NOTER_CONNECTION_SETTINGS&, NOTER_CREDENTIALS&, char*);
long int noter_changeUserPassword(NOTER_CONNECTION_SETTINGS&, NOTER_CREDENTIALS&, char*, char*);
long int noter_removeUser(NOTER_CONNECTION_SETTINGS&, NOTER_CREDENTIALS&, char*);

long int inline noter_getAnswerCode(NAMEDESCRIPTOR &descriptor)
{
    return getSingleInteger(descriptor["answer_info_code"]);
}

bool inline noter_correctResponse(HEADERS &headers)
{
    return ((getResponseCode(headers)==200) && (headers["Content-Type"]=="application/json"));
}

unsigned int inline noter_simplyMakeRequest(char* method, NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS *credentials,
                                            char* action, char* subject, char* entry, unsigned long int noteID, char* newPassword, HEADERS &heads,
                                            char* buffer)
{
    return  makeRequest((char*)connectionSettings.ipAddress.c_str(),
                        connectionSettings.port,
                        method,
                        (char*)connectionSettings.share.c_str(),
                        (char*)connectionSettings.userAgent.c_str(),
                        "application/x-www-form-urlencoded",
                        (char*)prepareContent(action,
                                              (credentials==NULL)?"":(char*)credentials->username.c_str(),
                                              (credentials==NULL)?"":(char*)credentials->password.c_str(),
                                              subject,
                                              entry,
                                              noteID,
                                              newPassword).c_str(),
                        heads,
                        buffer);
}

bool inline noter_checkServerVersion(NOTER_SERVER_INFO &serverInfo)
{
    return serverInfo.version==MATCH_VERSION;
}

#endif
