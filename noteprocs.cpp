#include "noteprocs.hpp"

bool indexMainResponse(json_value* value, NAMEDESCRIPTOR &descriptor)
{
    descriptor.clear();
    indexNames(value,"",descriptor);
    if((descriptor["server"]==NULL) || (descriptor["answer_info"]==NULL))
    {
        return false;
    }
    indexNames(descriptor["server"],"server_",descriptor);
    indexNames(descriptor["answer_info"],"answer_info_",descriptor);
    if((descriptor["server_name"]==NULL) || (descriptor["server_timezone"]==NULL) || (descriptor["server_version"]==NULL)
        || (descriptor["answer_info_code"]==NULL) || (descriptor["answer_info_attachment"]==NULL))
    {
        return false;
    }
    return true;
}

bool indexNote(NAMEDESCRIPTOR &descriptor)
{
    if(descriptor["answer"]==NULL)
    {
        return false;
    }
    indexNames(descriptor["answer"],"answer_",descriptor);
    if(descriptor["answer_note"]==NULL)
    {
        return false;
    }
    indexNames(descriptor["answer_note"],"answer_note_",descriptor);
    if((descriptor["answer_note_id"]==NULL) || (descriptor["answer_note_subject"]==NULL) || (descriptor["answer_note_entry"]==NULL)
        || (descriptor["answer_note_date_added"]==NULL) || (descriptor["answer_note_last_modified"]==NULL)
        || (descriptor["answer_note_locked"]==NULL) || (descriptor["answer_note_user_agent"]==NULL)
        || (descriptor["answer_note_last_user_agent"]==NULL))
    {
        return false;
    }
    return true;
}

bool indexNoteList(NAMEDESCRIPTOR &descriptor, NUMBERDESCRIPTOR &descriptor2)
{
    if(descriptor["answer"]==NULL)
    {
        return false;
    }
    descriptor2.clear();
    indexNames(descriptor["answer"],"answer_",descriptor);
    if((descriptor["answer_count"]==NULL) || (descriptor["answer_notes_summary"]==NULL))
    {
        return false;
    }
    if(getSingleInteger(descriptor["answer_count"])>0)
    {
        indexNumbers(getArrayElement(descriptor["answer_notes_summary"],0),"",descriptor2);
    }
    return true;
}

bool indexNewID(NAMEDESCRIPTOR &descriptor)
{
    if(descriptor["answer"]==NULL)
    {
        return false;
    }
    indexNames(descriptor["answer"],"answer_",descriptor);
    return descriptor["answer_new_id"]!=NULL;
}

long int getNoteList(NAMEDESCRIPTOR &descriptor, NUMBERDESCRIPTOR &descriptor2, NOTE_SUMMARY *&notes)
{
    long int notesCount;
    if(indexNoteList(descriptor,descriptor2))
    {
        notesCount=getSingleInteger(descriptor["answer_count"]);
        if(notesCount>0)
        {
            notes = new NOTE_SUMMARY[notesCount];

            for(long int x=0; x<notesCount; ++x)
            {
                notes[x].id=            getInteger(getArrayElement(descriptor["answer_notes_summary"],x),descriptor2["id"]);
                notes[x].subject=       getString(getArrayElement(descriptor["answer_notes_summary"],x),descriptor2["subject"]);
                notes[x].lastModified=  getString(getArrayElement(descriptor["answer_notes_summary"],x),descriptor2["last_modified"]);
            }
        }
        else
        {
            notes=NULL;
        }
        return notesCount;
    }
    else
    {
        return ERROR_WRONG_RESPONSE;
    }
}

NOTE getNote(NAMEDESCRIPTOR &descriptor)
{
    NOTE note;
    if(indexNote(descriptor))
    {
        note.id=            getSingleInteger(descriptor["answer_note_id"]);
        note.subject=       getSingleString(descriptor["answer_note_subject"]);
        note.entry=         getSingleString(descriptor["answer_note_entry"]);
        note.dateAdded=     getSingleString(descriptor["answer_note_date_added"]);
        note.lastModified=  getSingleString(descriptor["answer_note_last_modified"]);
        note.locked=        (bool)getSingleInteger(descriptor["answer_note_locked"]);
        note.userAgent=     getSingleString(descriptor["answer_note_user_agent"]);
        note.lastUserAgent= getSingleString(descriptor["answer_note_last_user_agent"]);
    }
    return note;
}

long int getNewID(NAMEDESCRIPTOR &descriptor)
{
    if(indexNewID(descriptor))
    {
        return getSingleInteger(descriptor["answer_new_id"]);
    }
    else
    {
        return ERROR_WRONG_RESPONSE;
    }
}

void freeNoteList(NOTE_SUMMARY *&notes)
{
    delete[] notes;
    return;
}
