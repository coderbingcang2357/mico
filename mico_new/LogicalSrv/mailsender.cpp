#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <curl/curl.h>

#include "mailsender.h"

using std::string;

namespace MAIL
{
    //邮件（主题、内容）
    const char* const SUBJECT_ACTIVATION = "注册确认（合信云平台）";
    const char* const CONTENT_ACTIVATION =
    "<p> 亲爱的%s：</p> \
        <p>您好！感谢您注册合信云平台！请在客户端输入验证码进行激活。</p> \
        <p><br></p> \
        <p>验证码：%s</p> \
        <p>（验证码将在您注册后失效）</p> \
        <p><br></p> \
        <p>本邮件由系统自动发出，请勿直接回复！</p><p>合信自动化</p> \
        <p><a href=\"http://www.co-trust.com\">www.co-trust.com</a></p>";

    const char* const SUBJECT_RESET_PWD = "密码修改（合信云平台）";
    const char* const CONTENT_RESET_PWD =
    "<p> 亲爱的%s：</p> \
        <p>您好！请在客户端输入验证码，确认修改。</p> \
        <p><br></p> \
        <p>验证码：%s</p> \
        <p>（验证码将在30分钟后失效）</p> \
        <p>设置密码请尽量遵循以下标准：</p> \
        <p>·包含大小写字母、数字和符号</p> \
        <p>·不少于10位</p> \
        <p>·不包含生日、手机号码等易被猜出的信息</p> \
        <p><br></p> \
        <p>本邮件由系统自动发出，请勿直接回复！</p> \
        <p>合信自动化</p> \
        <p><a href=\"http://www.co-trust.com\">www.co-trust.com</a></p>";

    const char* const SUBJECT_DEL_DEV_CLUSTER = "设备群删除（合信云平台）";
    const char* const CONTENT_DEL_DEV_CLUSTER =
    "<p> 亲爱的%s：</p> \
        <p>您好！请在客户端输入验证码，确认删除。</p> \
        <p><br></p> \
        <p>验证码：%s</p> \
        <p>（验证码将在您删除群后失效）</p> \
        <p><br></p> \
        <p>本邮件由系统自动发出，请勿直接回复！</p> \
        <p>合信自动化</p> \
        <p><a href=\"http://www.co-trust.com\">www.co-trust.com</a></p>";

    const char* const SUBJECT_MODIFY_MAILBOX = "邮箱地址修改（合信云平台）";
    const char* const CONTENT_MODIFY_MAILBOX =
    "<p> 亲爱的%s：</p> \
        <p>您好！请在客户端输入验证码，确认修改。</p> \
        <p><br></p> \
        <p>验证码：%s</p> \
        <p>（验证码将在30分钟后失效）</p> \
        <p><br></p> \
        <p>本邮件由系统自动发出，请勿直接回复！</p> \
        <p>合信自动化</p> \
        <p><a href=\"http://www.co-trust.com\">www.co-trust.com</a></p>";

}


//构造
MailSender::MailSender()
{
    m_apiUser = "postmaster@co-trust.sendcloud.org";
    m_apiPwd = "BtLX3ZgjIYaN3eFp";
    m_from = "noreply@itmg.co-trust.com";//liaowenhui@co-trust.com";
    m_fromName = "合信自动化";
    m_toAccount = "用户";
}

//析构
MailSender::~MailSender()
{

}

//设置邮件类型
void MailSender::setMailType (MailSender::EMType emType)
{
    m_emType = emType;
}

//设置目标邮箱
void MailSender::setTargetMailBox (string mailBox)
{
    m_to = mailBox;
}

//设置目标帐号
void MailSender::setTargetAccount (string account)
{
    m_toAccount = account;
}

//设置验证码
void MailSender::setVerifCode (const std::string &verifCode)
{
    m_verifCode = verifCode;
}

