#include "conversion.hpp"

std::string IntToStr(long int input)
{
    std::string temp;
    char test[32];
    ltoa(input,test,10);
    temp=test;
    return temp;
}
