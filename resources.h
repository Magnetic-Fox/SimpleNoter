#ifndef RESOURCES_H
#define RESOURCES_H

// Windows include
#include <windows.h>

// Icons
#define IDI_MAINICON                        100
#define IDI_EDITICON                        101

// Accelerators
#define IDR_ACCELERATORS                    200
#define ID_ACC_TAB                          201
#define ID_ACC_ALTF4                        202
#define ID_ACC_F1                           203
#define ID_ACC_CTRLN                        204
#define ID_ACC_ENTER                        205
#define ID_ACC_DEL                          206
#define ID_ACC_F5                           207
#define ID_ACC_CTRLA                        208
#define ID_ACC_CTRLS                        209
#define ID_ACC_CTRLD                        210
#define ID_ACC_CTRLI                        211
#define ID_ACC_CTRLE                        212

// Main window menu
#define IDR_MENU_MAIN                       300
#define ID_FILE_NEW                         301
#define ID_FILE_OPEN                        302
#define ID_FILE_RELOAD                      303
#define ID_FILE_DELETE                      304
#define ID_FILE_IMPORT                      305
#define ID_FILE_EXPORT                      306
#define ID_FILE_EXIT                        307
#define ID_EDIT_SELECTALL                   308
#define ID_OPTIONS_PREFERENCES              309
#define ID_OPTIONS_CONNECTION               310
#define ID_OPTIONS_CREDENTIALS              311
#define ID_HELP_HELP                        312
#define ID_HELP_HOWTO                       313
#define ID_HELP_ABOUT                       314

// Edit window menu
#define IDR_MENU_EDIT                       400
#define ID_FILE_ADDUP                       401
#define ID_FILE_PROPERTIES                  402
#define ID_FILE_TONEWNOTE                   403
#define ID_EDIT_UNDO                        404
#define ID_EDIT_CUT                         405
#define ID_EDIT_COPY                        406
#define ID_EDIT_PASTE                       407
#define ID_EDIT_CLEAR                       408
#define ID_EDIT_SELECTALL_2                 409

// Note information dialog
#define IDD_NOTEINFO                        500
#define IDC_LOCKBUTTON                      501
#define IDC_UNLOCKBUTTON                    502
#define IDC_STATIC                          503
#define IDC_STATIC1                         504
#define IDC_STATIC2                         505
#define IDC_STATIC3                         506
#define IDC_STATIC4                         507
#define IDC_STATIC5                         508
#define IDC_STATIC6                         509
#define IDC_STATIC7                         510

// Program information dialog
#define IDD_APPINFO                         600

// Preferences dialog
#define IDD_PREFERENCES                     700
#define IDC_RADIO1                          701
#define IDC_RADIO2                          702
#define IDC_COMBO1                          703
#define IDC_RADIO3                          704
#define IDC_RADIO4                          705
#define IDC_COMBO2                          706
#define IDC_AUTORELOADCHECK                 707
#define IDC_USE3DCONTROLSCHECK              708
#define IDC_BUTTONSCHECK                    709
#define IDC_LISTSCHECK                      710
#define IDC_EDITSCHECK                      711
#define IDC_COMBOSCHECK                     712
#define IDC_DIALOGSCHECK                    713
#define IDC_REFRESHONADDCHECK               714
#define IDC_SAVEWINPOSCHECK                 715
#define IDC_COMBO3                          716
#define IDC_COMBO4                          717
#define IDC_CHECK20                         718
#define IDC_STATIC18                        719

// Connection dialog
#define IDD_CONNECTION                      800
#define IDC_COMPRESSIONCHECK                801
#define IDC_EDIT1                           802
#define IDC_EDIT2                           803
#define IDC_EDIT3                           804
#define IDC_TESTBUTTON                      805
#define IDC_STATIC8                         806
#define IDC_STATIC9                         807
#define IDC_STATIC10                        808

// Credentials dialog
#define IDD_CREDENTIALS                     900
#define IDC_EDIT4                           901
#define IDC_REGISTERBUTTON                  902
#define IDC_EDIT5                           903
#define IDC_TESTBUTTON2                     904
#define IDC_STATIC11                        905
#define IDC_STATIC12                        906
#define IDC_STATIC13                        907
#define IDC_STATIC14                        908
#define IDC_STATIC15                        909
#define IDC_ACCDELETEBUTTON                 910
#define IDC_PASSCHANGEBUTTON                911

