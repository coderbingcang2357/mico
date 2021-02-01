#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <cassert>
#include <strings.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "BusinessManager.h"
//#include "../ShareMem/ShareMem.h"
#include "config/dbconfig.h"
#include "util/logwriter.h"
#include "config/configreader.h"
#include "msgqueue/ipcmsgqueue.h"

BusinessManager::BusinessManager()
{

}

BusinessManager::~BusinessManager()
{

}

int BusinessManager::Init()
{
    //ShareMem::Init();
    if (::createMsgQueue(Ext_Logical) != 0)
    {
        ::writelog("create msgqueue failed.");
        exit(0);
    }

    int sock = socket (PF_INET, SOCK_DGRAM, 0);
    assert (sock >= 0);

    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);

    int sendbuflen = 2 * 1024 * 1024;
    int r =::setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&sendbuflen, sizeof(sendbuflen));
    if (r != 0)
    {
        writelog("set buf error.");
        exit(3);
    }

    Configure *conf = Configure::getConfig();
    //conf->extconn_srv_address;

    sockaddr_in localAddr;
    bzero (&localAddr, sizeof (localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(conf->extconn_srv_port);
    inet_pton (AF_INET, "localhost", &localAddr.sin_addr);
    //inet_pton (AF_INET,
    //        conf->extconn_srv_address.c_str(),
    //        &localAddr.sin_addr);

    int ret = bind (sock, (sockaddr*) &localAddr, sizeof (localAddr));
    assert (ret >= 0);
    (void)ret;  // for unused warning

    // 从客户端收到消息时, 把消息转发到时LogicalSrv
    m_msgInput.Init(::msgqueueExtserverToLogical(), sock);
    // 从LogicalSrv收到消息时把它发到客户端
    m_msgOutput.Init(::msgqueueLogicalToExtserver(), sock);

    return sock;
}

// 处理从客户端收到的消息
int BusinessManager::HandleMsgInput()
{
    // 从socket接收消息, 然后发到posix消息队列中
    return m_msgInput.UdpRecv();
}

// 处理从LogicalSvr收到的消息, 把它发到客户端
int BusinessManager::HandleMsgOutput()
{
    // 从共享内存中读数据
    int cnt = 0;
    while (cnt < MAX_RECVIVE_MESSAGES)
    {
        if (m_msgOutput.Read() != 0)
            break;
        // 把数据发到客户端
        m_msgOutput.UdpSend();

        cnt++;
    }
    return cnt;
}

