#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdlib.h>
#include <stdint.h>

// 进程通信的消息队列中最多可以有多少个消息, 此值不可以超过下面这个文件中的值
// proc/sys/fs/mqueue/msgsize_max, 具体更多限制参数posix message queue文档和
// limits.conf文档
const uint32_t MAX_MSG_IN_MSG_QUEUE = 1000;

// 单个消息最大可以有多少个字节
const uint32_t MAX_MSG_SIZE      = 30 * 1024; // 30 k
const uint32_t MAX_MSG_SIZE_V2   = 512; // 协义版本号为2时最大包长度为1024字节
// 通信协义的包头以及包尾一共26字节, 除了数据域其它都算包头/尾
const uint32_t PACK_HEADER_SIZE_V2  = 26; 
// 通信协义的数据域最大数据长度
const uint32_t MAX_MSG_DATA_SIZE_V2 = MAX_MSG_SIZE_V2 - PACK_HEADER_SIZE_V2;
const uint32_t TCP_MAX_MSG_DATA_SIZE = 64 * 1024 - 20;

// 消息队列满时偿试重发的次数
const uint32_t MAX_RETRY_WHEN_MSGQUEUE_FULL = 10;

enum IdType{ IT_Unknown, IT_User, IT_Device, IT_UClus, IT_DClus};
IdType GetIDType(uint64_t id);
uint64_t Mac2DevID(uint64_t macAddr);
// ...
#define GetIdType(a) (GetIDType((a)))
#define Mac2DevId(a) (Mac2DevID((a)))
//IdType GetIdType(uint64_t id);
//uint64_t Mac2DevId(uint64_t macAddr);

const uint8_t  CLIENT_MSG_PREFIX = 0xF0;  //消息开始标识
const uint8_t  CLIENT_MSG_SUFFIX = 0xF1;  //消息结束标识
const uint8_t  DEVICE_MSG_PREFIX = 0xE0;  //消息开始标识
const uint8_t  DEVICE_MSG_SUFFIX = 0xE1;  //消息结束标识
const uint8_t  CD_MSG_PREFIX = 0xD0;  //通信消息开始标识
const uint8_t  CD_MSG_SUFFIX = 0xD1;  //通信消息结束标识
//
const uint8_t  SERVER_MSG_PREFIX = 0xFA;  //消息开始标识
const uint8_t  SERVER_MSG_SUFFIX = 0xFB;  //消息开始标识

const time_t HEART_BEAT_INTERVAL = 30;
const int HEART_BEAT_TIMEOUT = 30;

// NTP Notify type 二级功能码
namespace CMD
{
    // 内部用的ID: 0x0 - 0xfff
    const uint16_t INTERNAL_CLAIM_TIMER_RESEND  = 0x1;

    //----------- device module -----------//
    const uint16_t DCMD__OLD_FUNCTION                                 = 0x1000;
    const uint16_t DCMD__REGISTER                                     = 0x1001;
    const uint16_t DCMD__LOGIN1                                       = 0x1002;
    const uint16_t DCMD__LOGIN2                                       = 0x1003;
    const uint16_t DCMD__LOGOUT                                       = 0x1004;
    const uint16_t DCMD__LONG_HEART_BEAT                              = 0x1005;
    const uint16_t DCMD__SHORT_HEART_BEAT                             = 0x1006;
    //const uint16_t DCMD__NOTIFY_CLINET_SESSION_KEY_CHANGED          = 0x1007;
    const uint16_t DNTP__TRANSFER_DEIVCES                             = 0x1008;


    //----------- personal module -----------//
    const uint16_t CMD_PER__REGISTER1__SET_MAILBOX                    = 0x1200;//1000; //注册第一步，设置邮箱
    const uint16_t CMD_PER__REGISTER2__ACTIVATE                       = 0x1201;//1001; //注册第二步，激活
    const uint16_t CMD_PER__REGISTER3__SET_ACCOUNT_PWD                = 0x1202;//1002; //注册第三步，设置帐号密码
    const uint16_t CMD_PER__LOGIN1__REQ                               = 0x1203;//1010; //登录第一步
    const uint16_t CMD_PER__LOGIN2__VERIFY                            = 0x1204;//1011; //登录第二步
    const uint16_t CMD_PER__LOGOUT                                    = 0x1205;//1012; //注销
    const uint16_t CMD_GET_SERVER_LIST                                = 0x1206;//1012; //注销
    const uint16_t NTP_USER_LOGIN_AT_ANOTHER_CLIENT                   = 0x1207; // 通知, 用户在别处登录了
    const uint16_t SET_TCP_SESSION_ID                                 = 0x1208; //

