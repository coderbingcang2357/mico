#ifndef USEROPERATOR__H
#define USEROPERATOR__H
#include "iuseroperator.h"
#include <vector>
#include <list>
#include <string>
#include <map>
#include <memory>

class IMysqlConnPool;
class MysqlConnection;

class DeviceInfo
{
public:
    uint64_t id;
    uint32_t type;
    std::vector<char> macAddr;
    std::string name;
    std::string noteName;
    std::string description;
    uint64_t clusterID;
    std::vector<char> loginkey;
    int status;
};

class DevClusterInfo
{
public:
    uint8_t role;
    uint64_t id;
    std::string account;
    std::string noteName;
    std::string description;
};

class SceneInfo
{
public:
    uint64_t id;
    uint64_t creatorid;
    uint64_t ownerid;
    std::string creatorusername;
    std::string ownerusername;
    uint8_t authority; // 0
    std::string name;
    std::string notename;
    std::string description;
    std::string version;
};

class DeviceAutho
{
public:
    uint64_t id;             //deviceID or userID
    uint8_t authority;
    uint32_t type; // device type
    std::vector<char> macAddr; // .
    std::string name;             //deviceName or userName
    std::string signature; // use nickname
    uint32_t header; //  header
};

class SharedSceneUserInfo
{
public:
    uint64_t userid;
    uint8_t authority;
    uint8_t header;
    std::string username;
    std::string signature;
};

class UserMemberInfo
{
public:
    uint8_t role;
    uint64_t id;
    std::string account;
    std::string nickName;
    std::string signature;
    std::string mailBox;
    uint8_t head;
};

class UserOperator : public IUserOperator
{
public:
    UserOperator(IMysqlConnPool *mp);
    ~UserOperator();
    int registerUser(const std::string &username,
        const std::string &mail,
        const std::string &md5password,
        uint64_t *userid);


    int getUserPassword(uint64_t userid, std::string *password);
    int getUserPassword(const std::string &usraccount, std::string *password);
    int getUserIDByAccount(const std::string &usraccount, uint64_t *userid);
    int getUserAccount(uint64_t userid, std::string *transferorAccount);
    bool isUserExist(const std::string &usraccount);
    UserInfo *getUserInfo(uint64_t userid);
    UserInfo *getUserInfo(const std::string &useraccount);
    int modifyPassword(uint64_t userid, const std::string &newpassword);
    int modifyMail(uint64_t userid, const std::string &mail);
    int modifyNickName(uint64_t userid, const std::string &newnickname);
    int modifySignature(uint64_t userid, const std::string &newsignature);
    int modifyUserHead(uint64_t uid, uint8_t header);

    int getMyDeviceList(uint64_t userid, std::vector<DeviceInfo> *);
    int removeMyDevice(uint64_t userid, std::list<uint64_t> &deviceid);

    // 所用户所在的所有群ID
    int getClusterIDListOfUser(uint64_t userid, std::vector<uint64_t> *o);

    // device 
    int getDeviceName(uint64_t deviceID, std::string *deviceName);
    int modifyDeviceName(uint64_t devid, const std::string &name);
    int modifyNoteName(uint64_t deviceID, const std::string &noteName);

    int getDeviceIDAndLoginKey(uint64_t firmClusterID,
            uint64_t macAddr,
            uint64_t *deviceID,
            std::string *strLoginKey);

    int setDeviceStatus(const std::list<uint64_t> &ids, uint8_t status);

    DeviceInfo *getDeviceInfo(uint64_t devid);

    int getUserIDInDeviceCluster(uint64_t clusterid, std::list<uint64_t> *idlist);

    // cluster
    int getClusterAccount(uint64_t destClusterID, std::string *destClusterAccount);
    int getSysAdminID(uint64_t destClusterID, uint64_t *ownerid);
    int checkClusterAdmin( uint64_t userid,
        uint64_t clusterid,
        bool *isadmin);


    int checkClusterAccount(const std::string &clusterAccount, bool *isExisted);

