#ifndef RESPONSES_H
#define RESPONSES_H

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

#endif
