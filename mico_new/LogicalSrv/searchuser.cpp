#include "searchuser.h"
#include "useroperator.h"
#include "Message/Message.h"
#include "util/util.h"
#include "Crypt/tea.h"
#include "useroperator.h"
#include "assert.h"
#include "messageencrypt.h"
#include "onlineinfo/userdata.h"
#include "onlineinfo/isonline.h"
#include "onlineinfo/rediscache.h"

SearchUser::SearchUser(ICache *c, UserOperator *uop)
    : m_useroperator(uop), m_cache(c)
{

}

int SearchUser::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    uint8_t answccode = ANSC::MESSAGE_ERROR;
    Message* responseMsg = new Message();
    responseMsg->CopyHeader(reqmsg);
    responseMsg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(responseMsg);
    imsg->setEncrypt(true);
    // 添加到输出
    r->push_back(imsg);

    if(::decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        answccode = ANSC::MESSAGE_ERROR;
        responseMsg->appendContent(&answccode, Message::SIZE_ANSWER_CODE);

        return r->size();
    }

    std::string userAccount;
    int len = readString(reqmsg->content(), reqmsg->contentLen(), &userAccount);
    if (len <= 0)
    {
        // 解包失败
        assert(false);
        answccode = ANSC::FAILED;
        responseMsg->appendContent(&answccode, Message::SIZE_ANSWER_CODE);

        return r->size();
    }

    //
    UserInfo *userInfo;
    userInfo = m_useroperator->getUserInfo(userAccount);
    if(userInfo == 0)
    {
        // 读数据库失败
        answccode = ANSC::ACCOUNT_NOT_EXIST;
        responseMsg->appendContent(&answccode, Message::SIZE_ANSWER_CODE);

        return r->size();
    }
    IsOnline isonlne(m_cache, RedisCache::instance());
    std::unique_ptr<UserInfo> del(userInfo);

    responseMsg->appendContent(&ANSC::SUCCESS, Message::SIZE_ANSWER_CODE);
    responseMsg->appendContent(&(userInfo->userid), sizeof(userInfo->userid));
    responseMsg->appendContent(userInfo->account);
    responseMsg->appendContent(userInfo->nickName);
    responseMsg->appendContent(userInfo->signature);
    responseMsg->appendContent(userInfo->headNumber);
    uint8_t onlinestatu = isonlne.isOnline(userInfo->userid) ? ONLINE : OFFLINE;
    responseMsg->appendContent(onlinestatu); // 201版本这个字段改为在线状态

    return r->size();
}