// User registration dialog
#define IDD_REGISTRATION                    1000
#define IDC_EDIT6                           1001
#define IDC_EDIT7                           1002
#define IDC_EDIT8                           1003

// User account delete dialog
#define IDD_ACCDELETE                       1100
#define IDC_EDIT12                          1101

// User password change dialog
#define IDD_PASSCHANGE                      1200
#define IDC_EDIT9                           1201
#define IDC_EDIT10                          1202
#define IDC_EDIT11                          1203

// Note import dialog
// place left for IDD_IMPORT

// Note export dialog
#define IDD_EXPORT                          1400
#define IDC_EXPORT                          1401
#define IDC_BROWSEBUTTON                    1402
#define IDC_IGNOREFILENAMECHECK             1403
#define IDC_RADIO5                          1404
#define IDC_RADIO6                          1405
#define IDC_RADIO7                          1406
#define IDC_RADIO8                          1407
#define IDC_EDIT13                          1408
#define IDC_EDIT14                          1409
#define IDC_ADDPREFIXCHECK                  1410
#define IDC_EDIT15                          1411
#define IDC_FIRSTLINESUBJECTCHECK           1412
#define IDC_ONELINEGAPCHECK                 1413
#define IDC_ADDINFOATENDCHECK               1414
#define IDC_SEPARATEINFOCHECK               1415
#define IDC_SEPARATENOTESINFILESCHECK       1416
#define IDC_CONTINUEONERRORSCHECK           1417
#define IDC_STATIC16                        1418
#define IDC_STATIC17                        1419

