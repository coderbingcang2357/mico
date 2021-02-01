#ifndef CGIENV_H
#define CGIENV_H
#include "common.h"
namespace WXOfficialAcountsServer{

// 接收微信服务器返回的json数据
static const int RSP_BUF_LEN = 40960;

// 微信公众号开发者账号密码

// 测试号
//static const string APPID = "wxa7c74a5ae2eb378b";
//static const string APPSECRET = "eab58d964c96fbb8e16c4b7c4ccb98ea";
// 日志文件位置
static const string LOGFILEPATH = "/var/log/mico/fcgi/fcgi.log";
// 合信云平台
static const string APPID = "wxac6aa86d51459bba";
static const string APPSECRET = "ffc4de8bc2ad72dfea26616dc77274df";

// 合信及时通小程序
static const string APPID_INTIME = "wxa3376a8cbea27476";
static const string APPSECRET_INTIME= "04f74a356d8309896e988184d29f6ab0";
static const string TEMPLATE_ID_PHONE_VALID = "i9JhASnLEC9KGqT1i8VmAdEeMpuaZU7k6RtJWwx3o7Q";

// 微信服务器回调开发者的ip,需要添加到ip白名单中,每天更新一次,避免单点故障
static const vector<string> IPLIST ;
static const string TEMPLATE_ID_PRODUCT = "QOIFq3AcMNcCS_uxo7BQNCc0NT96f1IWzDN8hTu_9BU";
static const string TEMPLATE_ID_RELEASE = "MuSBN0hKqQjjfFRIErb_ig96wSkUxELomrdBaJKkPWY";
static const string TEMPLATE_PRODUCT_REMARK = "请及时处理报警信息！";
static const string TEMPLATE_RELEASE_REMARK = "感谢您的使用！";
//static const string OPEN_ID = "";
// 下面2项和微信公众开发者设置内容一致
static const string TOKEN = "wubcToken";
static const string SENCODINGAESKEY = "mDsbCO8o70YdXCNx7byNyCUNqU7501lIx0oVP2dCKa7";

// error code
static const int MYSQL_QUERY_NO_CONTENT = -100;
static const int ERROR_FORMAT_PHONENUM = -101;

// 小程序sessionkey过期时间600秒
static const uint32_t SESSIONKEYTIMEOUT = 600;

typedef struct _WXAuthVerify{
	string timestamp;
	string nonce;
	string msg_signature;
	string signature;
	string echostr;
	string openid;
	string encrypt;
}WXAuthVerify;

typedef struct _WXPostTextMsg{
	string ToUserName;
	string FromUserName;
	string CreateTime;
	string MsgType;
	string Content;
	//string MsgId;
}wxPostTextMsg;

bool WXValidateSignature(struct _WXAuthVerify &data);

int GetOpenidSessionKey(string code,string &get_content);

int Get3rdSessionKeyData(string strIn,string &strOut);

int ParseGetDataFromHttp(const string &getData,unordered_map<string,string> &getDataPair);

int GetPhoneData(string sData,string &phoneNumber);

int DecryptUserPhoneData(string &encryData,string &session_key,string &ivData, string &phone);

void GenerateJsonString(string &_3rdSessionKey);

int Set3rdSessionKeyInRedis(string sessionKey, string openid, string &_3rdSessionKey,uint32_t timeout);

int FindSessionKeyInRedis(string _3rdSessionKey,string &_sessionKey,string &openid);

//void create_weapp_template_msg(string &weapp_id);

void CreateMiniProgramMsg(string openid,string phone, string &msg);

int GetMiniProgramAccessToken(string &accessToken);

int MiniProgramNotifyUser(string miniaccesstoken,string openid,string phone);

int InsertPhoneMiniOpenidToDb(IMysqlConnPool *m_mysqlpool, const string &miniopenid, const string &phone); 

int WXParseAuthVerifyData(const string &getData,struct _WXAuthVerify &data,string &_3rdSessionKey,string &miniopenid,string &phone);

int ReadPostBuffer(const string &getData,string &strBuff);

int SetOneFieldToXml(tinyxml2::XMLDocument * pDoc, tinyxml2::XMLNode* pXmlNode, const char * pcFieldName,
		const std::string  &value, bool bIsCdata);

int CreateXmlMsgStr(const unordered_map<string,string> &unmap, string &replyMsg);

bool VerifyPhoneNum(string phone);

int LookupPhoneNumIndb(IMysqlConnPool *m_mysqlpool,string &openid,string phone);

int WXPostDataXMLParse(const string &buffer,unordered_map<string,string> &unmap);

string GetTimeOfNow();

int WXCreateMenu(string &jsonStr);

void VerifyPhoneNumInDB(IMysqlConnPool *m_mysqlpool,unordered_map<string,string> &unmap,string &replyContent);

string GetLinuxTime();

int WXCreateMsg(IMysqlConnPool *m_mysqlpool,unordered_map<string,string> &unmap, string &strXml);

void WXCreateTemplateMsg(const string &openid, const string &alarmMsg, const string &scenename,
		const string &sceneowner, const string &alarmId, uint8_t alarmEventType, const string &alarmTime, string *template_msg);

void GetIp(const string &domain, string *ip);

// https加密协议,端口443
int SocketHttps(const string &host, const string &request, string &response, const string &domain="api.weixin.qq.com");

int HttpsPostData(const string &host, string &post_content,string &post_response);

int HttpsGetData(const string &host, string &get_content);

int WXGetAccessToken(string &accessToken);

void gettimestr(char *buf, int sizeofbuf);

void WXGetCallbackIp(const string &accessToken); 

int ParseAlarmMsgFromJson(const string &msg, uint64_t *sceneid, string *alarmid, 
		string *alarmText, string *alarmTime, uint8_t *alarmEventType);

int FindOpenidWithSceneid(IMysqlConnPool *m_mysqlpool, uint64_t sceneid, string *openid); 

int FindSceneNameWithSceneid(IMysqlConnPool *m_mysqlpool, uint64_t sceneid, string *scenename); 

int FindSceneOwnerWithSceneid(IMysqlConnPool *m_mysqlpool, uint64_t sceneid, string *sceneowner); 

int NotifyUserWX(IMysqlConnPool *m_mysqlpool,const string &accessToken, const string &alarmMsg);

}
#endif