    int insertCluster(uint64_t creatuserid,
                    const std::string &clusterAccount,
                    const std::string &description,
                    uint64_t *newClusterID);

    int modifyClusterNoteName(uint64_t userid,
            uint64_t clusterID,
            const std::string &noteName);

    int modifyClusterDescription(uint64_t userID,
        uint64_t clusterID,
        const std::string &newDescription);

    int getClusterListOfUser(uint64_t userid, std::vector<DevClusterInfo> *o);

    int getClusterInfo(uint64_t userid,
            uint64_t clusterID,
            DevClusterInfo *clusterInfo);

    int getDevMemberInCluster(
        uint64_t clusterID, std::vector<DeviceInfo> *vDeviceInfo);

    int searchCluster(const std::string &clusterAccount, DevClusterInfo* clusterInfo);
    // 取设备群的所有者
    uint64_t getDeviceClusterOwner(uint64_t deviceclusterid);
    // 修改设备群的所有者
    int modifyClusterOwner(uint64_t clusterid, uint64_t newowner, uint64_t oldowner);
    // 设备群中是否有设备
    bool deviceClusterHasDevice(uint64_t deviceclusterid);
    // 删除一个设备群
    int deleteDeviceCluster(uint64_t deviceclusterid);
    // 用户退出群, 从群里面删除用户
    int deleteUserFromDeviceCluster(uint64_t userid, uint64_t devclusterid);
    // 取得群里面某一个设备的所有使用者
    int getOperatorListOfDevMember(uint64_t clusterID,
        uint64_t deviceMemberID,
        std::vector<DeviceAutho> *vDeviceAutho);
    // 取群里面某一个人可以用的设备
    int getDeviceListOfUserMember(uint64_t clusterID,
        uint64_t userMemberID,
        std::vector<DeviceAutho> *vDeviceAutho);
    // 取群里面用户成员列表
    int getUserMemberInfoList(uint64_t clusterID,
        std::vector<UserMemberInfo> *vUserInfo);
    // 设备认领
    int claimDevice(uint64_t userid, uint64_t deviceid);
    // 移交, 修改设备群号
    int modifyDevicesOwnership(const std::vector<uint64_t> &vDeviceID,
        uint64_t srcClusterID,
        uint64_t destClusterID);

    // 设备授权
    int asignDevicesToUserMember(
        uint64_t adminID,
        uint64_t clusterID,
        uint64_t userMemberID,
        const std::map<uint64_t, uint8_t> &mapDeviceIDAuthority);
    // 取消授权
    int removeDevicesOfUserMember(uint64_t adminID,
                    uint64_t clusterID,
                    uint64_t userMemberID,
                    const std::map<uint64_t, uint8_t> &mapDeviceIDAuthority);

    int assignDeviceToManyUser(uint64_t adminID,
                    uint64_t clusterID,
                    uint64_t deviceID,
                    const std::map<uint64_t, uint8_t> &mapUserIDAuthority);

    int removeDeviceUsers( uint64_t adminID,
                    uint64_t clusterID,
                    uint64_t deviceID,
                    const std::map<uint64_t, uint8_t> &mapUserIDAuthority);

    // 把用户加入群
    int insertUserMember(uint64_t userid, uint64_t clusterID);
    // 把用户从群里面删除
    int removeUserMembers(uint64_t adminID,
        std::vector<uint64_t> &vUserMemberID,
        uint64_t clusterID);
    // 设置用户在群里面的权限, 0, 1, 2, 3
    int modifyUserMemberRole(uint64_t adminid,
                            uint64_t userMemberID,
                            uint8_t newRole,
                            uint64_t clusterID);

    // 判断一些设备是否在一个群里面, 任何一个在群里面都返回true
    bool isDeviceInCluster(const std::vector<uint64_t> &idlist, uint64_t clusterid);
    bool isAllDeviceInCluster(uint64_t clusterID,
                            std::list<uint64_t> devids,
                            std::shared_ptr<MysqlConnection> conn = std::shared_ptr<MysqlConnection>());
    bool isUserInCluster(uint64_t clusterID,
                        std::list<uint64_t> userids,
                        std::shared_ptr<MysqlConnection> conn = std::shared_ptr<MysqlConnection>());