    //---------------------------------------
    const uint16_t CMD_PER__RECOVER_PWD1__REQ                         = 0x1210;//1013; //找回密码第一步
    const uint16_t CMD_PER__RECOVER_PWD2__VERIFY                      = 0x1211;//1014; //找回密码第二步
    const uint16_t CMD_PER__RECOVER_PWD3__RESET                       = 0x1212;//1015; //找回密码第三步
    const uint16_t CMD_PER__MODIFY_PASSWORD                           = 0x1213;//1017; //*修改密码
    const uint16_t CMD_PER__MODIFY_MAILBOX1__REQ                      = 0x1214;//1018; //*修改邮箱第一步
    const uint16_t CMD_PER__MODIFY_MAILBOX2__VERIFY                   = 0x1215;//1019; //*修改邮箱第二步
    const uint16_t CMD_PER__MODIFY_NICKNAME                           = 0x1216;//101A; //*修改昵称
    const uint16_t CMD_PER__MODIFY_SIGNATURE                          = 0x1217;//101B; //*修改签名
    const uint16_t CMD_PER__MODIFY_HEAD                               = 0x1218;//101B; //*修改头像
    //---------------------------------------
    const uint16_t CMD_PER__GET_USER_INFO                             = 0x1230;//1016; //*获取个人信息


    //----------- myFriends module -----------//
    const uint16_t CMD_FRI__ADD_FRIEND__REQ                           = 0x1400;//2020; //加好友请求
    const uint16_t NTP_FRI__ADD_FRIEND__REQ                           = 0x1400;//2020; //加好友请求
    const uint16_t CMD_FRI__ADD_FRIEND__APPROVAL                      = 0x1401;//2021; //
    const uint16_t NTP_FRI__ADD_FRIEND__APPROVAL                      = 0x1401;//2021; //
    //---------------------------------------
    const uint16_t CMD_FRI__GET_FRIEND_INFO_LIST                      = 0x1410;//2001; //获取好友信息列表

    //----------- userCluster module -----------//
    const uint16_t CMD_UCL__CREATE                                    = 0x1600;//2012; //创建用户群
    const uint16_t CMD_UCL__JOIN__SEARCH_CLUSTER                      = 0x1601;//2011; //搜索用户群
    const uint16_t CMD_UCL__JOIN__REQ                                 = 0x1602;//2022; //加入群，请求
    const uint16_t NTP_UCL__JOIN__REQ                                 = 0x1602;//2022; //加入群，通知请求
    const uint16_t CMD_UCL__JOIN__APPROVAL                            = 0x1603;//2023; //加入群，审批
    const uint16_t NTP_UCL__JOIN__RESULT                              = 0x1603;//2023; //加入群，通知结果
    //---------------------------------------
    const uint16_t CMD_UCL__GET_CLUSTER_LIST                          = 0x1610;//2003; //获取用户群列表
    const uint16_t CMD_UCL__GET_CLUSTER_INFO                          = 0x1611;//2005;

    //----------- myDevices module -----------//
    const uint16_t CMD_DEV__MODIFY_DEVICE_NOTENAME                    = 0x1800;//3009;
    const uint16_t CMD_DEV__MODIFY_DEVICE_DESCRIPTION                 = 0x1801;//300A;
    //---------------------------------------
    const uint16_t CMD_DEV__GET_MY_DEVICE_LIST                        = 0x1810;//3001; //获取我的设备列表
    const uint16_t CMD_DEV__GET_MY_DEVICE_ONLINE_LIST                 = 0x1811;//3002; //获取我的设备在线列表

