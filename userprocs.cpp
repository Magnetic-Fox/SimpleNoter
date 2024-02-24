#include "userprocs.hpp"

bool indexUser(NAMEDESCRIPTOR &descriptor) {
    if(descriptor["answer"]==NULL) {
        return false;
    }
    indexNames(descriptor["answer"],"answer_",descriptor);
    if(descriptor["answer_user"]==NULL) {
        return false;
    }
    indexNames(descriptor["answer_user"],"answer_user_",descriptor);
    if((descriptor["answer_user_id"]==NULL) || (descriptor["answer_user_username"]==NULL)
        || (descriptor["answer_user_date_registered"]==NULL) || (descriptor["answer_user_user_agent"]==NULL)
        || (descriptor["answer_user_last_changed"]==NULL) || (descriptor["answer_user_last_user_agent"]==NULL))
    {
        return false;
    }
    return true;
}

USER_INFO getUserInfo(NAMEDESCRIPTOR &descriptor) {
    USER_INFO userInfo;
    if(indexUser(descriptor)) {
        userInfo.ID=            getSingleInteger(descriptor["answer_user_id"]);
        userInfo.username=      getSingleString(descriptor["answer_user_username"]);
        userInfo.dateRegistered=getSingleString(descriptor["answer_user_date_registered"]);
        userInfo.userAgent=     getSingleString(descriptor["answer_user_user_agent"]);
        userInfo.lastChanged=   getSingleString(descriptor["answer_user_last_changed"]);
        userInfo.lastUserAgent= getSingleString(descriptor["answer_user_last_user_agent"]);
    }
    return userInfo;
}
