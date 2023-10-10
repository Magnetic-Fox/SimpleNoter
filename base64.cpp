/*************************************************************************
   Ultra simple Base64 implementation code

     20th February 2022 - 7th April 2022

   Use this code with respect to the author.
   (C)2022 Bart³omiej "Magnetic-Fox" Wêgrzyn!
**************************************************************************/

#include "base64.hpp"

// Unfortunately, it's better way to make it compile...
static const char al[] = {
         65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,
         81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  97,  98,  99, 100, 101, 102,
        103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118,
        119, 120, 121, 122,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  43,  47,
         61};

static const char al_d[] = {
        62,  0,  0,  0, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,
         0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
         0,  0,  0,  0,  0,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
        36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

void enc(char *in, char *out, int count)
{
        out[0]=0x40;
        out[1]=0x40;
        out[2]=0x40;
        out[3]=0x40;
        if(count>0)
        {
                out[0]=(((in[0]) >> 2) & 0x3F);
                out[1]=(((in[0]) & 0x03) << 4) & 0x30;
        }
        if(count>1)
        {
                out[1]|=(((in[1]) >> 4) & 0x0F);
                out[2]=(((in[1]) & 0x0F) << 2) & 0x3C;
        }
        if(count>2)
        {
                out[2]|=(((in[2]) >> 6) & 0x03);
                out[3]=((in[2]) & 0x3F);
        }
        out[0]=al[out[0]];
        out[1]=al[out[1]];
        out[2]=al[out[2]];
        out[3]=al[out[3]];
        return;
}

int dec(char *in, char *out)
{
        out[0]= (((al_d[in[0]-43]) & 0x3F) << 2);
        out[0]|=(((al_d[in[1]-43]) >> 4) & 0x03);
        out[1]= (((al_d[in[1]-43]) & 0x0F) << 4) & 0xF0;
        out[1]|=(((al_d[in[2]-43]) >> 2) & 0x0F);
        out[2]= ((((al_d[in[2]-43]) & 0x03) << 6) & 0xC0);
        out[2]|=(al_d[in[3]-43] & 0x3F);
        if((in[0]=='=') || (in[1]=='='))
        {
                return 0;
        }
        else if(in[2]=='=')
        {
                return 1;
        }
        else if(in[3]=='=')
        {
                return 2;
        }
        else
        {
                return 3;
        }
}

std::string base64_decode(char* input)
{
    std::string tempString="";
    unsigned long int x=0;
    char temp[4], temp2[4];
    if((input==NULL) || (strlen(input)==0))
    {
        return tempString;
    }
    while(input[x]!=0)
    {   
        if((x>0) && (x%4==0))
        {
            temp2[dec(temp,temp2)]=0;
            tempString=tempString+temp2;
        }
        temp[x%4]=input[x];
        ++x;
    }
    temp2[dec(temp,temp2)]=0;
    tempString=tempString+temp2;
    return tempString;
}

std::string base64_encode(char* input)
{
    std::string tempString="";
    unsigned long int x=0;
    char temp[3], temp2[5];
    if((input==NULL) || (strlen(input)==0))
    {
        return tempString;
    }
    while(input[x]!=0)
    {
        if((x>0) && (x%3==0))
        {
            enc(temp,temp2,3);
            temp2[4]=0;
            tempString=tempString+temp2;
        }
        temp[x%3]=input[x];
        ++x;
    }
    if(x%3>0)
    {
        enc(temp,temp2,x%3);
    }
    else
    {
        enc(temp,temp2,3);
    }
    temp2[4]=0;
    tempString=tempString+temp2;
    return tempString;
}