    //----------- deviceCluster module -----------//
    const uint16_t CMD_DCL__CREATE                                    = 0x1A00;//3017; //创建设备群
    const uint16_t CMD_DCL__DELETE__REQ                               = 0x1A01;//301A; //删除设备群请求
    const uint16_t NTP_DCL__DELETE                                    = 0x1A01;//301A;
    const uint16_t CMD_DCL__DELETE__VERIFY                            = 0x1A02;//301B; //删除设备群确认（通过验证码）
    const uint16_t CMD_DCL__JOIN__SEARCH_CLUSTER                      = 0x1A03;//3012; //搜索设备群
    const uint16_t CMD_DCL__JOIN__REQ                                 = 0x1A04;//3014; //加入设备群，请求
    const uint16_t NTP_DCL__JOIN__REQ                                 = 0x1A04;//3014; //加入设备群，通知请求
    const uint16_t CMD_DCL__JOIN__APPROVAL                            = 0x1A05;//3015; //加入设备群，审批
    const uint16_t NTP_DCL__JOIN__RESULT                              = 0x1A05;//3015; //加入设备群，通知结果
    const uint16_t NOTIFY_NEW_ADDED_TO_CLUSTER                        = 0x1A0B;//3015; //加入设备群，审批
    const uint16_t CMD_DCL__EXIT                                      = 0x1A06;//3019; //退出设备群
    //const uint16_t NTP_DCL__EXIT                                      = 0x1A0C;//3019;
    const uint16_t CMD_DCL__INVITE_TO_JOIN__REQ                       = 0x1A07;//3021; //邀请加入设备群，请求
    const uint16_t NTP_DCL__INVITE_TO_JOIN__REQ                       = 0x1A07;//3021; //邀请加入设备群，通知请求
    const uint16_t CMD_DCL__INVITE_TO_JOIN__APPROVAL                  = 0x1A08;//3022; //邀请加入设备群，审批
    const uint16_t NTP_DCL__INVITE_TO_JOIN__RESULT                    = 0x1A08;//3022; //邀请加入设备群，通知结果
    const uint16_t CMD_DCL__REMOVE_USER_MEMBERS                       = 0X1A09;//3024; //批量移除设备群的用户成员
    const uint16_t NTP_DCL__REMOVE_USER_MEMBERS                       = 0x1A09;//3024;

    //const uint16_t CMD_NEW_DEVICE_ADDED_TO_CLUSTER                    = 0x1A0E;

    const uint16_t CMD_A_USER_BE_REMOVED_FROM_CLUSTER                 = 0x1A0C;

    // 设备群移交
    const uint16_t CMD_DCL__TRANSFER_DEVCLUSTER_REQ                   = 0x1A23;
    const uint16_t CMD_DCL__TRANSFER_DEVCLUSTER_VERIRY                = 0x1A24;

    //---------------------------------------
    const uint16_t CMD_DCL__GET_CLUSTER_INFO                          = 0x1A10;//3018; //获取设备群信息
    const uint16_t CMD_DCL__GET_CLUSTER_LIST                          = 0x1A11;//3003; //获取设备群列表
    const uint16_t CMD_DCL__GET_DEVICE_MEMBER_LIST                    = 0x1A12;//3004; //获取设备群的设备成员列表
    const uint16_t CMD_DCL__GET_DEVICE_MEMBER_ONLINE_LIST             = 0x1A13;//3006; //获取设备群的设备在线列表
    const uint16_t CMD_DCL__GET_OPERATOR_LIST_OF_DEV_MEMBER           = 0x1A14;//302B; //获取群设备成员的操作员列表
    const uint16_t CMD_DCL__GET_USER_MEMBER_LIST                      = 0x1A15;//3005; //获取设备群的用户成员列表
    const uint16_t CMD_DCL__GET_USER_MEMBER_ONLINE_LIST               = 0x1A18;//3005; //获取设备群的用户成员列表
    const uint16_t CMD_DCL__GET_DEVICE_LIST_OF_USER_MEMBER            = 0x1A16;//302A; //获取群用户成员的设备列表
    const uint16_t CMD_DCL__GET_UNCLAIMED_DEVICE_LIST                 = 0x1A17;
    //---------------------------------------
    const uint16_t CMD_DCL__GET_TRANSER_DEVICE_LIST                   = 0x1A20;
    const uint16_t CMD_DCL__TRANSFER_DEVICES__REQ                     = 0x1A21;//3031
    const uint16_t NTP_DCL__TRANSFER_DEVICES__REQ                     = 0x1A21;//3031
    const uint16_t CMD_DCL__TRANSFER_DEVICES__APPROVAL                = 0x1A22;//
    const uint16_t NTP_DCL__TRANSFER_DEVICES__RESULT                  = 0x1A22;//
    const uint16_t DCL__TRANSFER_DEVICES__NOTIFY_SRC_CLUSTER          = 0x1A0D;//
    const uint16_t DCL__TRANSFER_DEVICES__NOTIFY_DEST_CLUSTER         = 0x1A0A;//

