#ifndef SHM_CONFIG_H
#define SHM_CONFIG_H

#include <stdint.h>
#include <time.h>
#include <sys/types.h>

namespace Ipc
{
    enum DataLimit
    {
        DL_MSG_TOTAL   = 2 * 1024 * 1024,
        DL_NOTIF_TOTAL = 1024 * 1024,
        DL_RELAY_TOTAL = 512 * 1024,

        Shm_Size_Logical_Ext_Conn_Server = 2 * 1024 * 1024,
        Shm_Size_Logical_Internal_Server = 2 * 1024 * 1024,
        Shm_Size_Logical_Notif_Server = 1024 * 1024,
        Shm_Size_Logical_Relay_Server = 512 * 1024
    };
}

namespace Cache
{
    enum DataLimit
    {
        DL_DATA_BLOCK_SIZE = 8 * 1024
    };
    enum CountLimit
    {
        CL_MAX_OBJECT_COUNT = 10000
    };
    enum HashLimit
    {
        HL_MAX_HASH_RANGE = 19997
    };

    enum DevKeyUpdateStatus
    {
        KUS_Updated    = 0x01,
        KUS_NeedUpdate = 0x02,
        KUS_Updating   = 0x03
    };

    //enum OnlineStatus
    //{
    //    OS_Online  = 0x01,
    //    OS_Offline = 0x02,
    //    OS_other   = 0x03
    //};
}

#endif