// String table constants
#define IDS_HELPFILE                        3000
#define IDS_APPNAME                         3001
#define IDS_USER_AGENT                      3002
#define IDS_STRING_BUILTIN_LANGUAGE         3003
#define IDS_STRING_NEW_NOTE                 3004
#define IDS_STRING_WRONG_RESPONSE           3005
#define IDS_STRING_SERVICE_DISABLED         3006
#define IDS_STRING_INTERNAL_SERVER_ERROR    3007
#define IDS_STRING_NOTE_ALREADY_UNLOCKED    3008
#define IDS_STRING_NOTE_ALREADY_LOCKED      3009
#define IDS_STRING_NOTE_LOCKED              3010
#define IDS_STRING_USER_REMOVAL_FAILURE     3011
#define IDS_STRING_USER_NOT_EXISTS          3012
#define IDS_STRING_NOTE_NOT_EXISTS          3013
#define IDS_STRING_NO_NECESSARY_INFORMATION 3014
#define IDS_STRING_USER_DEACTIVATED         3015
#define IDS_STRING_LOGIN_INCORRECT          3016
#define IDS_STRING_UNKNOWN_ACTION           3017
#define IDS_STRING_NO_CREDENTIALS           3018
#define IDS_STRING_USER_EXISTS              3019
#define IDS_STRING_NO_USABLE_INFORMATION    3020
#define IDS_STRING_INVALID_METHOD           3021
#define IDS_STRING_INFO_OK                  3022
#define IDS_STRING_INFO_USER_CREATED        3023
#define IDS_STRING_INFO_USER_UPDATED        3024
#define IDS_STRING_INFO_USER_REMOVED        3025
#define IDS_STRING_INFO_LIST_SUCCESSFUL     3026
#define IDS_STRING_INFO_NOTE_RETRIEVED      3027
#define IDS_STRING_INFO_NOTE_CREATED        3028
#define IDS_STRING_INFO_NOTE_UPDATED        3029
#define IDS_STRING_INFO_NOTE_DELETED        3030
#define IDS_STRING_INFO_USER_INFO_RETRIEVED 3031
#define IDS_STRING_INFO_NOTE_LOCKED         3032
#define IDS_STRING_INFO_NOTE_UNLOCKED       3033
#define IDS_STRING_UNKNOWN_ERROR            3034
#define IDS_STRING_ADD                      3035
#define IDS_STRING_UPDATE                   3036
#define IDS_STRING_PROPERTIES               3037
#define IDS_STRING_CLOSE                    3038
#define IDS_STRING_INFORMATION              3039
#define IDS_STRING_WARNING                  3040
#define IDS_STRING_ERROR                    3041
#define IDS_STRING_DOWNLOAD                 3042
#define IDS_STRING_CREATE                   3043
#define IDS_STRING_OPEN                     3044
#define IDS_STRING_DELETE                   3045
#define IDS_STRING_EXIT                     3046
#define IDS_STRING_NORMAL_WINDOW            3047
#define IDS_STRING_MINIMIZED_WINDOW         3048
#define IDS_STRING_MAXIMIZED_WINDOW         3049
#define IDS_STRING_ID                       3050
#define IDS_STRING_LAST_CHANGED             3051
#define IDS_STRING_NOT_CHOSEN               3052
#define IDS_STRING_NOT_CONNECTED            3053
#define IDS_STRING_NOT_LOGGED_IN            3054
#define IDS_STRING_SPACED_COUNT             3055
#define IDS_STRING_SPACED_LAST_MOD_DATE     3056
#define IDS_STRING_SPACED_COMPRESSION       3057
#define IDS_STRING_LOADING_NOTE_LIST        3058
#define IDS_STRING_CREATING_EDIT_WINDOW     3059
#define IDS_STRING_DOWNLOADING_NOTE         3060
#define IDS_STRING_TO_NEW_NOTE              3061
#define IDS_STRING_MENU_ADD                 3062
#define IDS_STRING_MENU_UPDATE              3063
#define IDS_STRING_EDITWIN_TITLE            3064
#define IDS_STRING_EDITWIN_ENTRY            3065
#define IDS_STRING_EDITWIN_CREATE_ERROR     3066
#define IDS_STRING_MSG_WANT_CHANGES_SAVED   3067
#define IDS_STRING_MSG_CTL3D_ERROR          3068
#define IDS_STRING_MSG_UNSUPPORTED_CHARS    3069
#define IDS_STRING_MSG_WINSOCK_ERROR        3070
#define IDS_STRING_MSG_WNDCLASS_ERROR       3071
#define IDS_STRING_MSG_WND_CREATE_ERROR     3072
#define IDS_STRING_MSG_ACCELERATORS_ERROR   3073
#define IDS_STRING_MSG_WANT_RELOAD          3074
#define IDS_STRING_MSG_WANT_NOTE_REMOVAL    3075
#define IDS_STRING_MSG_CTL3D_UNREG_ERROR    3076
#define IDS_STRING_MSG_CTL3D_CHANGE         3077
#define IDS_STRING_MSG_WRONG_PORT_NUMBER    3078
#define IDS_STRING_MSG_HOST_NOT_FOUND       3079
#define IDS_STRING_MSG_CONN_ESTABLISHED     3080
#define IDS_STRING_MSG_CONNECTION_ERROR     3081
#define IDS_STRING_MSG_CREDENTIALS_CHANGED  3082
#define IDS_STRING_MSG_LOGIN_SUCCESSFUL     3083
#define IDS_STRING_MSG_NO_CONN_SETTINGS     3084
#define IDS_STRING_MSG_ACC_DELETE_PART1     3085
#define IDS_STRING_MSG_ACC_DELETE_PART2     3086
#define IDS_STRING_MSG_REGISTRATION_SUCC    3087
#define IDS_STRING_MSG_REGISTRATION_ERROR   3088
#define IDS_STRING_MSG_PASSWORDS_NO_MATCH   3089
#define IDS_STRING_MSG_USER_SPACED          3090
#define IDS_STRING_MSG_USER_DELETED_SPACED  3091
#define IDS_STRING_MSG_WRONG_PASSWORD       3092
#define IDS_STRING_MSG_PASSWORD_CHANGED     3093
#define IDS_STRING_MSG_CODEPAGE_ERROR       3094
#define IDS_STRING_MSG_CODEPAGE_ERROR_2     3095
#define IDS_STRING_BUILD_DATE               3096
#define IDS_STRING_MSG_LIKE_TO_OPEN_SPACED  3097
#define IDS_STRING_MSG_NOTES_SPACED         3098
#define IDS_STRING_NOT_ALL_NOTES_LOADED     3099
#define IDS_STRING_MSG_WANT_NOTES_REMOVAL   3100
#define IDS_STRING_NOT_ALL_NOTES_REMOVED    3101
#define IDS_STRING_MULTIPLE_CHOSEN          3102
#define IDS_STRING_REMOVING_NOTE            3103

#endif