    const uint16_t CMD_DCL__MODIFY_USER_MEMBER_ROLE                   = 0x1A30;//3040; //修改设备群的用户成员角色（设置／取消管理员）
    const uint16_t CMD_DCL__MODIFY_NOTENAME                           = 0x1A31;//3041; //修改设备群备注名
    const uint16_t CMD_DCL__MODIFY_DISCRIPTION                        = 0x1A32;//3042; //修改设备群描述
    const uint16_t CMD_DCL__ASSIGN_DEVICES_TO_USER_MEMBER             = 0x1A33;//3026; //为群用户成员批量指派设备
    const uint16_t NTP_DCL__ASSIGN_DEVICES_TO_USER_MEMBER             = 0x1A33;//3026;
    const uint16_t CMD_DCL__REMOVE_DEVICES_OF_USER_MEMBER             = 0x1A34;//3027; //为群用户成员批量删除设备
    const uint16_t NTP_DCL__REMOVE_DEVICES_OF_USER_MEMBER             = 0x1A34;//3027;
    const uint16_t CMD_DCL__ASSIGN_OPERATORS_TO_DEV_MEMBER            = 0x1A35;//3028; //为群设备成员批量指派操作员
    const uint16_t NTP_DCL__ASSIGN_OPERATORS_TO_DEV_MEMBER            = 0x1A35;//3028;
    const uint16_t CMD_DCL__REMOVE_OPERATORS_OF_DEV_MEMBER            = 0x1A36;//3029; //为群设备成员批量删除操作员
    const uint16_t NTP_DCL__REMOVE_OPERATORS_OF_DEV_MEMBER            = 0x1A36;//3029;
    const uint16_t CMD_DCL__REMOVE_MY_DEVICES                         = 0x1A37;// 把设备从我的设备列表中删除
    const uint16_t CMD_DCL__REMOVE_DEVICES_FROM_DEVICE_CLUSTER        = 0x1A38;// 把设备从设备群中删除
    const uint16_t NTP_DCL__REMOVE_DEVICES_FROM_DEVICE_CLUSTER        = 0x1A38;// 上个命令的通知

    //---------------------------------------
    const uint16_t CMD_DCL__CLAIM_DEVICE                              = 0x1A40;
    const uint16_t CMD_CLAIM_CHECK_PASSWORD                           = 0x1A43;
    const uint16_t NTP_DCL__CLAIM_DEVICE                              = 0x1A40;
    const uint16_t CCMD__DEVICE_CLAIM                                 = 0x1A40;

    const uint16_t CMD_DCL__GET_SESSION_KEY_WITH_DEVICE               = 0x1A41;
    const uint16_t CMD_DCL__CANCEL_COMMUNICATION_WITH_DEVICE          = 0x1A42;

