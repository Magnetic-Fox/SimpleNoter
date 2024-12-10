/*************************************************************************
   Ultra simple Base64 implementation code

     20th February 2022 - 7th April 2022
   & 20th September 2023
   & 17th March 2024

   Use this code with respect to the author.
   (C)2022-2024 Bart³omiej "Magnetic-Fox" Wêgrzyn!
**************************************************************************/

#include "base64.hpp"

void enc(char *in, char *out, int count) {
        out[0]=0x40;
        out[1]=0x40;
        out[2]=0x40;
        out[3]=0x40;
        if(count>0) {
                out[0]=(((in[0]) >> 2) & 0x3F);
                out[1]=(((in[0]) & 0x03) << 4) & 0x30;
        }
        if(count>1) {
                out[1]|=(((in[1]) >> 4) & 0x0F);
                out[2]=(((in[1]) & 0x0F) << 2) & 0x3C;
        }
        if(count>2) {
                out[2]|=(((in[2]) >> 6) & 0x03);
                out[3]=((in[2]) & 0x3F);
        }
        out[0]=al[out[0]];
        out[1]=al[out[1]];
        out[2]=al[out[2]];
        out[3]=al[out[3]];
        return;
}

int dec(char *in, char *out) {
        out[0]= (((al_d[in[0]-43]) & 0x3F) << 2);
        out[0]|=(((al_d[in[1]-43]) >> 4) & 0x03);
        out[1]= (((al_d[in[1]-43]) & 0x0F) << 4) & 0xF0;
        out[1]|=(((al_d[in[2]-43]) >> 2) & 0x0F);
        out[2]= ((((al_d[in[2]-43]) & 0x03) << 6) & 0xC0);
        out[2]|=(al_d[in[3]-43] & 0x3F);
        if((in[0]=='=') || (in[1]=='=')) {
                return 0;
        }
        else if(in[2]=='=') {
                return 1;
        }
        else if(in[3]=='=') {
                return 2;
        }
        else {
                return 3;
        }
}

std::string base64_decode(char* input) {
    std::string tempString="";
    unsigned long int x=0;
    char temp[4];
    char temp2[4];
    if(input==NULL) {
        return tempString;
    }
    if(strlen(input)==0) {
        return tempString;
    }
    while(input[x]!=0) {
        if((x>0) && (x%4==0)) {
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

std::string base64_encode(char* input) {
    std::string tempString="";
    unsigned long int x=0;
    char temp[3];
    char temp2[5];
    if(input==NULL) {
        return tempString;
    }
    if(strlen(input)==0) {
        return tempString;
    }
    while(input[x]!=0) {
        if((x>0) && (x%3==0)) {
            enc(temp,temp2,3);
            temp2[4]=0;
            tempString=tempString+temp2;
        }
        temp[x%3]=input[x];
        ++x;
    }
    if(x%3>0) {
        enc(temp,temp2,(int)(x%3));
    }
    else {
        enc(temp,temp2,3);
    }
    temp2[4]=0;
    tempString=tempString+temp2;
    return tempString;
}
