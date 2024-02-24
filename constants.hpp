#ifndef CONSTANTS_H
#define CONSTANTS_H

#define BST_UNCHECKED                   0x0000
#define BST_CHECKED                     0x0001

#define MATCH_VERSION                   "1.0"
#define USER_AGENT                      "SimpleNoter/0.52"
#define INI_FILE                        "config.ini"
#define APPNAME                         "Simple Noter v0.52"
#define HELPFILE                        "SNOTER05.HLP"

// --------------------------------------------------------------
// STRING TABLES
// --------------------------------------------------------------

// Polish string table

#define STRING_NEW_NOTE                 "~ Nowa notatka ~ - "
#define STRING_WRONG_RESPONSE           "Nie uda�o si� po��czy� z serwerem."
#define STRING_SERVICE_DISABLED         "Us�uga tymczasowo niedost�pna."
#define STRING_INTERNAL_SERVER_ERROR    "B��d wewn�trzny serwera."
#define STRING_NOTE_ALREADY_UNLOCKED    "Notatka ju� odblokowana."
#define STRING_NOTE_ALREADY_LOCKED      "Notatka ju� zablokowana."
#define STRING_NOTE_LOCKED              "Notatka jest zablokowana."
#define STRING_USER_REMOVAL_FAILURE     "Usuni�cie u�ytkownika nie powiod�o si�."
#define STRING_USER_NOT_EXISTS          "Dany u�ytkownik nie istnieje."
#define STRING_NOTE_NOT_EXISTS          "Dana notatka nie istnieje."
#define STRING_NO_NECESSARY_INFORMATION "Brak wymaganych informacji (temat lub tre��)."
#define STRING_USER_DEACTIVATED         "U�ytkownik zablokowany."
#define STRING_LOGIN_INCORRECT          "Nieprawid�owe dane logowania."
#define STRING_UNKNOWN_ACTION           "Nieprawid�owe polecenie."
#define STRING_NO_CREDENTIALS           "Brak danych logowania."
#define STRING_USER_EXISTS              "Podana nazwa u�ytkownika jest ju� zaj�ta."
#define STRING_NO_USABLE_INFORMATION    "Brak u�ytecznych informacji w ��daniu."
#define STRING_INVALID_METHOD           "Nieobs�ugiwane ��danie."
#define STRING_INFO_OK                  "Wszystko w porz�dku."
#define STRING_INFO_USER_CREATED        "U�ytkownik zosta� zarejestrowany."
#define STRING_INFO_USER_UPDATED        "Dane u�ytkownika zosta�y zaktualizowane."
#define STRING_INFO_USER_REMOVED        "U�ytkownik zosta� usuni�ty."
#define STRING_INFO_LIST_SUCCESSFUL     "Lista notatek zosta�a za�adowana."
#define STRING_INFO_NOTE_RETRIEVED      "Notatka zosta�a za�adowana."
#define STRING_INFO_NOTE_CREATED        "Notatka zosta�a utworzona."
#define STRING_INFO_NOTE_UPDATED        "Notatka zosta�a zaktualizowana."
#define STRING_INFO_NOTE_DELETED        "Notatka zosta�a usuni�ta."
#define STRING_INFO_USER_INFO_RETRIEVED "Informacje o u�ytkowniku zosta�y za�adowane."
#define STRING_INFO_NOTE_LOCKED         "Notatka zosta�a zablokowana."
#define STRING_INFO_NOTE_UNLOCKED       "Notatka zosta�a odblokowana."
#define STRING_UNKNOWN_ERROR            "Nieznany kod odpowiedzi."

#define STRING_ADD                      "Dodaj"
#define STRING_UPDATE                   "Aktualizuj"
#define STRING_PROPERTIES               "W�a�ciwo�ci"
#define STRING_CLOSE                    "Zamknij"
#define STRING_INFORMATION              "Informacja"
#define STRING_WARNING                  "Ostrze�enie"
#define STRING_ERROR                    "B��d"
#define STRING_DOWNLOAD                 "Pobierz"
#define STRING_CREATE                   "Utw�rz"
#define STRING_OPEN                     "Otw�rz"
#define STRING_DELETE                   "Usu�"
#define STRING_EXIT                     "Zako�cz"

#define STRING_NORMAL_WINDOW            "Normalne okno"
#define STRING_MINIMIZED_WINDOW         "Zminimalizowane"
#define STRING_MAXIMIZED_WINDOW         "Zmaksymalizowane"

#define STRING_ID                       "ID:"
#define STRING_LAST_CHANGED             "Ostatnie zmiany:"
#define STRING_NOT_CHOSEN               "-- nie wybrano --"
#define STRING_NOT_CONNECTED            "-- nie po��czono --"
#define STRING_NOT_LOGGED_IN            "-- nie zalogowano --"
#define STRING_SPACED_COUNT             " Ilo��: "
#define STRING_SPACED_LAST_MOD_DATE     " Data ostatniej modyfikacji: "