    //----------- HMI module -----------//
    const uint16_t CMD_HMI__PENETRATE__GET_MY_DEVICE_LIST             = 0x1C00;//4000; //HMI，批量穿透，获取我的设备列表
    const uint16_t CMD_HMI__PENETRATE__REQ                            = 0x1C01;//4001; //HMI，批量穿透
    const uint16_t CMD_HMI__GET_EXTRANET_IP_PORT                      = 0x1C02;//4002; //HMI，获取外网ip，port
    const uint16_t CMD_HMI__OPEN_RELAY_CHANNELS                       = 0x1C03;//4003; //HMI，批量打开转发通道
    const uint16_t CMD_HMI__CLOSE_RELAY_CHANNELS                      = 0x1C04;//4004; //HMI，批量关闭转发通道
    const uint16_t CMD_HMI_IS_SCENE_RUN_AT_SERVER                     = 0x1c07; // 场景是否启动离线监控
    const uint16_t CMD_HMI_START_SCENE_RUN_AT_SERVER                  = 0x1c08; // 启动场景离线监控
    const uint16_t CMD_HMI_STOP_SCENE_RUN_AT_SERVER                   = 0x1c09; // 停止场景离线监控
    const uint16_t CMD_NEW_RABBITMQ_MESSAGE_ALARM_NOTIFY              = 0x1c0a; // 报警通知 
    //---------------------------------------
    const uint16_t CMD_HMI__UPLOAD_SCENE                              = 0x1C10;//4005; //HMI，上传场景
    const uint16_t CMD_HMI__UPLOAD_SCENE_CONFIRM                      = 0x1C19;//4005; //HMI，上传场景Confirm
    const uint16_t CMD_HMI__DOWNLOAD_SCENE                            = 0x1C11;//4007; //HMI，下载场景
    const uint16_t CMD_HMI__SHARE_SCENE                               = 0x1C12;//4008; //HMI，共享场景
    const uint16_t CMD_HMI__CANCEL_SHARE_SCENE                        = 0x1C13;//400A; //HMI，取消共享场景
    const uint16_t CMD_HMI__DELETE_SCENE                              = 0x1C14;//400C; //HMI，删除场景
    const uint16_t CMD_HMI__TRANSFER_SYSADMIN                         = 0x1C15;
    const uint16_t CMD_HMI__SET_SHARE_SCENE_AUTH                    = 0x1C16;

    const uint16_t CMD_SCENE_FILE_CHANGED = 0x1C18; // 服务器收到此命令后给场景的所有共享者发送通知,告知场景文件被修改需要从新下载

    //---------------------------------------
    const uint16_t CMD_HMI__GET_MY_SCENE_LIST                         = 0x1C20;//4006; //HMI，获取我的场景列表
    const uint16_t CMD_HMI__GET_SCENE_USER_LIST                       = 0x1C21;//400F; //HMI，获取场景的用户列表
    const uint16_t CMD_HMI__GET_NEW_CDSESSION_KEY                     = 0x1C22;

    const uint16_t CMD_HMI__MODIFY_SCENE_INFO                       = 0x1C23; // 修改场景信息
    const uint16_t CMD_HMI__MODIFY_NOTENAME                         = 0x1C24; // 修改场景备注名
    const uint16_t CMD_HMI__GET_SCENE_INFO                          = 0x1C25; // 取场景信息

    // scene conector
    const uint16_t CMD_SET_SCENE_CONNECTOR_DEVICES = 0x1C27; // 修改场景连接的设备
    const uint16_t CMD_GET_SCENE_CONNECTOR_DEVICES = 0x1C28; // 取得场景连接的设备
    const uint16_t NOTIFY_SCNEE_CONNECTOR_DEVICES_CHANGED = 0x1C29; // 场景连接设备被修改


    //----------- system module -----------//
    const uint16_t CMD_GEN__FEEDBACK                                = 0x1E00;//101C;

    //----------- serverNotification module -----------//
    const uint16_t NTP_DEV__DEVICE_ONLINE_STATUS_CHANGE             = 0x2000;//3008;
    const uint16_t NTP_DEV__USER_ONLINE_STATUS_CHANGE               = 0x2001;//3008;
    const uint16_t NTP_CMD_DEVICE_NAME_CHANGED                      = 0x2002;
    const uint16_t NTP_CMD_SERVER_PUSH_MESSAGE                      = 0x2003;

    //----------- common module -----------//
    const uint16_t CMD_FRI__ADD_FRIEND__SEARCH_USER                   = 0x2200;//2010; //搜索用户
    const uint16_t CMD_DCL__INVITE_TO_JOIN__SEARCH_INVITEE            = 0x2201;//3020; //邀请加入设备群，搜索用户
    const uint16_t CMD_HMI__SHARE_SCENE__SEARCH_USER                  = 0x2202;//400E; //HMI，共享场景，搜索用户
    const uint16_t CMD_FRI__GET_USER_INFO                             = 0x2203;//2004; //
    const uint16_t CMD_DCL__GET_DEVICE_INFO                           = 0x2204;//3007;
    const uint16_t CMD_SEARCH_DEVICE_BY_NAME_AND_MAC                  = 0x2205;// 搜索设备

