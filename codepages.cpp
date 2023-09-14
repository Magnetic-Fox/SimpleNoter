#include "codepages.hpp"

static long int  the_index = 0;
static long int  the_length = 0;
static long int  the_char = 0;
static long int  the_byte = 0;
static char* the_input;

std::string fromCodePage(RAWCODEPAGE nmCodePage, char* input)
{
    std::string temp="";
    for(unsigned long int x=0; x<strlen(input); ++x)
    {
        if(input[x]<0x80)
        {
            temp=temp+input[x];
        }
        else
        {
            char xtest[8];
            int xout=utf8_encode(xtest,nmCodePage[input[x]-0x80]);
            if(xout==0)
            {
                temp=temp+'.';
            }
            else
            {
                for(unsigned int y=0; y<xout; ++y)
                {
                    temp=temp+xtest[y];
                }
            }
        }
    }
    return temp;
}

std::string toCodePage(CODEPAGE &codepage, char* input)
{
    std::string temp="";
    utf8_decode_init(input,strlen(input));
    while(true)
    {
        long int one=utf8_decode_next();
        if(one<0)
        {
            if(one==UTF8_ERROR)
            {
                temp=temp+'~';
            }
            break;
        }
        else
        {
            if(one<0x80)
            {
                temp=temp+(char)one;
            }
            else
            {
                temp=temp+(char)codepage[one];
            }
        }
    }
    return temp;
}

void prepareCodePage(CODEPAGE &codepage, RAWCODEPAGE cpdef)
{
    for(unsigned int x=0; x<128; ++x)
    {
        codepage[cpdef[x]]=0x80+x;
    }
    return;
}
