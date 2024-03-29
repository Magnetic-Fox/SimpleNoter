/*************************************************************************
   Ultra simple Base64 implementation header

     20th February 2022 - 7th April 2022
   & 20th September 2023
   & 17th March 2024

   Use this code with respect to the author.
   (C)2022-2024 Bart�omiej "Magnetic-Fox" W�grzyn!
**************************************************************************/

#ifndef BASE64_H
#define BASE64_H

#include <string>
#include <string.h>

const char al[] = {
         65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,
         81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  97,  98,  99, 100, 101, 102,
        103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118,
        119, 120, 121, 122,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  43,  47,
         61
};

const char al_d[] = {
        62,  0,  0,  0, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,
         0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
         0,  0,  0,  0,  0,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
        36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

void enc(char*, char*, int);
int dec(char*, char*);
std::string base64_decode(char*);
std::string base64_encode(char*);

#endif
