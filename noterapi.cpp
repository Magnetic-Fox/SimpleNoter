#include "noterapi.hpp"

bool noter_credentialsAvailable(NOTER_CREDENTIALS &credentials)
{
    return ((credentials.username.length()>0) && (credentials.password.length()>0));
}

bool noter_connectionSettingsAvailable(NOTER_CONNECTION_SETTINGS &connectionSettings)
{
    return ((connectionSettings.ipAddress.length()>0) && (connectionSettings.port>0) && (connectionSettings.share.length()>0));
}

std::string noter_getAnswerString(long int answerCode)
{
    switch(answerCode)
    {
        case ERROR_WRONG_RESPONSE:
            return "Nie uda³o siê po³¹czyæ z serwerem.";
        case ERROR_SERVICE_DISABLED:
            return "Us³uga tymczasowo niedostêpna.";
        case ERROR_INTERNAL_SERVER_ERROR:
            return "B³¹d wewnêtrzny serwera.";
        case ERROR_NOTE_ALREADY_UNLOCKED:
            return "Notatka ju¿ odblokowana.";
        case ERROR_NOTE_ALREADY_LOCKED:
            return "Notatka ju¿ zablokowana.";
        case ERROR_NOTE_LOCKED:
            return "Notatka jest zablokowana.";
        case ERROR_USER_REMOVAL_FAILURE:
            return "Usuniêcie u¿ytkownika nie powiod³o siê.";
        case ERROR_USER_NOT_EXISTS:
            return "Dany u¿ytkownik nie istnieje.";
        case ERROR_NOTE_NOT_EXISTS:
            return "Dana notatka nie istnieje.";
        case ERROR_NO_NECESSARY_INFORMATION:
            return "Brak wymaganych informacji (temat lub treœæ).";
        case ERROR_USER_DEACTIVATED:
            return "U¿ytkownik zablokowany.";
        case ERROR_LOGIN_INCORRECT:
            return "Nieprawid³owe dane logowania.";
        case ERROR_UNKNOWN_ACTION:
            return "Nieprawid³owe polecenie.";
        case ERROR_NO_CREDENTIALS:
            return "Brak danych logowania.";
        case ERROR_USER_EXISTS:
            return "Podana nazwa u¿ytkownika jest ju¿ zajêta.";
        case ERROR_NO_USABLE_INFORMATION:
            return "Brak u¿ytecznych informacji w ¿¹daniu.";
        case ERROR_INVALID_METHOD:
            return "Nieobs³ugiwane ¿¹danie.";
        case INFO_OK:
            return "Wszystko w porz¹dku.";
        case INFO_USER_CREATED:
            return "U¿ytkownik zosta³ zarejestrowany.";
        case INFO_USER_UPDATED:
            return "Dane u¿ytkownika zosta³y zaktualizowane.";
        case INFO_USER_REMOVED:
            return "U¿ytkownik zosta³ usuniêty.";
        case INFO_LIST_SUCCESSFUL:
            return "Lista notatek zosta³a za³adowana.";
        case INFO_NOTE_RETRIEVED:
            return "Notatka zosta³a za³adowana.";
        case INFO_NOTE_CREATED:
            return "Notatka zosta³a utworzona.";
        case INFO_NOTE_UPDATED:
            return "Notatka zosta³a zaktualizowana.";
        case INFO_NOTE_DELETED:
            return "Notatka zosta³a usuniêta.";
        case INFO_USER_INFO_RETRIEVED:
            return "Informacje o u¿ytkowniku zosta³y za³adowane.";
        case INFO_NOTE_LOCKED:
            return "Notatka zosta³a zablokowana.";
        case INFO_NOTE_UNLOCKED:
            return "Notatka zosta³a odblokowana.";
        default:
            return "Nieznany kod odpowiedzi.";
    }
}

NOTER_CREDENTIALS noter_prepareCredentials(char* username, char* password)
{
    NOTER_CREDENTIALS credentials;
    credentials.username=username;
    credentials.password=password;
    return credentials;
}

NOTER_CONNECTION_SETTINGS noter_prepareConnectionSettings(char* ipAddress, unsigned int port, char* share, char* userAgent)
{
    NOTER_CONNECTION_SETTINGS connectionSettings;
    connectionSettings.ipAddress=ipAddress;
    connectionSettings.port=port;
    connectionSettings.share=share;
    connectionSettings.userAgent=userAgent;
    return connectionSettings;
}

bool noter_checkAndPrepareResponse(HEADERS &heads, char *&buffer, unsigned int &bufDataSize, json_value *&jsonData, NAMEDESCRIPTOR &desc)
{
    if(noter_correctResponse(heads))
    {
        jsonData=json_parse(buffer,bufDataSize);
        if(jsonData==NULL)
        {
            return false;            
        }
        else
        {
            return indexMainResponse(jsonData, desc);
        }
    }
    else
    {
        return false;
    }
}

NOTER_SERVER_INFO noter_getServerInfo(NOTER_CONNECTION_SETTINGS &connectionSettings, char *buffer)
{
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    NOTER_SERVER_INFO serverInfo;

    recSize=noter_simplyMakeRequest("GET",
                                    connectionSettings,
                                    NULL,
                                    "",
                                    "",
                                    "",
                                    0,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc))
    {
        if(noter_getAnswerCode(desc)==INFO_OK)
        {
            serverInfo.name=    getSingleString(desc["server_name"]);
            serverInfo.timezone=getSingleString(desc["server_timezone"]);
            serverInfo.version= getSingleString(desc["server_version"]);
        }
        json_value_free(data);
    }
    
    return serverInfo;
}

