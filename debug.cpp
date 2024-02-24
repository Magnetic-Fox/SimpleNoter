#include "debug.hpp"

void ShowInteger(long int integer) {
    char test[20];
    ltoa(integer,test,10);
    MessageBox(0,test,"ShowInteger",MB_OK);
    return;
}
