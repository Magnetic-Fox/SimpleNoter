#ifndef NOTERAPI_H
#define NOTERAPI_H

#include <string>
#include "maths.hpp"
#include "requests.hpp"
#include "responses.hpp"
#include "jsonhelper.hpp"
#include "noteprocs.hpp"
#include "userprocs.hpp"
#include "constants.hpp"

#include "unbzip2.h"

typedef struct NOTER_CREDENTIALS {
    std::string username, password;
} NOTER_CREDENTIALS;

typedef struct NOTER_CONNECTION_SETTINGS {
    std::string ipAddress, share, userAgent;
    unsigned int port;
    bool requestCompression;
} NOTER_CONNECTION_SETTINGS;

typedef struct NOTER_SERVER_INFO {
    std::string name, timezone, version;
} NOTER_SERVER_INFO;

typedef struct NOTER_NOTES {
    NOTE_SUMMARY *notes;
    long int noteCount;
} NOTER_NOTES;

bool noter_credentialsAvailable(NOTER_CREDENTIALS&);
bool noter_connectionSettingsAvailable(NOTER_CONNECTION_SETTINGS&);
std::string noter_getAnswerString(long int);
NOTER_CREDENTIALS noter_prepareCredentials(char*, char*);
NOTER_CONNECTION_SETTINGS noter_prepareConnectionSettings(char*, unsigned int, char*, char*, bool);
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
unsigned int getCompressionRatio(void);

long int inline noter_getAnswerCode(NAMEDESCRIPTOR &descriptor) {
    return getSingleInteger(descriptor["answer_info_code"]);
}

bool inline noter_correctResponse(HEADERS &headers) {
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
                                              newPassword,
                                              connectionSettings.requestCompression).c_str(),
                        heads,
                        buffer);
}

bool inline noter_checkServerVersion(NOTER_SERVER_INFO &serverInfo) {
    return serverInfo.version==MATCH_VERSION;
}

#endif
