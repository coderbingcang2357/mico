#ifndef DB_AND_ADRESS_CONFIG_H__
#define DB_AND_ADRESS_CONFIG_H__

#include <arpa/inet.h>

// 外部连接服务器 ExtConnSrv 端口
//static const in_port_t ExtConn_Srv_Port = 8888;
//// 外部连接服务器 ExtConnSrv 地址
//#ifdef _DEBUG_VERSION
////static const char *const ExtConn_Srv_Address = "203.195.201.120";
//static const char *const ExtConn_Srv_Address = "127.0.0.1";
//#else
//static const char *const ExtConn_Srv_Address = "203.195.237.53";
//#endif
//
//// 数据库服务器 DataBaseeSrv 地址
//#ifdef _DEBUG_VERSION
//static const char* const DataBaseSrv_Address = "127.0.0.1";
//#else
////static const char* const DataBaseSrv_Address = "10.221.229.98";
//static const char* const DataBaseSrv_Address = "203.195.236.218";
//#endif

// 数据库服务器 DataBaseeSrv 端口
const in_port_t   DataBaseSrv_Port = 8887;

namespace DB
{
    //#ifdef _DEBUG_VERSION
    ////const int LOCAL_PORT                           = 8887;
    //const int SQL_PORT                             = 3306;
    //const char* const SQL_HOST                     = "localhost";
    //const char* const SQL_USER                     = "root";
    ////const char* const SQL_PWD                      = "yill3813";
    //const char* const SQL_PWD                      = "SSDD5512";
    //const char* const SQL_DB                       = "CTCloud";
    //#else
    ////const int LOCAL_PORT                           = 8888;
    //const int SQL_PORT                             = 3306;
    //const char* const SQL_HOST                     = "localhost";
    //const char* const SQL_USER                     = "root";
    //const char* const SQL_PWD                      = "vfgf#h89";
    //const char* const SQL_DB                       = "CTCloud";
    //#endif


    /******************************************************
    * database: CTCloud
    * table: T_User
    ******************************************************/
    const char* const TUSER                        = "T_User";
    const char* const TUSER__ID                    = "ID";
    const char* const TUSER__ACCOUNT               = "account";
    const char* const TUSER__PWD                   = "password";
    const char* const TUSER__MAIL                  = "mail";
    const char* const TUSER__PHONE                 = "phone";
    const char* const TUSER__SECURE_MAIL           = "secureMail";
    const char* const TUSER__NICKNAME              = "nickName";
    const char* const TUSER__SIGNATURE             = "signature";
    const char* const TUSER__HEAD                  = "head";
    const char* const TUSER__SEX                   = "sex";
    const char* const TUSER__BIRTHDAY              = "birthday";
    const char* const TUSER__REGION                = "region";
    const char* const TUSER__LAST_LOGIN_TIME       = "lastLoginTime";
    const char* const TUSER__ACTIVATE_FLAG         = "activateFlag";
    const char* const TUSER__ACTIVATION_CODE       = "activationCode";
    const char* const TUSER__CREATE_DATE           = "createDate";

    /******************************************************
    * database: CTCloud
    * table: T_FriendShip
    ******************************************************/
    const char* const TFRIENDS                     = "T_FriendShip";
    const char* const TFRIENDS__SELF_ID            = "selfID";
    const char* const TFRIENDS__FRIEND_ID          = "friendID";
    const char* const TFRIENDS__SELF_NOTENAME      = "selfNoteName";
    const char* const TFRIENDS__FRIEND_NOTENAME    = "friendNoteName";
    const char* const TFRIENDS__CREATE_DATE        = "createDate";

    /******************************************************
    * database: CTCloud
    * table: T_UserCluster
    ******************************************************/
    const char* const TUSERCL                      = "T_UserCluster";
    const char* const TUSERCL__ID                  = "ID";
    const char* const TUSERCL__NAME                = "name";
    const char* const TUSERCL__DESCRIPTION         = "description";
    const char* const TUSERCL__CREATOR_ID          = "creatorID";
    const char* const TUSERCL__CREATE_DATE         = "createDate";

    /******************************************************
    * database: CTCloud
    * table: T_Device
    ******************************************************/
    const char* const TDEVICE                      = "T_Device";
    const char* const TDEVICE__ID                  = "ID";
    const char* const TDEVICE__TYPE                = "type";
    const char* const TDEVICE__NAME                = "name";
    const char* const TDEVICE__NOTENAME            = "noteName";
    const char* const TDEVICE__DESCRIPTION         = "description";
    const char* const TDEVICE__MAC_ADDR            = "macAddr";
    const char* const TDEVICE__CLUSTER_ID          = "clusterID";
    const char* const TDEVICE__LOCAL_UDP_PORT      = "localUDPPort";
    const char* const TDEVICE__NEW_PROTOCOL_FLAG   = "newProtocolFlag";
    const char* const TDEVICE__FIRM_CLUSTER_ID     = "firmClusterID";
    const char* const TDEVICE__ENCRYPTION_METHOD   = "encryptionMethod";
    const char* const TDEVICE__KEY_GENERATION_METHOD   = "keyGenerationMethod";
    const char* const TDEVICE__CLAIM_METHOD        = "claimMethod";
    const char* const TDEVICE__TRANSFER_METHOD     = "transferMethod";
    const char* const TDEVICE__LOGIN_KEY           = "szLoginKey";
    const char* const TDEVICE__CLIENT_TRANSFER_FLAG    = "clientTransferFlag";
    const char* const TDEVICE__CREATE_DATE         = "createDate";
    const char* const TDEVICE__DELETE_FLAG         = "deleteFlag";
    const char* const TDEVICE__PREVIOUS_ID          = "previousID";

