#include "simplecrypto.hpp"

std::string secureString(bool encode, char* input, char* key)
{
    std::string tempString="";
    unsigned int keyLength=strlen(key);
    unsigned int subtract=0;
    char temp=0;
    bool switchChar=false;
    if(encode)
    {
        for(unsigned int x=0; x<strlen(input); ++x)
        {
            temp=input[x] ^ key[x % keyLength];
            if(temp==0)
            {
                tempString=tempString+(char)255;
                tempString=tempString+(char)1;
            }
            else if(temp==255)
            {
                tempString=tempString+(char)255;
                tempString=tempString+(char)2;
            }
            else
            {
                tempString=tempString+temp;
            }
        }
    }
    else
    {
        for(unsigned int x=0; x<strlen(input); ++x)
        {
            if(input[x]==255)
            {
                switchChar=true;
                ++subtract;
            }
            else
            {
                if(switchChar)
                {
                    if(input[x]==1)
                    {
                        temp=(char)0 ^ key[(x-subtract) % keyLength];
                    }
                    else if(input[x]==2)
                    {
                        temp=(char)255 ^ key[(x-subtract) % keyLength];
                    }
                    else
                    {
                        temp='?';
                    }
                    tempString=tempString+temp;
                }
                else
                {
                    temp=input[x] ^ key[(x-subtract) % keyLength];
                    tempString=tempString+temp;
                }
                switchChar=false;
            }
        }
    }
    return tempString;
}

std::string makeSecureString_B64(bool encode, char* input, char* key)
{
    if(encode)
    {
        return base64_encode((char*)secureString(encode,input,key).c_str());
    }
    else
    {
        return secureString(encode,(char*)base64_decode(input).c_str(),key);
    }
}
