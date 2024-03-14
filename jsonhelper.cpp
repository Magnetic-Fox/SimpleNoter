#include "jsonhelper.hpp"

void indexNames(json_value *value, std::string prefix, NAMEDESCRIPTOR &descriptor) {
    std::string tempName;
    for(unsigned int x=0; x<value->u.object.length; ++x) {
        tempName=value->u.object.values[x].name;
        tempName=prefix+tempName;
        descriptor[tempName]=value->u.object.values[x].value;
    }
    return;
}

void indexNumbers(json_value *value, std::string prefix, NUMBERDESCRIPTOR &descriptor) {
    std::string tempName;
    for(unsigned int x=0; x<value->u.object.length; ++x) {
        tempName=value->u.object.values[x].name;
        tempName=prefix+tempName;
        descriptor[tempName]=x;
    }
    return;
}

char* getArrayString(json_value *value, unsigned long int index) {
    return value->u.array.values[index]->u.string.ptr;
}

json_value* getArrayElement(json_value *value, unsigned long int index) {
    return value->u.array.values[index];
}

long int getInteger(json_value *value, unsigned long int objectPos) {
    return value->u.object.values[objectPos].value->u.integer;
}

char* getString(json_value *value, unsigned long int objectPos) {
    return value->u.object.values[objectPos].value->u.string.ptr;
}

long int getSingleInteger(json_value *value) {
    return value->u.integer;
}

char* getSingleString(json_value *value) {
    return value->u.string.ptr;
}

bool jsonLooksValid(char *&buffer, unsigned int &bufDataSize) {
    return (((buffer[0]=='[') || (buffer[0]=='{')) && ((buffer[bufDataSize-1]==']') || (buffer[bufDataSize-1]=='}')));
}