long int noter_getNoteList(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, char* buffer, NOTE_SUMMARY *&notes)
{
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    NUMBERDESCRIPTOR desc2;
    json_value *data;
    long int noteCount=0;

    recSize=noter_simplyMakeRequest("POST",
                                    connectionSettings,
                                    &credentials,
                                    "list",
                                    "",
                                    "",
                                    0,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc))
    {
        noteCount=noter_getAnswerCode(desc);
        if(noteCount==INFO_LIST_SUCCESSFUL)
        {
            noteCount=getNoteList(desc,desc2,notes);
        }
        json_value_free(data);
    }
    else
    {
        noteCount=ERROR_WRONG_RESPONSE;
    }

    return noteCount;
}

long int noter_getNote(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, unsigned long int noteID, char* buffer, NOTE &note)
{
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    NUMBERDESCRIPTOR desc2;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest("POST",
                                    connectionSettings,
                                    &credentials,
                                    "retrieve",
                                    "",
                                    "",
                                    noteID,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc))
    {
        answerCode=noter_getAnswerCode(desc);
        if(answerCode==INFO_NOTE_RETRIEVED)
        {
            note=getNote(desc);
        }
        json_value_free(data);
    }
    else
    {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_getUserInfo(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, char* buffer, USER_INFO &userInfo)
{
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest("POST",
                                    connectionSettings,
                                    &credentials,
                                    "info",
                                    "",
                                    "",
                                    0,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc))
    {
        answerCode=noter_getAnswerCode(desc);
        if(answerCode==INFO_USER_INFO_RETRIEVED)
        {
            userInfo=getUserInfo(desc);
        }
        json_value_free(data);
    }
    else
    {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_lockNote(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, unsigned long int noteID, char* buffer)
{
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest("POST",
                                    connectionSettings,
                                    &credentials,
                                    "lock",
                                    "",
                                    "",
                                    noteID,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc))
    {
        answerCode=noter_getAnswerCode(desc);
        json_value_free(data);
    }
    else
    {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_unlockNote(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, unsigned long int noteID, char* buffer)
{
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest("POST",
                                    connectionSettings,
                                    &credentials,
                                    "unlock",
                                    "",
                                    "",
                                    noteID,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc))
    {
        answerCode=noter_getAnswerCode(desc);
        json_value_free(data);
    }
    else
    {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_deleteNote(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, unsigned long int noteID, char* buffer)
{
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest("POST",
                                    connectionSettings,
                                    &credentials,
                                    "delete",
                                    "",
                                    "",
                                    noteID,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc))
    {
        answerCode=noter_getAnswerCode(desc);
        json_value_free(data);
    }
    else
    {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_addNote(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, NOTE &note, char* buffer)
{
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;
    long int newID=0;

    recSize=noter_simplyMakeRequest("POST",
                                    connectionSettings,
                                    &credentials,
                                    "add",
                                    (char*)note.subject.c_str(),
                                    (char*)note.entry.c_str(),
                                    0,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc))
    {
        answerCode=noter_getAnswerCode(desc);
        if(answerCode==INFO_NOTE_CREATED)
        {
            newID=getNewID(desc);
            if(newID>0)
            {
                note.id=newID;
            }
            else
            {
                note.id=0;
                answerCode=ERROR_WRONG_RESPONSE;
            }
        }
        json_value_free(data);
    }
    else
    {
        answerCode=ERROR_WRONG_RESPONSE;
    }
    return answerCode;
}

long int noter_updateNote(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, NOTE &note, char* buffer)
{
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest("POST",
                                    connectionSettings,
                                    &credentials,
                                    "update",
                                    (char*)note.subject.c_str(),
                                    (char*)note.entry.c_str(),
                                    note.id,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc))
    {
        answerCode=noter_getAnswerCode(desc);
        json_value_free(data);
    }
    else
    {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_registerUser(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, char* buffer)
{
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest("POST",
                                    connectionSettings,
                                    &credentials,
                                    "register",
                                    "",
                                    "",
                                    0,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc))
    {
        answerCode=noter_getAnswerCode(desc);
        json_value_free(data);
    }
    else
    {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_changeUserPassword(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, char* newPassword, char* buffer)
{
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest("POST",
                                    connectionSettings,
                                    &credentials,
                                    "change",
                                    "",
                                    "",
                                    0,
                                    newPassword,
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc))
    {
        answerCode=noter_getAnswerCode(desc);
        json_value_free(data);
    }
    else
    {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}

long int noter_removeUser(NOTER_CONNECTION_SETTINGS &connectionSettings, NOTER_CREDENTIALS &credentials, char* buffer)
{
    unsigned int recSize;
    HEADERS heads;
    NAMEDESCRIPTOR desc;
    json_value *data;
    long int answerCode=0;

    recSize=noter_simplyMakeRequest("POST",
                                    connectionSettings,
                                    &credentials,
                                    "remove",
                                    "",
                                    "",
                                    0,
                                    "",
                                    heads,
                                    buffer);

    if(noter_checkAndPrepareResponse(heads, buffer, recSize, data, desc))
    {
        answerCode=noter_getAnswerCode(desc);
        json_value_free(data);
    }
    else
    {
        answerCode=ERROR_WRONG_RESPONSE;
    }

    return answerCode;
}
