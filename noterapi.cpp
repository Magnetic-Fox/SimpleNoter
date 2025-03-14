#include "noterapi.hpp"

static unsigned int compressionRatio=0;

bool noter_credentialsAvailable(NOTER_CREDENTIALS &credentials) {
    return ((credentials.username.length()>0) && (credentials.password.length()>0));
}

bool noter_connectionSettingsAvailable(NOTER_CONNECTION_SETTINGS &connectionSettings) {
    return ((connectionSettings.serverAddress.length()>0) && (connectionSettings.port>0) && (connectionSettings.share.length()>0));
}

std::string noter_getAnswerString(long int answerCode) {
    switch(answerCode) {
        case ERROR_WRONG_RESPONSE:
            return (std::string)getStringFromTable(IDS_STRING_WRONG_RESPONSE);
        case ERROR_SERVICE_DISABLED:
            return (std::string)getStringFromTable(IDS_STRING_SERVICE_DISABLED);
        case ERROR_INTERNAL_SERVER_ERROR:
            return (std::string)getStringFromTable(IDS_STRING_INTERNAL_SERVER_ERROR);
        case ERROR_NOTE_ALREADY_UNLOCKED:
            return (std::string)getStringFromTable(IDS_STRING_NOTE_ALREADY_UNLOCKED);
        case ERROR_NOTE_ALREADY_LOCKED:
            return (std::string)getStringFromTable(IDS_STRING_NOTE_ALREADY_LOCKED);
        case ERROR_NOTE_LOCKED:
            return (std::string)getStringFromTable(IDS_STRING_NOTE_LOCKED);
        case ERROR_USER_REMOVAL_FAILURE:
            return (std::string)getStringFromTable(IDS_STRING_USER_REMOVAL_FAILURE);
        case ERROR_USER_NOT_EXISTS:
            return (std::string)getStringFromTable(IDS_STRING_USER_NOT_EXISTS);
        case ERROR_NOTE_NOT_EXISTS:
            return (std::string)getStringFromTable(IDS_STRING_NOTE_NOT_EXISTS);
        case ERROR_NO_NECESSARY_INFORMATION:
            return (std::string)getStringFromTable(IDS_STRING_NO_NECESSARY_INFORMATION);
        case ERROR_USER_DEACTIVATED:
            return (std::string)getStringFromTable(IDS_STRING_USER_DEACTIVATED);
        case ERROR_LOGIN_INCORRECT:
            return (std::string)getStringFromTable(IDS_STRING_LOGIN_INCORRECT);
        case ERROR_UNKNOWN_ACTION:
            return (std::string)getStringFromTable(IDS_STRING_UNKNOWN_ACTION);
        case ERROR_NO_CREDENTIALS:
            return (std::string)getStringFromTable(IDS_STRING_NO_CREDENTIALS);
        case ERROR_USER_EXISTS:
            return (std::string)getStringFromTable(IDS_STRING_USER_EXISTS);
        case ERROR_NO_USABLE_INFORMATION:
            return (std::string)getStringFromTable(IDS_STRING_NO_USABLE_INFORMATION);
        case ERROR_INVALID_METHOD:
            return (std::string)getStringFromTable(IDS_STRING_INVALID_METHOD);
        case INFO_OK:
            return (std::string)getStringFromTable(IDS_STRING_INFO_OK);
        case INFO_USER_CREATED:
            return (std::string)getStringFromTable(IDS_STRING_INFO_USER_CREATED);
        case INFO_USER_UPDATED:
            return (std::string)getStringFromTable(IDS_STRING_INFO_USER_UPDATED);
        case INFO_USER_REMOVED:
            return (std::string)getStringFromTable(IDS_STRING_INFO_USER_REMOVED);
        case INFO_LIST_SUCCESSFUL:
            return (std::string)getStringFromTable(IDS_STRING_INFO_LIST_SUCCESSFUL);
        case INFO_NOTE_RETRIEVED:
            return (std::string)getStringFromTable(IDS_STRING_INFO_NOTE_RETRIEVED);
        case INFO_NOTE_CREATED:
            return (std::string)getStringFromTable(IDS_STRING_INFO_NOTE_CREATED);
        case INFO_NOTE_UPDATED:
            return (std::string)getStringFromTable(IDS_STRING_INFO_NOTE_UPDATED);
        case INFO_NOTE_DELETED:
            return (std::string)getStringFromTable(IDS_STRING_INFO_NOTE_DELETED);
        case INFO_USER_INFO_RETRIEVED:
            return (std::string)getStringFromTable(IDS_STRING_INFO_USER_INFO_RETRIEVED);
        case INFO_NOTE_LOCKED:
            return (std::string)getStringFromTable(IDS_STRING_INFO_NOTE_LOCKED);
        case INFO_NOTE_UNLOCKED:
            return (std::string)getStringFromTable(IDS_STRING_INFO_NOTE_UNLOCKED);
        default:
            return (std::string)getStringFromTable(IDS_STRING_UNKNOWN_ERROR);
    }
}