#define STRING_LOADING_NOTE_LIST        "Pobieranie listy notatek..."
#define STRING_CREATING_EDIT_WINDOW     "Tworzenie okna edycji..."
#define STRING_DOWNLOADING_NOTE         "Pobieranie notatki..."
#define STRING_TO_NEW_NOTE              "Przekierowano na now� notatk�."

#define STRING_MENU_ADD                 "Dodaj\tCtrl+S"
#define STRING_MENU_UPDATE              "Aktualizuj\tCtrl+S"

#define STRING_EDITWIN_TITLE            "Tytu�:"
#define STRING_EDITWIN_ENTRY            "Tre��:"
#define STRING_EDITWIN_CREATE_ERROR     "Nie uda�o si� utworzy� okna edycji."

#define STRING_MSG_WANT_CHANGES_SAVED   "Czy chcesz zapisa� zmiany?"
#define STRING_MSG_CTL3D_ERROR          "Zarejestrowanie CTL3D nie powiod�o si�!"
#define STRING_MSG_UNSUPPORTED_CHARS    "Wybrana notatka zawiera nieobs�ugiwane znaki UTF-8.\nDokonywanie zmian t� wersj� programu spowoduje nieodwracaln� utrat� tych informacji."
#define STRING_MSG_WINSOCK_ERROR        "Inicjalizacja sk�adnika WinSock nie powiod�a si�. Program zostanie zamkni�ty."
#define STRING_MSG_WNDCLASS_ERROR       "Utworzenie klasy okna nie powiod�o si�. Program zostanie zamkni�ty."
#define STRING_MSG_WND_CREATE_ERROR     "Utworzenie okna nie powiod�o si�. Program zostanie zamkni�ty."
#define STRING_MSG_ACCELERATORS_ERROR   "Nie uda�o si� za�adowa� akcelerator�w! Program zostanie zamkni�ty."
#define STRING_MSG_WANT_RELOAD          "Czy chcesz prze�adowa� list� notatek?"
#define STRING_MSG_WANT_NOTE_REMOVAL    "Czy na pewno chcesz usun�� notatk�"
#define STRING_MSG_CTL3D_UNREG_ERROR    "Wyrejestrowanie aplikacji z CTL3D nie powiod�o si�!"
#define STRING_MSG_CTL3D_CHANGE         "Aby zmiany ustawie� kontrolek 3D odnios�y skutek, wymagane jest ponowne uruchomienie programu."
#define STRING_MSG_WRONG_PORT_NUMBER    "Nieprawid�owy numer portu."
#define STRING_MSG_HOST_NOT_FOUND       "Nie uda�o si� odnale�� hosta."
#define STRING_MSG_CONN_ESTABLISHED     "Uda�o si� prawid�owo zestawi� po��czenie."
#define STRING_MSG_CONNECTION_ERROR     "Po��czenie nie powiod�o si�."
#define STRING_MSG_CREDENTIALS_CHANGED  "Dane logowania uleg�y zmianie na serwerze.\nCzy na pewno chcesz porzuci� zmiany?"
#define STRING_MSG_LOGIN_SUCCESSFUL     "Uda�o si� prawid�owo zalogowa�."
#define STRING_MSG_NO_CONN_SETTINGS     "Brak ustawie� po��czenia z serwerem."
#define STRING_MSG_ACC_DELETE_PART1     "Czy na pewno chcesz usun�� konto "
#define STRING_MSG_ACC_DELETE_PART2     "?\nKontynuacja usunie wszystkie notatki oraz informacje o u�ytkowniku przechowywane na serwerze.\nOperacja ta jest nieodwracalna!!!\n\nCzy na pewno chcesz usun�� swoje konto?"
#define STRING_MSG_REGISTRATION_SUCC    "Uda�o si� prawid�owo zarejestrowa� nowego u�ytkownika."
#define STRING_MSG_REGISTRATION_ERROR   "Nie uda�o si� zarejestrowa� nowego u�ytkownika.\n"
#define STRING_MSG_PASSWORDS_NO_MATCH   "Has�a nie s� takie same!"
#define STRING_MSG_USER_SPACED          "U�ytkownik "
#define STRING_MSG_USER_DELETED_SPACED  " zosta� usuni�ty."
#define STRING_MSG_WRONG_PASSWORD       "Nieprawid�owe has�o!"
#define STRING_MSG_PASSWORD_CHANGED     "Has�o zosta�o zmienione."

// End of Polish string table

#endif