//////////////////////////////////////////////////////////////////
struct RetString{ char *ptr;size_t len;};
void init_string(RetString *rs)
{
    rs->len = 0;
    rs->ptr = (char*) malloc(rs->len + 1);
    if (rs->ptr == NULL){
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    rs->ptr[0] = '\0';
}
size_t writefunc(void *ptr, size_t size, size_t nmemb, RetString *s)
{
   size_t new_len = s->len + size*nmemb;
   s->ptr = (char*) realloc(s->ptr, new_len+1);
   if (s->ptr == NULL){
          fprintf(stderr, "realloc() failed\n");
          exit(EXIT_FAILURE);
    }
    memcpy(s->ptr+s->len, ptr, size*nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;
    return size*nmemb;
}
//////////////////////////////////////////////////////////////////

//发送邮件
int MailSender::send()
{
    CURL *curl;
    CURLcode res;

    // 返回信息
    RetString ret_str;
    init_string (&ret_str);

    curl_global_init (CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl != NULL) {
        curl_easy_setopt (curl, CURLOPT_URL, "https://sendcloud.sohu.com/webapi/mail.send.json");
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        // time out 30 s
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        /*转义*/       

        char content[1024] = {0};
        switch(m_emType) {
            case EM_Activation: {
                m_subject = MAIL::SUBJECT_ACTIVATION;
                //std::cout << m_subject << std::endl;
                sprintf(content,
                    MAIL::CONTENT_ACTIVATION,
                    m_toAccount.c_str(),
                    m_verifCode.c_str());
                //std::cout << content << std::endl;
                break;
            }
            case EM_ResetPwd: {
                m_subject = MAIL::SUBJECT_RESET_PWD;
                //std::cout << m_subject << std::endl;
                sprintf(content,
                    MAIL::CONTENT_RESET_PWD,
                    m_toAccount.c_str(),
                    m_verifCode.c_str());
                //std::cout << content << std::endl;
                break;
            }
            case EM_ModifyMailBox: {
                m_subject = MAIL::SUBJECT_MODIFY_MAILBOX;
                //std::cout << m_subject << std::endl;
                sprintf(content,
                    MAIL::CONTENT_MODIFY_MAILBOX,
                    m_toAccount.c_str(),
                    m_verifCode.c_str());
                //std::cout << content << std::endl;
                break;                
            }
            case EM_DelDevGrp: {
                m_subject = MAIL::SUBJECT_DEL_DEV_CLUSTER;
                //std::cout << m_subject << std::endl;
                sprintf(content,
                    MAIL::CONTENT_DEL_DEV_CLUSTER,
                    m_toAccount.c_str(),
                    m_verifCode.c_str());
                //std::cout << content << std::endl;
                break;
            }
            default:
                break;
        }
        char *fromName = curl_easy_escape (curl, m_fromName.c_str(), 0);
        char *subject = curl_easy_escape (curl, m_subject.c_str(), 0);        
        char *html = curl_easy_escape (curl, content, 0);

        string postFileds = "api_user=";
        postFileds += m_apiUser;
        postFileds += "&api_key=";
        postFileds += m_apiPwd;
        postFileds += "&from=";
        postFileds += m_from;
        postFileds += "&to=";
        postFileds += m_to;
        postFileds += "&fromname=";
        postFileds += fromName;
        postFileds += "&subject=";
        postFileds += subject;
        postFileds += "&html=";
        postFileds += html;

        //std::cout << postFileds << std::endl;

        curl_easy_setopt (curl, CURLOPT_POSTFIELDS, postFileds.c_str());

        // 获取返回
        curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt (curl, CURLOPT_WRITEDATA, &ret_str);

        res = curl_easy_perform (curl);
        if (res != CURLE_OK)
            fprintf (stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror (res));
        else
            printf ("%s\n", ret_str.ptr);

        free (ret_str.ptr);
        curl_free(fromName);
        curl_free(subject);
        curl_free(html);

        curl_easy_cleanup (curl);
    }

    curl_global_cleanup();

    return 0;
}