    int deleteDeviceFromCluster(uint64_t clustid, const std::list<uint64_t> &ids);


    // scene ------------------------------------------------
    //
    int getMySceneList(uint64_t userid, std::vector<SceneInfo> *r);
    // 
    int modifySceneInfo(uint64_t userid, uint64_t sceneid,
        const std::string &name,
        const std::string &version,
        const std::string &desc);
    // modify noite name of scene
    int modifySceneNoteName(uint64_t userid,
        uint64_t sceneid,
        const std::string &notename);
    // get scene info
    int getSceneInfo(uint64_t userid, uint64_t sceneid, SceneInfo *);

    int getSceneName(uint64_t sceneid, std::string *name);

    // 取得场景被共享给了哪些人
    int getSceneSharedUses(uint64_t userid,
        uint64_t sceneid,
        std::vector<SharedSceneUserInfo> *);

    // 检查userid对sceneid是否有share auth
    bool hasShareAuth(uint64_t userid, uint64_t scendid);

    // 共享场景给users
    int shareSceneToUser(uint64_t userid, uint64_t sceneid,
        const std::vector<std::pair<uint64_t, uint8_t> > &targetusers);

    // 取消共享场景
    int cancelShareSceneToUser(uint64_t userid, uint64_t sceneid,
        const std::vector<std::pair<uint64_t, uint8_t> > &targetusers);

    // 设置共享场景的权限
    int setShareSceneAuth(uint64_t userid, uint64_t sceneid, uint8_t auth,
        const std::vector<uint64_t> &targetusers);

    // 删除场景
    int deleteScene(uint64_t sceneid);

    // .
    int freeDisk(uint64_t userid, uint32_t scenesize);
    int allocDisk(uint64_t targetuserid, uint32_t scenesize);
    // inc version 
    int incSceneVersion(uint64_t sceneid);

    // 用户对场景是否有访部权
    int isUserHasAccessRightOfScene(uint64_t sceneid, uint64_t userid,
        bool *hasauth);

    // 设置场景的所有者: 场景移交
    int setSceneOwnerID(uint64_t sceneid, uint64_t newowner);

    // 场景的所有者
    int getSceneOwner(uint64_t sceneid, uint64_t *owner);

    // 更新场景的大小
    int updateSceneInfo(uint64_t userid,
        const std::string &scenename,
        uint64_t sceneid,
        const std::string &files,
        uint32_t oldszendsize,
        uint32_t newscenesize);

    // 插入新的场景项
    int insertScene(uint64_t userid,
        const std::string &scenename,
        uint32_t allfilesize,
        const std::string &files,
        uint32_t serverid,
        bool shouldAddToMySceneList,
        uint64_t *sceneid);

    int removeUploadLog(uint64_t sceneid);

    // 用户使用的多少空间, 上传的场景一个用户最多100M
    int getUserDiskSpace(uint64_t userid, uint32_t *allsize, uint32_t *usedsize);

    // 取场景的大小
    int getSceneSize(uint64_t sceneid, uint32_t *cursize);

    // 取场景上传的服务器ID
    int getSceneServerid(uint64_t sceneid, uint32_t *serverid);

    // 取场景的文件列表
    int getSceneFileList(uint64_t sceneid, std::string *files);

    // feedback, 反馈
    int saveFeedBack(uint64_t userid, const std::string &text);

    // 更新在线时长和最后登录时间
    int updateUserLastLoginTime(uint64_t userid, time_t lastlogintime);
    int updateUserOnlineTime(uint64_t userid, int onlinetime);
    int updateDeviceLastLoginTime(uint64_t deviceid, time_t lastlogintime);
    int updateDeviceOnlineTime(uint64_t deviceid, int onlinetime);
	IMysqlConnPool *getmysqlconnpool(){ return m_mysqlpool; }

private:
    // MysqlConnection *m_sql;
    IMysqlConnPool *m_mysqlpool;
};

#endif

