#include <vector>
#include "Crypt/MD5.h"
#include "Crypt/tea.h"
#include "Message/Message.h"
#include "util/logwriter.h"
#include "getserverlist.h"
#include "serverinfo/serverinfo.h"
#include "util/util.h"

int GetServerList::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(0);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setEncrypt(false);
    r->push_back(imsg);

    char *p = reqmsg->content();
    int plen = reqmsg->contentLen();

    // 因为key是16字节
    if (plen <= 16)
    {
        ::writelog("get serverlist error msg len.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    std::vector<char> key(p, p + 16);
    p += 16;
    plen -= 16;
    //char buf[245];
    std::vector<char> buf(plen, char(0));
    int l = tea_dec(p, plen, &buf[0], &key[0], 16);
    if (l < 0)
    {
        ::writelog("get serverlist decrypt failed.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    //
    std::string account;
    uint16_t clientversion;
    char *pbuf = &buf[0];
    int readlen = ::readString(pbuf, l, &account);
    if (readlen <= 0)
    {
        // 为了兼容设备某个版本bug: 它发来的数据格式是 [key] [account]
        // 但是这个account不是以0结尾的, 所以为会导致readString失败, 这种
        // 情况, 就把剩余的所有字节作为account了
        account = std::string(pbuf, l);
        ::writelog(InfoType, 
                "get serverlist read account error.%s",
                account.c_str());
        l = 0;
        //msg->appendContent(ANSC::MESSAGE_ERROR);
        //msg->Encrypt(&key[0], key.size());
        //return r->size();
    }
    else
    {
        l -= readlen;
        pbuf += readlen;
    }

    if (l == 2)
    {
        clientversion = ::ReadUint16(pbuf);
        ::writelog(InfoType,
                    "get serverlist read client version:%u.",
                    clientversion);
    }
    else if (l != 0)
    {
        ::writelog("get serverlist length error.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        msg->Encrypt(&key[0], key.size());
        return r->size();
    }
    // ok
    msg->appendContent(ANSC::SUCCESS);
    // count
    uint16_t count;
    const std::list<std::shared_ptr<ServerInfo>> &sl =  ServerInfo::getExtServerlist();
    count = sl.size();
    msg->appendContent(count);
    if (0)//(account == "wqs" || account=="afang007" || account=="liu001" || account=="afang001")
    {
        auto it = sl.begin();
        for (int i = 0; i < count; ++i)
        {
            msg->appendContent((*it)->ip());
            msg->appendContent((*it)->getPort());
        }
    }
#if 0
    else if (account == "afang001" )
    {
        auto it = sl.begin();
        ++it;
        for (int i = 0; i < count; ++i)
        {
            msg->appendContent((*it)->ip());
            msg->appendContent((*it)->getPort());
        }
    }
#endif
    else
    {
        for (auto it= sl.begin(); it != sl.end(); ++it)
        {
            msg->appendContent((*it)->ip());
            msg->appendContent((*it)->getPort());
        }
    }

    msg->Encrypt(&key[0], key.size());

    char buftmp[1024];
    char *pp = buftmp;
    for (uint32_t i = 0; i < key.size(); ++i)
    {
        sprintf(pp, "%02x", uint8_t(key[i]));
        pp += 2;
    }
    *pp = 0;

    ::writelog(InfoType, "get serlist: len: %d, key:%s", msg->contentLen(), buftmp);


    return r->size();
}