    /******************************************************
    * database: CTCloud
    * table: T_DeviceCluster
    ******************************************************/
    const char* const TDEVCL                       = "T_DeviceCluster";
    const char* const TDEVCL__ID                   = "ID";
    const char* const TDEVCL__TYPE                 = "type";
    const char* const TDEVCL__ACCOUNT              = "account";
    const char* const TDEVCL__NOTENAME             = "noteName";
    const char* const TDEVCL__FULL_NAME            = "fullName";
    const char* const TDEVCL__DESCRIPTION          = "describ";
    const char* const TDEVCL__CREATOR_ID           = "creatorID";
    const char* const TDEVCL__SYSADMIN_ID          = "sysAdminID";
    const char* const TDEVCL__CREATE_DATE          = "createDate";
    /******************************************************
    * database: CTCloud
    * table: T_User_UsrCluster, 用户加入了哪些群
    ******************************************************/
    const char* const TUSER_USERCL                 = "T_User_UsrCluster";
    const char* const TUSER_USERCL__USER_ID        = "userID";
    const char* const TUSER_USERCL__USRCLUSTER_ID  = "userClusterID";
    const char* const TUSER_USERCL__ROLE           = "role";

    /******************************************************
    * database: CTCloud
    * table: T_User_DevCluster, 用户加入了哪些设备群
    ******************************************************/
    const char* const TUSER_DEVCL                  = "T_User_DevCluster";
    const char* const TUSER_DEVCL__USER_ID         = "userID";
    const char* const TUSER_DEVCL__DEVCLUSTER_ID   = "deviceClusterID";
    const char* const TUSER_DEVCL__ROLE            = "role";

    /******************************************************
    * database: CTCloud
    * table: T_User_Device   用户拥有哪些设备
    ******************************************************/
    const char* const TUSER_DEVICE                 = "T_User_Device";
    const char* const TUSER_DEVICE__USER_ID        = "userID";
    const char* const TUSER_DEVICE__DEVICE_ID      = "deviceID";
    const char* const TUSER_DEVICE__AUTHORITY      = "authority";
    const char* const TUSER_DEVICE__AUTHORIZER_ID  = "authorizerID";

    /******************************************************
    * database: CTCloud
    * table: T_Scene
    ******************************************************/
    const char* const TSCENE                       = "T_Scene";
    const char* const TSCENE__ID                   = "ID";
    const char* const TSCENE__NAME                 = "name";
    const char* const TSCENE__CREATOR_ID           = "creatorID";
    const char* const TSCENE__OWNER_ID             = "ownerID";
    const char* const TSCENE__CREATE_DATE          = "createDate";

    /******************************************************
    * database: CTCloud
    * table: T_User_Scene
    ******************************************************/
    const char* const TUSER_SCENE                  = "T_User_Scene";
    const char* const TUSER_SCENE__USER_ID         = "userID";
    const char* const TUSER_SCENE__SCENE_ID        = "sceneID";
    const char* const TUSER_SCENE__AUTHORITY       = "authority";

    /******************************************************
    * database: CTCloud
    * table: T_DevCLuster_Scene
    ******************************************************/
    const char* const TDEVCL_SCENE                 = "T_DevCLuster_Scene";
    const char* const TDEVCL_SCENE__DEVCLUSTER_ID  = "deviceClusterID";
    const char* const TDEVCL_SCENE__SCENE_ID       = "sceneID";

    /******************************************************
    * database: CTCloud
    * table: T_Feedback
    ******************************************************/
    const char* const TFEEDBACK                    = "T_Feedback";
    const char* const TFEEDBACK__USER_ID           = "userID";
    const char* const TFEEDBACK__FEEDBACK          = "feedback";
    const char* const TFEEDBACK__SUBMIT_TIME       = "submitTime";
    //******************************************************
    /******************************************************
    * database: CTCloud
    * table: T_OfflineMsg
    ******************************************************/
    const char* const TOFFL             = "T_OfflineMsg";
    const char* const TOFFL_ID          = "ID";
    const char* const TOFFL_OBJECT_ID   = "subjectID";
    const char* const TOFFL_PRIORITY    = "priority";
    const char* const TOFFL_CREATE_TIME = "recvTime";
    const char* const TOFFL_NOTIF_NUM   = "notifyNum";
    const char* const TOFFL_MSG_LEN     = "msgLen";
    const char* const TOFFL_MSG_BUF     = "msg";
}

#endif 

