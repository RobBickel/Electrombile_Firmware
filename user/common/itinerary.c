#include <eat_interface.h>
#include <eat_fs.h>

#include "itinerary.h"
#include "log.h"



int itinerary_store(MSG_ITINERARY_REQ* itinerary_msg)
{

    FS_HANDLE fh, rc;


    fh = eat_fs_Open(ITINERARYFILE_NAME,FS_CREATE);
    if (fh < EAT_FS_NO_ERROR)
    {
        LOG_ERROR("read initerary file fail, rc: %d", fh);
        return -1;
    }

    rc = eat_fs_Write(fh,itinerary_msg,sizeof(MSG_ITINERARY_REQ),NULL);
    if(EAT_FS_NO_ERROR > rc)
    {
        LOG_ERROR("write file failed, and Return Error is %d", rc);
        return -1;
    }
    eat_fs_Close(fh);
    return 0;

}

int itinerary_delete(void)
{

    int rc;

    rc = eat_fs_Delete(ITINERARYFILE_NAME);

    if(EAT_FS_NO_ERROR > rc)
    {
        LOG_ERROR("delete file failed, and Return Error is %d", rc);
        return -1;
    }

    return 0;
}


int itinerary_get(MSG_ITINERARY_REQ* itinerary_msg)
{
    FS_HANDLE fh, rc;


    fh = eat_fs_Open(ITINERARYFILE_NAME,FS_READ_ONLY);
    if(fh == EAT_FS_FILE_NOT_FOUND)
    {
        return -9;
    }

    if (fh < EAT_FS_NO_ERROR)
    {
        LOG_ERROR("read initerary file fail, rc: %d", fh);
        return -1;
    }

    rc = eat_fs_Read(fh,itinerary_msg,sizeof(MSG_ITINERARY_REQ),NULL);
    if(EAT_FS_NO_ERROR > rc)
    {
        LOG_ERROR("read file failed, and Return Error is %d", rc);
        return -1;
    }

    eat_fs_Close(fh);
    return 0;
}

