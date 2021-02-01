#include "fcgiEnv.h"

using namespace WXOfficialAcountsServer;
using namespace EncryptAndDecrypt;
using namespace tinyxml2;

int main(int argc, char **argv){
	// 服务器调试信息在当前目录
	::loginit(LOGFILEPATH.c_str());
	logSetPrefix("Wxdeveloper Server");
	string strTmp;
	string alarm;
	char *getenv_ = NULL;
	// 微信公众号和小程序的accessToken每2小时刷新一次
	string accessToken,miniaccesstoken ;
	// read config
	char configpath[1024] = "../build_new/mico.conf";
	Configure *config = Configure::getConfig();

	if (config->read(configpath) != 0)
	{
		::writelog("configure file position wrong!");
		exit(-1);
	}
	// mysql 连接池
	IMysqlConnPool *m_mysqlpool = new MysqlConnPool;
	// 首次获取微信公众号验证token，token有效期2小时，提前5分钟刷新,刷新期间新旧token都有效
	if(0 != WXGetAccessToken(accessToken)){
		::writelog(InfoType,"WXGetAccessToken failed!");
		return -1;
	}
	::writelog(InfoType,"WXaccessToken:%s",accessToken.c_str());
	thread th([&accessToken](){
			while(1){
			sleep(2*60*60-5*60);
			WXGetAccessToken(accessToken);
			}
		});
#if 0
	thread th_get_callback_ip([&accessToken](){
			while(1){
				WXGetCallbackIp(accessToken);
				sleep(24*60*60);
			}
			});
#endif

	// 获取小程序accessToken，原理同上
	if(0 != GetMiniProgramAccessToken(miniaccesstoken)){
		::writelog(InfoType,"GetMiniProgramAccessToken failed!");
		return -1;
	}
	::writelog(InfoType,"WXMiniprogram accessToken:%s",miniaccesstoken.c_str());
	thread th_mini([&miniaccesstoken](){
		while(1){
			sleep(2*60*60-5*60);
			GetMiniProgramAccessToken(miniaccesstoken);
		}
	});

	string strPostData; 
	string strRecvData;
	
	// 自定义菜单
	string host = "https://api.weixin.qq.com/cgi-bin/menu/create?access_token=";
	host += accessToken;
	WXCreateMenu(strPostData);
	HttpsPostData(host, strPostData,strRecvData);
	
	// rabbitmq
	shared_ptr<RabbitMq> m_rabbitmq = make_shared<RabbitMq>(Configure::getConfig()->rabbitmq_addr, "openchannelexchange",\
			"alarmqueue", "alarmrtkey");
	m_rabbitmq->init();
	m_rabbitmq->consume([m_mysqlpool, &miniaccesstoken](const std::string &m){
			NotifyUserWX(m_mysqlpool, miniaccesstoken, m);
			});
	thread mqth([&m_rabbitmq,&alarm](){
		m_rabbitmq->run();
	});

	while(FCGI_Accept() >= 0){
		printf("Content-type:text/html\r\n\r\n");

		strTmp = "";
		getenv_ = NULL;
		struct _WXAuthVerify wxData;
		if((getenv_ = getenv("QUERY_STRING")) == NULL){
			::writelog("getenv query_string is null!");
		}
		else{
			strTmp = string(getenv_);
			// 解析微信公众号服务器发来的数据
			int ret;
			string _3rdSessionKey;
			string phone,miniopenid;
			ret = WXParseAuthVerifyData(strTmp,wxData,_3rdSessionKey,miniopenid,phone);
			if(-1 == ret){					
				::writelog("parse wxData failed!");
				continue;
			}
			else if(!_3rdSessionKey.empty()){
				//::writelog(InfoType,"_3rdSessionKey22:%s",_3rdSessionKey.c_str());
				printf("%s",_3rdSessionKey.c_str());
				continue;
			}
			else if(!phone.empty()){
				// 将手机号和小程序openid存入数据库,插入失败，回复空给小程序，小程序会报错给用户
				if(0 != InsertPhoneMiniOpenidToDb(m_mysqlpool, miniopenid, phone)) {
					::writelog(InfoType, "SetPhoneMiniOpenidToDb failed!");
					printf("");
				}
				else{
					printf("%s",phone.c_str());
					// 小程序发消息给用户,开通报警功能
					MiniProgramNotifyUser(miniaccesstoken,miniopenid,phone);
				}
				continue;
			}
			// 加密鉴权
			else if(!WXValidateSignature(wxData)){		
				::writelog("authority verify failed!");
				continue;
			}
			else{
				if(wxData.echostr != ""){
					printf("%s",wxData.echostr.c_str()); 
				}
			} 
		}

		::writelog(InfoType,"openid:%s",wxData.openid.c_str());
		strTmp = "";
		getenv_ = NULL;
		if((getenv_ = getenv("CONTENT_LENGTH")) == NULL){
			::writelog("getenv content_length is null!");
		}
		else{
			strTmp = string(getenv_);
			// 微信服务器发给开发者的加密消息
			string sPostData;
			// 解密之后的消息
			string sMsg;

			struct _WXPostTextMsg wxPostBackData;
			// 获取微信服务器post内容
			::writelog(InfoType,"strTmp = %s",strTmp.c_str());
			ReadPostBuffer(strTmp,sPostData);
			// 回复微信服务器消息
			// decrypt消息
			::writelog(InfoType, "sPostData= %s", sPostData.c_str());
			WXBizMsgCrypt oWXBizMsgCrypt(TOKEN,SENCODINGAESKEY ,APPID);
			int ret = oWXBizMsgCrypt.DecryptMsg(wxData.msg_signature,wxData.timestamp,wxData.nonce,sPostData,sMsg);
			if( ret != 0 ){
				::writelog(InfoType, "decrypt msg failed! return code is %d", ret);	
				continue;
			}

			unordered_map<string,string> unmap;
			unmap.clear();
			string strXml;
			string sEncryptMsg;
			// 将xml格式消息转换到哈希表容器
			WXPostDataXMLParse(sMsg,unmap);

			// 创建发给微信服务器的消息
			ret = WXCreateMsg(m_mysqlpool,unmap ,strXml);
			if(0 != ret){
				::writelog("create xml data failed!");
				printf("");
				continue;
			}
			// encrypt消息 
			ret = oWXBizMsgCrypt.EncryptMsg(strXml,wxData.timestamp,wxData.nonce,sEncryptMsg);
			if( 0 != ret ){
				::writelog("encrypt msg failed!");
				continue;
			}
			// 消息发给微信服务器
			::writelog(InfoType, "strXml= ",strXml.c_str());
			::writelog(InfoType, "sEncryptMsg= ", sEncryptMsg.c_str());
			
			printf("%s",sEncryptMsg.c_str());
		}
	}
	delete m_mysqlpool;
	m_rabbitmq->quit();
    mqth.join();
	th.join();
	//th_get_callback_ip.join();
	th_mini.join();
	return 0;
}
