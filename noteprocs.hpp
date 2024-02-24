#ifndef NOTEPROCS_H
#define NOTEPROCS_H

#include <map>
#include "json.h"
#include "jsonhelper.hpp"
#include "responses.hpp"

typedef struct NOTE {
    long int    id;
    std::string subject;
    std::string entry;
    std::string dateAdded;
    std::string lastModified;
    bool        locked;
    std::string userAgent;
    std::string lastUserAgent;
} NOTE;

typedef struct NOTE_SUMMARY {
    long int    id;
    std::string subject;
    std::string lastModified;
} NOTE_SUMMARY;

bool indexMainResponse(json_value*, NAMEDESCRIPTOR&);
bool indexNote(NAMEDESCRIPTOR&);
bool indexNoteList(NAMEDESCRIPTOR&, NUMBERDESCRIPTOR&);
bool indexNewID(NAMEDESCRIPTOR&);
long int getNoteList(NAMEDESCRIPTOR&, NUMBERDESCRIPTOR&, NOTE_SUMMARY*&);
NOTE getNote(NAMEDESCRIPTOR&);
long int getNewID(NAMEDESCRIPTOR&);
void freeNoteList(NOTE_SUMMARY*&);

#endif