NOTER_CREDENTIALS noter_prepareCredentials(char* username, char* password) {
    NOTER_CREDENTIALS credentials;
    credentials.username=username;
    credentials.password=password;
    return credentials;
}

NOTER_CONNECTION_SETTINGS noter_prepareConnectionSettings(char* serverAddress, unsigned int port, char* share, char* userAgent, bool requestCompression) {
    NOTER_CONNECTION_SETTINGS connectionSettings;
    connectionSettings.serverAddress=serverAddress;
    connectionSettings.port=port;
    connectionSettings.share=share;
    connectionSettings.userAgent=userAgent;
    connectionSettings.requestCompression=requestCompression;
    return connectionSettings;
}

bool noter_checkAndPrepareResponse(HEADERS &heads, char *&buffer, unsigned int &bufDataSize, json_value *&jsonData, NAMEDESCRIPTOR &desc) {
    unsigned int tempBufDataSize;
    if(noter_correctResponse(heads)) {
        if(bufDataSize>0) {
            tempBufDataSize=bufDataSize;
            if(heads[BZ_COMPRESSED_HEADER]==TRUE_ANSWER) {
                bufDataSize=(unsigned int)uncompressDataInPlace((unsigned char*)buffer,bufDataSize,65535);
            }
            compressionRatio=calculateCompressionRatio(tempBufDataSize,bufDataSize);
            if(bufDataSize>0) {
                if(jsonLooksValid(buffer,bufDataSize)) {
                    jsonData=json_parse(buffer,bufDataSize);
                    if(jsonData==NULL) {
                        return false;            
                    }
                    else {
                        return indexMainResponse(jsonData, desc);
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

NOTER_SERVER_INFO noter_getServerInfo(NOTER_CONNECTION_SETTINGS &connectionSettings, char *buffer) {
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    NOTER_SERVER_INFO serverInfo;

    recSize=noter_simplyMakeRequest(GET_METHOD,
                                    connectionSettings,
                                    NULL,
                                    "",
                                    "",
                                    "",
                                    0,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc)) {
        if(noter_getAnswerCode(desc)==INFO_OK) {
            serverInfo.name=    getSingleString(desc["server_name"]);
            serverInfo.timezone=getSingleString(desc["server_timezone"]);
            serverInfo.version= getSingleString(desc["server_version"]);
        }
        json_value_free(data);
    }
    
    return serverInfo;
}

long int noter_getNoteList(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, char* buffer, NOTE_SUMMARY *&notes) {
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    NUMBERDESCRIPTOR desc2;
    json_value *data;
    long int noteCount=0;

    recSize=noter_simplyMakeRequest(POST_METHOD,
                                    connectionSettings,
                                    &credentials,
                                    LIST_ACTION,
                                    "",
                                    "",
                                    0,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc)) {
        noteCount=noter_getAnswerCode(desc);
        if(noteCount==INFO_LIST_SUCCESSFUL) {
            noteCount=getNoteList(desc,desc2,notes);
        }
        json_value_free(data);
    }
    else {
        noteCount=ERROR_WRONG_RESPONSE;
    }

    return noteCount;
}

long int noter_getNote(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, unsigned long int noteID, char* buffer, NOTE &note) {
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    NUMBERDESCRIPTOR desc2;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest(POST_METHOD,
                                    connectionSettings,
                                    &credentials,
                                    RETRIEVE_ACTION,
                                    "",
                                    "",
                                    noteID,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc)) {
        answerCode=noter_getAnswerCode(desc);
        if(answerCode==INFO_NOTE_RETRIEVED) {
            note=getNote(desc);
        }
        json_value_free(data);
    }
    else {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_getUserInfo(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, char* buffer, USER_INFO &userInfo) {
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest(POST_METHOD,
                                    connectionSettings,
                                    &credentials,
                                    INFO_ACTION,
                                    "",
                                    "",
                                    0,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc)) {
        answerCode=noter_getAnswerCode(desc);
        if(answerCode==INFO_USER_INFO_RETRIEVED) {
            userInfo=getUserInfo(desc);
        }
        json_value_free(data);
    }
    else {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_lockNote(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, unsigned long int noteID, char* buffer) {
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest(POST_METHOD,
                                    connectionSettings,
                                    &credentials,
                                    LOCK_ACTION,
                                    "",
                                    "",
                                    noteID,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc)) {
        answerCode=noter_getAnswerCode(desc);
        json_value_free(data);
    }
    else {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_unlockNote(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, unsigned long int noteID, char* buffer) {
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest(POST_METHOD,
                                    connectionSettings,
                                    &credentials,
                                    UNLOCK_ACTION,
                                    "",
                                    "",
                                    noteID,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc)) {
        answerCode=noter_getAnswerCode(desc);
        json_value_free(data);
    }
    else {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_deleteNote(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, unsigned long int noteID, char* buffer) {
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest(POST_METHOD,
                                    connectionSettings,
                                    &credentials,
                                    DELETE_ACTION,
                                    "",
                                    "",
                                    noteID,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc)) {
        answerCode=noter_getAnswerCode(desc);
        json_value_free(data);
    }
    else {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_addNote(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, NOTE &note, char* buffer) {
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;
    long int newID=0;

    recSize=noter_simplyMakeRequest(POST_METHOD,
                                    connectionSettings,
                                    &credentials,
                                    ADD_ACTION,
                                    (char*)note.subject.c_str(),
                                    (char*)note.entry.c_str(),
                                    0,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc)) {
        answerCode=noter_getAnswerCode(desc);
        if(answerCode==INFO_NOTE_CREATED) {
            newID=getNewID(desc);
            if(newID>0) {
                note.id=newID;
            }
            else {
                note.id=0;
                answerCode=ERROR_WRONG_RESPONSE;
            }
        }
        json_value_free(data);
    }
    else {
        answerCode=ERROR_WRONG_RESPONSE;
    }
    return answerCode;
}

long int noter_updateNote(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, NOTE &note, char* buffer) {
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest(POST_METHOD,
                                    connectionSettings,
                                    &credentials,
                                    UPDATE_ACTION,
                                    (char*)note.subject.c_str(),
                                    (char*)note.entry.c_str(),
                                    note.id,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc)) {
        answerCode=noter_getAnswerCode(desc);
        json_value_free(data);
    }
    else {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_registerUser(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, char* buffer) {
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest(POST_METHOD,
                                    connectionSettings,
                                    &credentials,
                                    REGISTER_ACTION,
                                    "",
                                    "",
                                    0,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc)) {
        answerCode=noter_getAnswerCode(desc);
        json_value_free(data);
    }
    else {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_changeUserPassword(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, char* newPassword, char* buffer) {
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest(POST_METHOD,
                                    connectionSettings,
                                    &credentials,
                                    CHANGE_ACTION,
                                    "",
                                    "",
                                    0,
                                    newPassword,
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc)) {
        answerCode=noter_getAnswerCode(desc);
        json_value_free(data);
    }
    else {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_removeUser(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, char* buffer) {
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest(POST_METHOD,
                                    connectionSettings,
                                    &credentials,
                                    REMOVE_ACTION,
                                    "",
                                    "",
                                    0,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc)) {
        answerCode=noter_getAnswerCode(desc);
        json_value_free(data);
    }
    else {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

unsigned int getCompressionRatio(void) {
    return compressionRatio;
}