    //----------- other module -----------//
    const uint16_t CMD_GEN__NOTIFY                                    = 0xFFFE;//FFFE; //通知
    const uint16_t CMD_NOTIFY_RESP                                    = 0xFFFD;//FFFD; //客户端通知服务器收到通知
    //const uint16_t CMD_MoveNotifToDatabase                            = 0x2308;
    // sessional通知的意思是, 这个通知不会保存到数据库中, 重发几次后不管有没有收到都会丢掉
    //const uint16_t CMD_SendSessionalNotify                            = 0x2308;

    const uint16_t CMD_GEN__HEART_BEAT                                = 0xFFFF;//FFFF; //客户端心跳包


    //----------- internal cmd -----------//
    const uint16_t INCMD_USER_ONLINE                             = 0x0010; //用户上线
    const uint16_t INCMD_USER_OFFLINE                            = 0x0011; //用户下线
    const uint16_t INCMD_DEVICE_ONLINE                           = 0x0012; //设备上线
    const uint16_t INCMD_DEVICE_OFFLINE                          = 0x0013; //设备下线
    const uint16_t INCMD_GET_DEVICE_ONLINE_LIST                  = 0x0014; //获取设备在线列表
    const uint16_t CMD_HMI__OPEN_RELAY_CHANNELS_RESULT           = 0x0015;//4004; //HMI，批量关闭转发通道
    const uint16_t CMD_HMI__CLOSE_RELAY_CHANNELS_RESULT          = 0x0016;//4004; //HMI，批量关闭转发通道

    // CMD_SendNotif 这个命令由noitfysr生成, 发到logicalsrv中告诉logicalsrv把这个通知发出去
    const uint16_t CMD_SEND_NOTIF                                 = 0x0017;
    // 这种通知会保存到数据库中, 直到用户确认收到才会从数据库中删除
    const uint16_t CMD_NEW_NOTIFY                                 = 0x0018;
    const uint16_t CMD_NEW_SESSIONAL_NOTIFY                        = 0x0019;
    //const uint16_t CMD_SaveSessionalOnlineNotif                       = 0x2304;
    const uint16_t CMD_GET_NOTIFY_WHEN_LOGIN                         = 0x001A;
    const uint16_t CMD_GET_NOTIFY_WHEN_BACK_ONLINE                    = 0x001B;
    const uint16_t CMD_DELETE_NOTIFY                               = 0x001C;

    // 内部消息, 当用户的 "我的设备列表" 发生改变时产生
    // 此消息的处理类会检查用户对它自己所有的场景中使用的设备是否有权限
    // 如果没有权限则产生一个通知发到客户端
    //const uint16_t USER_DEVICE_LIST_CHANGED = 0x1D;
    //const uint16_t USER_DEVICE_LIST_CHANGED_TRANSFERED = 0x1E; // 当设备被移走后发的消息

    const uint16_t CMD_DEL_LOCAL_CACHE = 0x001D;// 在本地删除用户的登录信息

    const uint16_t IN_CMD_NEW_PUSH_MESSAGE                           = 0x001E;

    const uint16_t IN_CMD_NEW_RABBITMQ_MESSAGE                           = 0x001F;
    const uint16_t IN_CMD_NEW_RABBITMQ_MESSAGE_ALARM_NOTIFY              = 0x1c0a; // 报警通知 
	//const uint16_t CMD_SaveOfflineNotif							   	     = 0x0020;
}

