#ifndef CLAIM_DEVICE__H
#define CLAIM_DEVICE__H

#include <stdint.h>
#include <vector>
#include <memory>
#include "imessageprocessor.h"
#include "useroperator.h"

class Timer;
class Message;
class IClaimManager;
class ICommand;
class ICache;
class ClaimDevice;

class ClaimDeviceProcessor : public IMessageProcessor
{
public:
    ClaimDeviceProcessor(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    int req(Message *reqmsg, std::list<InternalMessage *> *r);
    int checkPasswordResult(Message *reqmsg, std::list<InternalMessage *> *r);
    int sendCheckPasswordToDevice(const std::shared_ptr<ClaimDevice> &, std::list<InternalMessage *> *r);
    int resendCheckPassword(Message *msg, std::list<InternalMessage *> *r);
    void genNotifyUserClaimResult(uint64_t userid,
        uint64_t devid,
        uint8_t res,
        std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_useroperator;
    IClaimManager *m_claimmanager;
};

class ClaimDevice
{
    enum State { WaitDevicePassword};
    static const int FromDevice = 1;
    static const int FromClient = 2;
public:
    ClaimDevice(uint64_t usrid, uint64_t deviceid, const std::string &pwd) 
        : m_timer(0), m_userID(usrid), m_deviceID(deviceid), m_password(pwd), m_resendtimes(0)
    {
    }
    ~ClaimDevice();

    int resendTimes() { return m_resendtimes;}
    void incResendTimes() {m_resendtimes++;}
    uint64_t userID() {return m_userID;}
    uint64_t deviceID() {return m_deviceID;}
    Timer *timer() {return m_timer;}
    void setTimer(Timer *t) { m_timer = t; }
    const std::string &password() { return m_password; }
protected:
    Timer *m_timer;
    uint64_t m_userID;
    uint64_t m_deviceID;
    std::string m_password;
    int m_resendtimes;

    friend class ClaimDeviceComparable;
    friend class ClaimtimeoutProc;
};

class ClaimDeviceComparable
{
public:
    ClaimDeviceComparable() {}
    bool operator () (const std::shared_ptr<ClaimDevice> &l, const std::shared_ptr<ClaimDevice> &r)
    {
        if (l->m_userID < r->m_userID)
            return true;
        else if (l->m_userID == r->m_userID)
            if (l->m_deviceID < r->m_deviceID)
                return true;
            else
                return false;
        else
            return false;
    }
};

#endif

