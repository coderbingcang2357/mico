#ifndef MAILSENDER__H
#define MAILSENDER__H

#include <stdint.h>
#include <string>

class MailSender
{
public:
    MailSender();
    ~MailSender();

    enum EMType{EM_Activation, EM_ResetPwd, EM_ModifyMailBox, EM_DelDevGrp};

    void setMailType(EMType emType);
    void setTargetMailBox(std::string mailBox);
    void setTargetAccount(std::string account);
    void setVerifCode(const std::string &verifCode);

    int send();
    //std::string Test();
private:
    EMType m_emType;

    std::string m_apiUser;
    std::string m_apiPwd;
    std::string m_from;
    std::string m_fromName;
    std::string m_to;
    std::string m_toAccount;
    std::string m_subject;

    std::string m_verifCode;
};

#endif