namespace ANSC
{
    const uint8_t PENDING               = 0x00; //待处理
    const uint8_t SUCCESS               = 0x01; //成功
    const uint8_t REGIST_FAILED          = 0x02; //注册失败，帐号或邮箱已被使用
    const uint8_t VERIFY_CODE_ERROR         = 0x03; //验证码错误
    const uint8_t ACCOUNT_NOT_EXIST       = 0x04; //帐号不存在
    const uint8_t PASSWORD_ERROR                = 0x05; //登录失败，帐号或密码错误
    const uint8_t EMAIL_NOT_EXIST         = 0x06; //邮箱错误
    const uint8_t EMAIL_USED             = 0x07; //邮箱已被使用
    const uint8_t REGIST_RETRY_TIMES_OUT   = 0x08; //同一个IP频繁注册，限制一小时不允许注册
    const uint8_t LOGIN_RETRY_TIMES_OUT    = 0x09; //同一个帐号密码错误重试十次，限制一小时不允许登录
    const uint8_t DEVICE_ID_ERROR           = 0x0A; //设备不存在或没有权限
    const uint8_t SCENE_ID_ERROR            = 0x0B; //场景不存在或没有权限
    const uint8_t SCENE_ID_OR_USER_ID_ERROR    = 0x0C; //场景或用户不存在或没有权限
    const uint8_t CLOUD_DISK_NOT_ENOUGH     = 0x0D; //场景上传失败，服务器上空间不够，请删除旧的场景
    const uint8_t ACCOUNT_USED           = 0x0E; //创建设备群失败，帐号已存在
    const uint8_t CLUSTER_ID_ERROR          = 0x0F; //设备群不存在或没有权限
    const uint8_t DEVICE_OFFLINE         = 0x10; //认领设备失败，设备不在线或没有权限
    const uint8_t DEVICE_PASSWORD_ERROR          = 0x11; //认领设备失败，密码错误
    const uint8_t CLUSTER_ID_OR_DEVICE_ID_ERROR= 0x12; //群或设备不存在或没有权限
    const uint8_t USER_ID_OR_DEVICE_ID_ERROR   = 0x13; //用户或设备不存在或没有权限
    const uint8_t CLUSTER_ID_OR_USER_ID_ERROR  = 0x14; //群或用户不存在或没有权限
    const uint8_t AUTH_ERROR               = 0x15; //没有权限
    const uint8_t FAILED                = 0x16; // 失败
    const uint8_t VERIFY_CODE_EXPIRED     = 0x17; // 失败
    const uint8_t DEVICE_NO_RESP          = 0x18; // 设备没有回复
    const uint8_t TURN_NOT_EXIST          = 0x19; //转发通道未打开, 这个错误码没有用到
    const uint8_t CLIENT_NEED_UPDATE      = 0x1A; //客户端版本太低，需要更新
    const uint8_t CLUSTER_MAX_USER_LIMIT          = 0x1B; // 群里用户数达到最大限制
    const uint8_t CLUSTER_MAX_DEVICE_LIMIT        = 0x1C; // 群里设备数达到最大限制
    const uint8_t CLUSTER_MAX_LIMIT        = 0x1D; // 群数量达到最大限制
    const uint8_t SCENE_SHARE_MAX_LIMIT        = 0x1E; // 场景共享次数达到最大限制
	const uint8_t DEL_SCENE_WITH_ALARM         = 0X20; // 带离散报警的场景不能直接被删除，必须先停止报警监控
	const uint8_t CHANGE_DEVICE_SCENE_WITH_ALARM         = 0X21; // 带离散报警的场景不能变更设备，必须先停止报警监控

    const uint8_t MESSAGE_ERROR                = 0x7F; //命令格式错误
    const uint8_t ACCEPT            = 0x7D; //审批同意
    const uint8_t REFUSE            = 0x7E; //审批拒绝
}

namespace Relay
{
    const uint8_t OPEN_CHANNEL = 0x01;
    const uint8_t CLOSE_CHANNEL = 0x02;
    const uint8_t CLOSE_ALL_CHANNEL = 0x03;
}

namespace ClusterAuth
{
    const uint8_t OWNER = 1; // 群主, 系统管理员
    const uint8_t MANAGER = 2;  // 管理员
    const uint8_t OPERATOR = 3; // 操作者
}

namespace SceneAuth
{
    const uint8_t READ = 1;
    const uint8_t READ_WRITE = 2;
}

enum OnlineStatus
{
    ONLINE  = 0x01,
    OFFLINE = 0x02,
    OS_OTHER   = 0x03
};

namespace DeviceStatu
{
    const uint8_t Active = 0;
    const uint8_t Deleted = 1; // deleted
};
#endif //PROTOCOL_H
