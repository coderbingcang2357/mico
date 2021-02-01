#include "fcgiEnv.h"
using namespace rapidjson;
using namespace tinyxml2;
using namespace EncryptAndDecrypt;
using namespace WxBizDataSecure;
namespace WXOfficialAcountsServer{
#define DELETE_PTR(ptr) \
	if (NULL != (ptr)) {\
		delete (ptr);\
		(ptr) = NULL;\
	}
bool WXValidateSignature(struct _WXAuthVerify &data){
	// 加密算法会生成20位字符
	vector<string> svec;
	string token = TOKEN;   
	svec.push_back(token);
	svec.push_back(data.timestamp);
	svec.push_back(data.nonce);

	sort(svec.begin(),svec.end());
	string sStr = svec[0]+svec[1]+svec[2];

	//compute
	unsigned char output[SHA_DIGEST_LENGTH] = { 0 };
	if( NULL == SHA1( (const unsigned char *)sStr.c_str(), sStr.size(), output ) )
	{
		::writelog("sha1 is null!");
		return false;
	}

	// to hex
	string sSignature;
	char tmpChar[ 8 ] = { 0 };

	for( int i = 0; i < SHA_DIGEST_LENGTH; i++ )
	{
		snprintf( tmpChar, sizeof( tmpChar ), "%02x", 0xff & output[i] );
		sSignature.append( tmpChar );
	}


	if(data.signature != sSignature){
		::writelog("data.signature != sSignature error!");
		return false;
	}

	return true;
}

int GetOpenidSessionKey(string code,string &get_content)
{
	string cod = code;
	string url = "https://api.weixin.qq.com/sns/jscode2session?appid=";
	//小程序appid
	url += APPID_INTIME;
	url += "&secret=";
	//小程序密钥
	url += APPSECRET_INTIME;
	url += "&js_code=";
	url += cod;
	url += "&grant_type=authorization_code";
	stringstream stream;
	stream << "GET " << url;
	stream << " HTTP/1.0\r\n";
	stream << "Host: " << "api.weixin.qq.com" << "\r\n";
	stream <<"User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\n";
	stream <<"Connection:close\r\n\r\n";

	if(0 != SocketHttps(url, stream.str(),get_content)){
		::writelog(InfoType,"SocketHttps failed!");
		return -1;
	}
	return 0;
}

int Get3rdSessionKeyData(string strIn,string &sessionKey,string &openid){

	int begPos,endPos;
	begPos = strIn.find("{");
	endPos = strIn.find("}");
	//sessionKey = strIn.substr(begPos,endPos-begPos+1);
#if 1
	string strJson = strIn.substr(begPos,endPos-begPos+1);
	Document doc;
	doc.Parse(strJson.c_str());
	if(!doc.IsObject()){
		::writelog(InfoType,"strJson not json string!");
		return -1;
	}

	bool valid = doc.HasMember("session_key") && doc.HasMember("openid");
	if(!valid){
		::writelog("doc.HasMember(\"session_key\") && doc.HasMember(\"openid\") not valid!");
		return -1;
	}
	sessionKey = doc["session_key"].GetString();
	openid = doc["openid"].GetString();
#endif
	return 0;
}	

int ParseGetDataFromHttp(const string &getData,unordered_map<string,string> &getDataPair){
	if(getData == ""){
		::writelog("GET no data!");
		return -1;
	}	
	::writelog(InfoType,"GET:%s",getData.c_str());
	size_t startPos = 0;
	size_t endPos = getData.find_first_of("=");
	string key,value;

	while(endPos != string::npos){
		key = getData.substr(startPos,endPos-startPos);
		startPos = endPos+1;
		endPos = getData.find_first_of("&",startPos);
		value = getData.substr(startPos,endPos-startPos);
		getDataPair.insert({key,value});

		if(endPos == string::npos){
			break;
		}

		startPos = endPos+1;
		endPos = getData.find_first_of("=",startPos);
	} 

	return 0;
}

int GetPhoneData(string sData,string &phoneNumber){
	
    Document doc;
    doc.Parse(sData.c_str());
	if(!doc.IsObject()){
		::writelog(InfoType,"sData not json string!");
		return -1;
	}

	string purePhoneNumber,coutryCode,appid;
	uint64_t timeStamp;
	rapidjson::Value watermark;
	bool valid = doc.HasMember("phoneNumber") && doc.HasMember("purePhoneNumber") && doc.HasMember("countryCode") 
		&& doc.HasMember("watermark");
	if(!valid){
		::writelog("doc.HasMember(\"phoneNumber\") && doc.HasMember(\"purePhoneNumber\") \
				&& doc.HasMember(\"countryCode\") && doc.HasMember(\"watermark\") not valid!");
		return -1;
	}
	phoneNumber = doc["phoneNumber"].GetString();
	purePhoneNumber = doc["purePhoneNumber"].GetString();
	coutryCode = doc["countryCode"].GetString();
	watermark = doc["watermark"];

	valid = watermark.HasMember("timestamp") && watermark.HasMember("appid"); 
	if(!valid){
		::writelog("watermark.HasMember(\"timestamp\") && watermark.HasMember(\"appid\") not valid!");
		return -1;
	}
	timeStamp = watermark["timestamp"].GetUint64();
	appid = watermark["appid"].GetString();
	::writelog(InfoType,"phoneNumber:%s",phoneNumber.c_str());
	::writelog(InfoType,"appid:%s",appid.c_str());
	return 0;
}
int DecryptUserPhoneData(string &encryData,string &session_key,string &ivData,string &phone){

	string sData;
	string sEncryptData = UrlDecode(encryData);
	//string sSessionKey = UrlDecode(session_key);
	string sIvData = UrlDecode(ivData);

	WXBizDataCrypt oWXBizDataCrypt(APPID_INTIME, session_key);

	int ret = oWXBizDataCrypt.DecryptData(sEncryptData,sIvData,sData );
	::writelog(InfoType, "DecryptData ret: %d size: %d data: %s", ret, sData.size(), sData.c_str());
	return GetPhoneData(sData,phone);
}

void GenerateJsonString(string &_3rdSessionKey){
	string tmp;
	tmp += "{";
	tmp += "\"_3rdSessionKey\"";
	tmp += ":";
	tmp += "\"";
	tmp += _3rdSessionKey;
	tmp += "\"";
	tmp += "}";
	_3rdSessionKey = tmp;
}
// 生成128位随机数，并以键值对存入redis，设置10分钟有效期
int Set3rdSessionKeyInRedis(string sessionKey,string openid,string &_3rdSessionKey,uint32_t timeout){

	random_device rd("/dev/urandom");
	uint32_t i1 = rd();
	uint32_t i2 = rd();
	uint32_t i3 = rd();
	uint32_t i4 = rd();
	_3rdSessionKey += to_string(i1);
	_3rdSessionKey += to_string(i2);
	_3rdSessionKey += to_string(i3);
	_3rdSessionKey += to_string(i4);
	string value = sessionKey+" "+openid;

	shared_ptr<RedisConnPool> pool = RedisConnPool::get_instance();
	pool->Set(_3rdSessionKey, value);
	pool->Expire(_3rdSessionKey, timeout);

	// 生成json格式传给小程序
	GenerateJsonString(_3rdSessionKey);
	::writelog(InfoType,"_3rdSessionKey111:%s",_3rdSessionKey.c_str());

	return 0;
}

int FindSessionKeyInRedis(string _3rdSessionKey,string &_sessionKey,string &openid){

	shared_ptr<RedisConnPool> pool = RedisConnPool::get_instance();
	string  result;
	pool->Get(_3rdSessionKey, result);
	::writelog(InfoType,"result:%s",result.c_str());
	if(result.empty()){
		::writelog("result is empty!");
		return -1;
	}
	int pos = result.find(" ");
	_sessionKey = result.substr(0,pos);
	openid = result.substr(pos+1);
	return 0;
}

#if 0
void create_weapp_template_msg(string &weapp_id){
	StringBuffer buf;
	PrettyWriter<StringBuffer> writer(buf); // it can word wrap
	writer.StartObject();
	writer.Key("template_id"); writer.String(TEMPLATE_ID_PHONE_VALID.c_str());
	writer.Key("page"); writer.String("pages/index/index");
	writer.Key("form_id"); writer.String();
	
	writer.Key("data");
	writer.StartObject();
	writer.key("keyword1");
	writer.StartObject();
	writer.Key("value"); writer.String("3333333");
	writer.EndObject();
	writer.EndObject();
	writer.Key("emphasis_keyword"); writer.String("keyword1.DATA");
	writer.EndObject();
	const char *json_content = buf.GetString();
	weapp_id.append(json_content);
	
}
#endif
void CreateMiniProgramMsg(string openid, string phone, string &msg){
	StringBuffer buf;
	PrettyWriter<StringBuffer> writer(buf); // it can word wrap
	writer.StartObject();
	writer.Key("touser"); writer.String(openid.c_str());
	writer.Key("template_id"); writer.String(TEMPLATE_ID_PHONE_VALID.c_str());
	writer.Key("page"); writer.String("pages/index/index");
	writer.Key("miniprogram_state"); writer.String("");
	writer.Key("lang"); writer.String("zh_CN");
	
	writer.Key("data");
	writer.StartObject();
	writer.Key("time1"); 
	writer.StartObject();
	string now = GetTimeOfNow();
	writer.Key("value"); writer.String(now.c_str());
	writer.EndObject();
	writer.Key("phone_number2");
	writer.StartObject();
	writer.Key("value"); writer.String(phone.c_str());
	writer.EndObject();
	writer.Key("thing3");
	writer.StartObject();
	string thing = "远程报警通知功能开启成功!";
	writer.Key("value"); writer.String(thing.c_str());
	writer.EndObject();
	writer.EndObject();

	writer.EndObject();
	const char *json_content = buf.GetString();
	msg.append(json_content);
}

int GetMiniProgramAccessToken(string &accessToken){
	string host = "https://api.weixin.qq.com/cgi-bin/token?grant_type=client_credential&";
	host += "appid=";
	host += APPID_INTIME;
	host += "&secret=";
	host += APPSECRET_INTIME;

	string  accessTokenJson,accessTokenReply;
	if(0 != HttpsGetData(host,accessTokenReply)){
		::writelog(InfoType,"HttpsGetData miniprogram accessToken failed!");
		return -1;
	}
	int begPos,endPos;
	begPos = accessTokenReply.find("{");
	endPos = accessTokenReply.find("}");
	accessTokenJson = accessTokenReply.substr(begPos,endPos-begPos+1);
	::writelog(InfoType,"accessTokenJson:%s",accessTokenJson.c_str());
	Document doc;
	doc.Parse(accessTokenJson.c_str());
	bool valid = doc.HasMember("access_token");
	if(!valid){
		::writelog(InfoType,"accessToken get failed!");
		return -1;
	}
	if(!doc.IsObject()){
		::writelog(InfoType,"accessTokenJson parse failed!");
		return -1;
	}

	accessToken = doc["access_token"].GetString();
	return 0;
}

int MiniProgramNotifyUser(string miniaccesstoken,string openid, string phone){
	string msg,response;
	CreateMiniProgramMsg(openid, phone, msg);
	// 发给小程序用户的消息
	string host = "https://api.weixin.qq.com/cgi-bin/message/subscribe/send?access_token=";
	::writelog(InfoType,"miniaccesstoken:%s",miniaccesstoken.c_str());
	host += miniaccesstoken;
	if(0 != HttpsPostData(host,msg,response)){
		::writelog(InfoType,"HttpsPostData(host,msg,response) failed!");
		return -1;
	}
	::writelog(InfoType,"response msg:%s",response.c_str());

	//NotifyUserWX1(miniaccesstoken, openid, TEMPLATE_ID);
	return 0;
}

int InsertPhoneMiniOpenidToDb(IMysqlConnPool *m_mysqlpool, const string &miniopenid, const string &phone) {
#if 0
	// 查询手机号是否已经在mico客户端注册
	auto sql = m_mysqlpool->get();
	string query = FormatSql("select wechartphone from runningscene where wechartphone = %s", phone.c_str());
	int ret = sql->Select(query);
	if(ret == 0){
		
	}
	else if(ret == 1){
		//::writelog(InfoType,"mysql查询结果为空!");
		::writelog(InfoType, "phone not register in mico!");
		return -1;
	}
	else{
		::writelog(InfoType," select phone from runningscene failed!");
		return -1;
	}
#endif
	auto sql = m_mysqlpool->get();
	string query = FormatSql("select phone from phonewechartid where phone = %s", phone.c_str());
	int ret = sql->Select(query);
	if(ret == 0){
		auto res = sql->result();
		int c = res->rowCount();
		::writelog(InfoType,"c:%d",c);
	}
	else if(ret == 1){
		//::writelog(InfoType,"mysql查询结果为空!");
		query = FormatSql("insert into phonewechartid(phone,wechartid) values(\"%s\",\"%s\")", phone.c_str(),miniopenid.c_str());
		ret = sql->Insert(query);
		if(ret != 0){
			::writelog(InfoType,"insert phone ,miniopenid into  phonewechartid failed!");
			return -1;
		}
	}
	else{
		::writelog(InfoType," select phone from phonewechartid failed!");
		return -1;
	}
	return 0;
}

// 参数_3rdSessionKey只有在服务器和小程序交互获取用户手机号时有意义 
int WXParseAuthVerifyData(const string &getData,struct _WXAuthVerify &data,string &_3rdSessionKey,string &miniopenid,string &phone){
	unordered_map<string,string> getDataPair;

	if(-1 == ParseGetDataFromHttp(getData, getDataPair)){
		::writelog(InfoType,"ParseGetDataFromHttp failed!");
		return -1;	
	}

	::writelog("getDataPair content:");
	for(unordered_map<string,string>::iterator it = getDataPair.begin();it != getDataPair.end();++it){
		cout<<it->first<<"\t"<<it->second<<endl; 
	}

	// 用户使用小程序,开发者服务器回复小程序sessionkey
	string get_content;
	if(getDataPair.find("code") != getDataPair.end()){
		if(0 == GetOpenidSessionKey(getDataPair["code"],get_content)){
			string sessionKey,openid;
			if(0 != Get3rdSessionKeyData(get_content,sessionKey,openid)){
				::writelog(InfoType,"Get3rdSessionKeyData failed!");
				return -1;
			}	
			//::writelog(InfoType,"_3rdSessionKey333:%s",_3rdSessionKey.c_str());
			Set3rdSessionKeyInRedis(sessionKey, openid, _3rdSessionKey, SESSIONKEYTIMEOUT);
			return 0;
		}
		else{
			::writelog(InfoType,"GetOpenidSessionKey failed!");
			return -1;
		}
	}
	// 小程序将手机号等加密数据发给开发者服务器
	if(getDataPair.find("encrydata") != getDataPair.end() && getDataPair.find("sessionkey") != getDataPair.end()
			&& getDataPair.find("ivdata") != getDataPair.end()){
		// 查询sessionkey是否存在开发者服务器中
		string _sessionKey;
		//::writelog(InfoType,"222sessionkey:%s",getDataPair["sessionkey"].c_str());
		if(0 != FindSessionKeyInRedis(getDataPair["sessionkey"], _sessionKey,miniopenid)){
			::writelog(InfoType,"sessionkey not in redis");
			return -1;
		}
		::writelog(InfoType,"miniopenidddddd:%s",miniopenid.c_str());
		//::writelog(InfoType,"33333:%s",_sessionKey.c_str());
		// 解析加密数据
		if(0 != DecryptUserPhoneData(getDataPair["encrydata"], _sessionKey, getDataPair["ivdata"],phone)){
			::writelog(InfoType,"DecryptUserPhoneData failed!");
			return -1;
		}
		return 0;
	}

	if((getDataPair.find("timestamp") == getDataPair.end())
			||(getDataPair.find("nonce") == getDataPair.end())
			||(getDataPair.find("signature") == getDataPair.end())){
		//||(getDataPair.find("echostr") == getDataPair.end())){
		::writelog("field parse failed");
		return -1;
	}
	else{
		data.timestamp = getDataPair["timestamp"];	
		data.nonce = getDataPair["nonce"];	
		data.signature = getDataPair["signature"];	
	}
	if(getDataPair.find("echostr") != getDataPair.end()){
		data.echostr = getDataPair["echostr"];	
	}
	if(getDataPair.find("openid") != getDataPair.end()){
		data.openid = getDataPair["openid"];
	}
	// 加密消息才有此关键字
	if((getDataPair.find("msg_signature") != getDataPair.end())){
		data.msg_signature = getDataPair["msg_signature"];	
	}

	return 0;
}

int ReadPostBuffer(const string &getData,string &strBuff)
{
	char 		 *buffer = NULL;
	uint32_t     post_len ;
	uint32_t     read_remain_len ;
	uint32_t     read_len ;
	uint32_t     len ;
	if( getData == "" )
	{
		::writelog("no post data");
		return -1;
	}
	post_len = stoi(getData) ;
	//::writelog(InfoType,"==========post_len %d",post_len);
	buffer = new char[post_len]{0};
	if(buffer == NULL){
		::writelog("buffer new failed");
		return -1;
	}
	//memset(buffer ,0,post_len);

	read_remain_len = post_len ;
	read_len = 0 ;
	while( read_remain_len > 0 )
	{
		len = FCGI_fread( buffer + read_len , 1 , read_remain_len , FCGI_stdin ) ;
		if( len == 0 )
			break;
		read_remain_len -= len ;
		read_len += len ;
	}

	strBuff.append(buffer,read_len).append(1,'\0');
	delete buffer;
	return 0;
}

#if 0
// only for test!!!
int CreateTextXml222(struct _WXPostTextMsg &wxPostBackData, tinyxml2::XMLDocument &doc){
	//const char *xmlFile = "/data/web/textReply.xml";
	//tinyxml2::xmlDeclaration *decl = new tinyxml2::xmlDeclaration("1.0","","");
	tinyxml2::XMLElement *root = doc.NewElement("xml");
	doc.InsertEndChild(root);

	tinyxml2::XMLElement *toUser = doc.NewElement("ToUserName");
	tinyxml2::XMLText *text1 = doc.NewText(wxPostBackData.ToUserName.c_str());
	toUser->InsertEndChild(text1);
	root->InsertEndChild(toUser);

	tinyxml2::XMLElement *fromUser = doc.NewElement("FromUserName");
	tinyxml2::XMLText *text2 = doc.NewText(wxPostBackData.FromUserName.c_str());
	toUser->InsertEndChild(text2);
	root->InsertEndChild(fromUser);

	tinyxml2::XMLElement *createTime = doc.NewElement("CreateTime");
	tinyxml2::XMLText *text3 = doc.NewText(wxPostBackData.CreateTime.c_str());
	toUser->InsertEndChild(text3);
	root->InsertEndChild(createTime);

	tinyxml2::XMLElement *msgType = doc.NewElement("MsgType");
	tinyxml2::XMLText *text4 = doc.NewText(wxPostBackData.MsgType.c_str());
	toUser->InsertEndChild(text4);
	root->InsertEndChild(msgType);

#if 0
	tinyxml2::XMLElement *msgId = doc.NewElement("MsgId");
	tinyxml2::XMLText *text5 = doc.NewText(wxPostBackData.MsgId.c_str());
	toUser->InsertEndChild(text5);
	root->InsertEndChild(msgId);
#endif

	tinyxml2::XMLElement *content = doc.NewElement("Content");
	tinyxml2::XMLText *text6 = doc.NewText(wxPostBackData.Content.c_str());
	toUser->InsertEndChild(text6);
	root->InsertEndChild(content);

	//doc.LinkEndChild(decl);
	//doc.SaveFile(xmlFile);
	return 0; 
}
#endif

#if 1
int SetOneFieldToXml(tinyxml2::XMLDocument * pDoc, tinyxml2::XMLNode* pXmlNode, const char * pcFieldName,
		const std::string  &value, bool bIsCdata)
{
	if(!pDoc || !pXmlNode || !pcFieldName)
	{
		::writelog("pDoc or pxmlNode or pcFieldName is NULL");
		return -1;
	}

	tinyxml2::XMLElement * pFiledElement = pDoc->NewElement(pcFieldName);
	if(NULL == pFiledElement)
	{
		::writelog("pFiledElement is NULL");
		return -1;
	}

	tinyxml2::XMLText * pText = pDoc->NewText(value.c_str());
	if(NULL == pText)
	{
		::writelog("pText is NULL");
		return -1;
	}

	pText->SetCData(bIsCdata);
	pFiledElement->LinkEndChild(pText);

	pXmlNode->LinkEndChild(pFiledElement);
	return 0;
}
#endif

int CreateXmlMsgStr(const unordered_map<string,string> &unmap, string &replyMsg){
	tinyxml2::XMLPrinter oPrinter;
	tinyxml2::XMLNode* pXmlNode = NULL;
	tinyxml2::XMLDocument * pDoc = new tinyxml2::XMLDocument();
	if(NULL == pDoc)
	{
		::writelog("xml node is NUll 1");
		return -1;
	}

	pXmlNode = pDoc->InsertEndChild( pDoc->NewElement( "xml" ) );
	if(NULL == pXmlNode)
	{
		::writelog("xml node is NUll 2");
		DELETE_PTR(pDoc);
		return -1;
	}

	if(0 != SetOneFieldToXml(pDoc,pXmlNode,"ToUserName",unmap.at("ToUserName"),true))
	{
		::writelog("xml node is NUll 3");
		DELETE_PTR(pDoc);
		return -1;
	}

	if(0 != SetOneFieldToXml(pDoc,pXmlNode,"FromUserName",unmap.at("FromUserName"),true))
	{
		::writelog("xml node is NUll 4");
		DELETE_PTR(pDoc);
		return -1;
	}

	if(0 != SetOneFieldToXml(pDoc,pXmlNode,"CreateTime",unmap.at("CreateTime"),true))
	{
		::writelog("xml node is NUll 5");
		DELETE_PTR(pDoc);
		return -1;
	}

	if(0 != SetOneFieldToXml(pDoc,pXmlNode,"MsgType",unmap.at("MsgType"),true))
	{
		::writelog("xml node is NUll 6");
		DELETE_PTR(pDoc);
		return -1;
	}

	if(0 != SetOneFieldToXml(pDoc,pXmlNode,"Content",unmap.at("Content"),true))
	{
		::writelog("xml node is NUll 7");
		DELETE_PTR(pDoc);
		return -1;
	}

	//转成string
	pDoc->Accept(&oPrinter);
	replyMsg = oPrinter.CStr();

	DELETE_PTR(pDoc);
	return 0;    
}

bool VerifyPhoneNum(string phone){
	// 检查电话长度是否为11位
	if(phone.size() != 11){
		::writelog("phone size not 11!");
		return false;
	}

	// 检查电话是否有非法字符
	for(uint32_t i = 0; i < phone.size();++i){
		if(phone[i] < '0' || phone[i] > '9'){
			::writelog("phone has invalid character!");
			return false;
		}
	}
	return true;
}
int LookupPhoneNumIndb(IMysqlConnPool *m_mysqlpool,string &openid,string phone){
	size_t pos = phone.find_first_of("#");
	if(pos == string::npos){
		::writelog("wrong format phone!");
		return ERROR_FORMAT_PHONENUM;
	}
	phone = phone.substr(0,pos);
	
	if(!VerifyPhoneNum(phone)){
		::writelog("VerifyPhoneNum(phone) failed!");
		return ERROR_FORMAT_PHONENUM;
	}
	
#if 1
	auto sql = m_mysqlpool->get();
	string query = FormatSql("select sceneid,wechartid,wechartphone from runningscene where wechartphone = %s",
			phone.c_str());
	if(sql->Select(query) == 0){
		auto res = sql->result();
		int c = res->rowCount();
		auto row = res->nextRow();
		for(;c > 0; c--){
			string wechartid = row->getString(1);
			//::writelog(InfoType,"wechartid3333333:%s",wechartid.c_str());
			uint64_t sceneid = row->getInt64(0);
			if(wechartid == ""){
				query = FormatSql("update runningscene set wechartid = '%s' where sceneid = %llu",openid.c_str(),sceneid);
				if(sql->Update(query) == -1 ){
					::writelog(InfoType,"update db failed!");
					return -1;
				}
			}
		}
		return 0;
	}
	else{
		::writelog(InfoType,"mysql查询结果为空!");
		return MYSQL_QUERY_NO_CONTENT;
		
	}
#endif
	return 0;
}

int WXPostDataXMLParse(const string &buffer,unordered_map<string,string> &unmap){
	unmap.clear();
	XMLDocument xmlDoc;
	if(XML_SUCCESS != xmlDoc.Parse(buffer.c_str(), buffer.size()))
	{
		::writelog("XML_SUCCESS != xmlDoc.Parse(buffer.c_str(), buffer.size()) error!");
		return -1;
	}

	XMLElement * xmlElement = xmlDoc.FirstChildElement("xml");
	if(NULL == xmlElement)
	{
		::writelog("NULL == xmlElement error!");
		return -1;
	}
	XMLElement * msgElement = xmlElement->FirstChildElement();
	for(;msgElement != NULL; msgElement = msgElement->NextSiblingElement()){
		unmap.insert({msgElement->Name(),msgElement->GetText()});
	}
	for(auto m:unmap){
		::writelog(InfoType,"%s-------- %s",m.first.c_str(),m.second.c_str());
	}
	return 0; 
}

#if 0
int WXPostDataToWX(IMysqlConnPool *m_mysqlpool,unordered_map<string,string> &unmap,string &replyMsg){
	if(unmap.at("MsgType") == "text"){
		
	}
	// 用户点击了菜单，推送菜单事件给开发者,提示用户输入手机号
	else if(unmap.at("Event") == "CLICK" && unmap.at("EventKey") == "V1001_INPUT_PHONE"){
		wxPostBackData.ToUserName = unmap.at("FromUserName");
		wxPostBackData.FromUserName = unmap.at("ToUserName");
		wxPostBackData.CreateTime = unmap.at("CreateTime");
		wxPostBackData.MsgType = "text";
		wxPostBackData.Content = "请输入mico客户端报警界面所输入的手机号,并以'#'结束!\n示例：138xxxx1234#";
		CreateXmlStr(wxPostBackData, replyContent);
	}
	return 0;
}
#endif

string GetTimeOfNow(){
	// 基于当前系统的当前日期/时间
	string res = "";
	time_t now = time(0);
	tm *ltm = localtime(&now);

	res += to_string(1900+ltm->tm_year);
	res += "年";
	res += to_string(1 + ltm->tm_mon);
	res += "月";
	res += to_string(ltm->tm_mday);
	res += "日";
	res += " ";
	res += to_string(ltm->tm_hour);
	res += ":";
	res += to_string(ltm->tm_min);
	return res;
}

int WXCreateMenu(string &jsonStr){
	StringBuffer buf;
	PrettyWriter<StringBuffer> writer(buf); // it can word wrap
	writer.StartObject();
	writer.Key("button");
	writer.StartArray();

#if 0
	writer.StartObject();
	writer.Key("type");	writer.String("click");
	writer.Key("name");	writer.String("≡合信报警1");
	writer.Key("key");	writer.String("V1001_INPUT_PHONE");
	writer.EndObject();
#endif

	// 第一个主菜单
	writer.StartObject();
	writer.Key("name"); writer.String("服务中心");
	writer.Key("sub_button"); 

	writer.StartArray();
	writer.StartObject();
	writer.Key("type"); writer.String("miniprogram");
	writer.Key("name"); writer.String("合信报警");
	writer.Key("url"); writer.String("http://mp.weixin.qq.com");
	writer.Key("appid"); writer.String("wxa3376a8cbea27476");
	writer.Key("pagepath"); writer.String("pages/index/index");
	writer.EndObject();
	writer.EndArray();

	writer.EndObject();

	// 第二个主菜单
	writer.StartObject();
	writer.Key("name"); writer.String("产品展示");
	writer.Key("sub_button"); 

	writer.StartArray();
	writer.StartObject();
	writer.Key("type"); writer.String("view");
	writer.Key("name");	writer.String("人机界面");
	writer.Key("url");	writer.String("http://www.co-trust.com/Products/Interface/center.html");
	writer.EndObject();
	writer.EndArray();

	writer.EndObject();

	// 第三个主菜单
	writer.StartObject();
	writer.Key("name"); writer.String("交流合作");
	writer.Key("sub_button"); 

	writer.StartArray();
	writer.StartObject();
	writer.Key("type"); writer.String("view");
	writer.Key("name");	writer.String("联系我们");
	//writer.Key("url");	writer.String("http://20201218.ct.com:8880/login");
	writer.Key("url");	writer.String("http://www.co-trust.com/Service/information/index.html");
	writer.EndObject();
	writer.EndArray();

	writer.EndObject();

	writer.EndArray();
	writer.EndObject();
	const char *json_content = buf.GetString();
	jsonStr.append(json_content);
	return 0;
}

void VerifyPhoneNumInDB(IMysqlConnPool *m_mysqlpool,unordered_map<string,string> &unmap,string &replyContent){
	string phone = unmap.at("Content");
	string wechartid = unmap.at("FromUserName");
	// 从数据库读取手机号关联的信息
	int ret = LookupPhoneNumIndb(m_mysqlpool,wechartid,phone);

	if(MYSQL_QUERY_NO_CONTENT == ret){
		replyContent = "该手机号尚未开启mico远程报警功能！";
	}
	else if(ERROR_FORMAT_PHONENUM == ret){
		replyContent = "您输入的手机号格式不对，请重试,并以'#'结束！\n示例：138xxxx1234#";
	}
	else if(0 == ret){
		replyContent = "恭喜您成功开启mico远程报警功能！";
	}
	else{
		replyContent = "请输入手机号开启mico远程报警功能，并以'#'结尾\n示例：138xxxx1234#";
	}
}

string GetLinuxTime(){
	time_t timep;
	struct tm *p;
	time(&timep);
	printf("time() : %d \n",timep);
	p=localtime(&timep);
	timep = mktime(p);
	printf("time()->localtime()->mktime():%d\n",timep);
	return to_string(timep);
}

int WXCreateMsg(IMysqlConnPool *m_mysqlpool,unordered_map<string,string> &unmap, string &strXml){
	if(unmap.find("FromUserName") == unmap.end() || unmap.find("ToUserName") == unmap.end()
			|| unmap.find("CreateTime") == unmap.end() || unmap.find("MsgType") == unmap.end()){
		::writelog(InfoType,"no msg type!!!!");
		return -1;
	}
	unordered_map<string,string> unmap_reply;
	unmap_reply.clear();
	string replyContent;
	if(unmap.at("MsgType") == "text"){
#if 1
		//VerifyPhoneNumInDB(m_mysqlpool,unmap,replyContent);
		replyContent = "欢迎使用合信云平台公众号！";
		unmap_reply.insert({"MsgType","text"});
		unmap_reply.insert({"FromUserName",unmap.at("ToUserName")});
		unmap_reply.insert({"ToUserName",unmap.at("FromUserName")});
		unmap_reply.insert({"Content",replyContent});
		unmap_reply.insert({"CreateTime",unmap.at("CreateTime")});
		unmap_reply.insert({"MsgId",unmap.at("MsgId")});
#endif
	}
	// 用户点击了报警菜单，推送菜单事件给开发者,提示用户输入手机号
	else if(unmap.at("MsgType") == "event" && unmap.at("Event") == "CLICK" && unmap.at("EventKey") == "V1001_INPUT_PHONE"){
#if 0
		unmap_reply.insert({"MsgType","text"});
		unmap_reply.insert({"FromUserName",unmap.at("ToUserName")});
		unmap_reply.insert({"ToUserName",unmap.at("FromUserName")});
		unmap_reply.insert({"Content", "欢迎您使用合信mico远程报警功能！\n您输入的手机号格式不对，请重试,并以'#'结束！\n示例：138xxxx1234#"});
		unmap_reply.insert({"CreateTime",unmap.at("CreateTime")});
		// 使用时间戳作为消息id
		unmap_reply.insert({"MsgId",GetLinuxTime()});
#endif
	}
	else{
		::writelog("user click not text and event type!");
		return -1;
#if 0
		// 用户发送了其他消息类型，如图片，音视频等,暂不做处理
		unmap_reply.insert({"MsgType","text"});
		unmap_reply.insert({"FromUserName",unmap.at("ToUserName")});
		unmap_reply.insert({"ToUserName",unmap.at("FromUserName")});
		unmap_reply.insert({"Content", "欢迎您使用合信云公众号平台！"});
		unmap_reply.insert({"CreateTime",unmap.at("CreateTime")});
		unmap_reply.insert({"MsgId",unmap.at("MsgId")});
#endif
	}
	
	return CreateXmlMsgStr(unmap_reply, strXml);
}

void WXCreateTemplateMsg(const string &openid, const string &alarmMsg, const string &scenename,
		const string &sceneowner, const string &alarmId, uint8_t alarmEventType, const string &alarmTime, string *template_msg){
	rapidjson::StringBuffer buf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf); // it can word wrap
	writer.StartObject();

	writer.Key("touser");writer.String(openid.c_str());
	writer.Key("mp_template_msg"); 
	writer.StartObject();
	writer.Key("appid"); writer.String(APPID.c_str());
	writer.Key("template_id"); 
	if(alarmEventType == 0){
		writer.String(TEMPLATE_ID_PRODUCT.c_str());
	}
	else if(alarmEventType == 1){
		writer.String(TEMPLATE_ID_RELEASE.c_str());
	}
	writer.Key("url"); writer.String("http://weixin.qq.com/download");
	writer.Key("miniprogram");
	writer.StartObject();
	writer.Key("appid"); writer.String(APPID_INTIME.c_str());
	writer.Key("pagepath"); writer.String("pages/index/index");
	writer.EndObject();

	writer.Key("data");
	writer.StartObject();

	writer.Key("first");
	writer.StartObject();
	writer.Key("value");
	writer.String(alarmMsg.c_str());
	writer.Key("color");
	writer.String("#173177");
	writer.EndObject();

	writer.Key("keyword1");
	writer.StartObject();
	writer.Key("value");
	writer.String(scenename.c_str());
	writer.Key("color");
	writer.String("#173177");
	writer.EndObject();

	writer.Key("keyword2");
	writer.StartObject();
	writer.Key("value");
	writer.String(sceneowner.c_str());
	writer.Key("color");
	writer.String("#173177");
	writer.EndObject();

	writer.Key("keyword3");
	writer.StartObject();
	writer.Key("value");
	writer.String(alarmId.c_str());
	writer.Key("color");
	writer.String("#173177");
	writer.EndObject();

	writer.Key("keyword4");
	writer.StartObject();
	writer.Key("value");
	writer.String(alarmTime.c_str());
	writer.Key("color");
	writer.String("#173177");
	writer.EndObject();

	writer.Key("remark");
	writer.StartObject();
	writer.Key("value");
	if(alarmEventType == 0){
		writer.String(TEMPLATE_PRODUCT_REMARK.c_str());
	}
	else if(alarmEventType == 1){
		writer.String(TEMPLATE_RELEASE_REMARK.c_str());
	}
	writer.Key("color");
	writer.String("#173177");
	writer.EndObject();

	writer.EndObject();
	writer.EndObject();

	//writer.Start("array");
	writer.EndObject();
	const char *json_content = buf.GetString();
	template_msg->append(json_content);
}

void GetIp(const string &domain, string *ip)
{

	struct hostent *host = gethostbyname(domain.c_str());
	if (host == NULL){
		::writelog("host is null!");
		return;

	}
	//for (int i = 0; host->h_addr_list[i]; i++)
	if(NULL == host->h_addr_list[0]){
		::writelog("NULL == host->h_addr_list[0] error!");
		return;	
	}
	// 1个ip地址
	char buf[20];
	char *tmp = buf;
	memset(tmp,'\0',20);
	strcpy(tmp, inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));
	//strcpy(const_cast<char*>(ip->c_str()), inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));
	ip->append(tmp,20);
}

// https加密协议,端口443
int SocketHttps(const string &host, const string &request, string &response, const string &domain)
{
	int sockfd;
	struct sockaddr_in address;

	sockfd = socket(AF_INET,SOCK_STREAM,0);
	address.sin_family = AF_INET;
	address.sin_port = htons(443);
	string ip;
	GetIp(domain, &ip);
	::writelog(InfoType,"wxip:%s",ip.c_str());
	if(ip == ""){
		::writelog(InfoType, "getIp failed!");
		return -1;
	}
	if(inet_pton(AF_INET, ip.c_str(), &address.sin_addr) <= 0){
		::writelog(InfoType,"inet pton error");
		return -1;
	}

	if(-1 == connect(sockfd,(struct sockaddr *)&address,sizeof(address))){
		::writelog("connection error!");
		return -1;
	}

	SSL *ssl;
	SSL_CTX *ctx;

	/*ssl init*/
	SSL_library_init();
	SSL_load_error_strings();
	ctx = SSL_CTX_new(SSLv23_client_method());
	if(ctx == NULL)
	{
		close(sockfd);
		::writelog("ctx is null!");
		return -1;
	}

	ssl = SSL_new(ctx);
	if(ssl == NULL)
	{
		close(sockfd);
		::writelog("ssl is null!");
		return -1;
	}

	/*把socket和SSL关联*/
	int ret = SSL_set_fd(ssl, sockfd);
	if(ret == 0)
	{
		close(sockfd);
		::writelog("ret == 0 error!");
		return -1;
	}

	ret = SSL_connect(ssl);
	if(ret != 1)
	{
		close(sockfd);
		::writelog("ret != 1 error!");
		return -1;
	}
	// 
	int send = 0;
	int totalsend = 0;
	const char *req = request.c_str();
	int nbytes = request.length();
	while(totalsend < nbytes)
	{
		send = SSL_write(ssl, req+totalsend, nbytes - totalsend);
		if(send == -1)
		{
			close(sockfd);
			::writelog("send == -1 error!");
			return -1;
		}
		totalsend  = send;
	}

	char rsp_str[RSP_BUF_LEN] = {0};
	
	ret = SSL_read(ssl, rsp_str, RSP_BUF_LEN);
	if(ret < 0)
	{
		close(sockfd);
		::writelog("ret < 0 error!");
		return -1;
	}
	response.append(rsp_str).append(1,'\0');

	/*end ssl*/
	SSL_shutdown(ssl);
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	ERR_free_strings();

	//::writelog(InfoType,"11111%s",response.c_str());

	close(sockfd);
	return 0;
}

int HttpsPostData(const string &host, string &post_content,string &post_response)
{
	//POST请求方式
	stringstream stream;
		//printf("Content-type:text/html\r\n\r\n");
	stream << "POST " << host; 
	stream << " HTTP/1.0\r\n";
	stream << "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n";
	stream << "Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\n";
	stream << "Accept-Encoding: gzip, deflate\r\n";
	stream << "Host: api.weixin.qq.com\r\n";
	stream << "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:80.0) Gecko/20100101 Firefox/80.0\r\n";
	//stream << "Content-Type:application/x-www-form-urlencoded\r\n";
	stream << "Content-Type: application/json;encoding=utf-8\r\n";
	stream << "Content-Length:" << post_content.length()<<"\r\n";
	stream << "Connection: keep-alive\r\n\r\n";
	stream << post_content.c_str();
	//stream << "\r\n\r\n";

	return SocketHttps(host, stream.str(), post_response);
}

int HttpsGetData(const string &host, string &get_content)
{
	//GET请求方式
	stringstream stream;
	//string str = "https://api.weixin.qq.com/cgi-bin/token?grant_type=client_credential&appid=wxa7c74a5ae2eb378b&secret=eab58d964c96fbb8e16c4b7c4ccb98ea";
	stream << "GET " << host;
	stream << " HTTP/1.0\r\n";
	stream << "Host: " << "api.weixin.qq.com" << "\r\n";
	stream <<"User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\n";
	stream <<"Connection:close\r\n\r\n";

	if(0 != SocketHttps(host, stream.str(),get_content)) {
		::writelog(InfoType, "SocketHttps failed errmsg: %S",get_content.c_str());
		return -1;
	}
	return 0;
}

int WXGetAccessToken(string &accessToken){
	
	// 获取access_token填写client_credential
	string host = "https://api.weixin.qq.com/cgi-bin/token?grant_type=client_credential";
	host += "&appid=";
	host += APPID;
	host += "&secret=";
	host += APPSECRET;
	if(0 != HttpsGetData(host,accessToken)){
		::writelog("HttpsGetData(host,accessToken) error!");
		return -1;
	}
	// 查找"accessToken"
	uint32_t posBeg,posEnd;
	posBeg = accessToken.find_first_of("\"",0);
	if(posBeg == string::npos){
		::writelog(InfoType,"1111");
		return -1;
	}

	posEnd = accessToken.find_first_of("\"",posBeg+1);
	if(posEnd == string::npos){
		::writelog(InfoType,"3333");
		return -1;
	}
	if("access_token" != accessToken.substr(posBeg+1,posEnd-posBeg-1)){
		::writelog("no accessToken string");
		return -1;
	}
	
	posBeg = accessToken.find_first_of("\"",posEnd+1);
	if(posBeg == string::npos){
		::writelog("posBeg == string::npos error!");
		return -1;
	}
	posEnd = accessToken.find_first_of("\"",posBeg+1);
	if(posEnd == string::npos){
		::writelog("posEnd == string::npos error!");
		return -1;
	}
	accessToken = accessToken.substr(posBeg+1,posEnd-posBeg-1);

	return 0; 
}

void gettimestr(char *buf, int sizeofbuf)
{
    time_t t = time(0);
    struct tm local_time;
    localtime_r(&t, &local_time);
    strftime(buf, sizeofbuf, "%F %T", &local_time);
}

void WXGetCallbackIp(const string &accessToken) {
	string host = "https://api.weixin.qq.com/cgi-bin/getcallbackip?access_token=";
	host += accessToken;
	string iplistofjson;
	vector<string> iplist;
	if(0 != HttpsGetData(host,iplistofjson)) {
		::writelog(InfoType, "get iplistofjson failed!");
		return;
	}
	int beg,end;
	beg = iplistofjson.find("{");
	end = iplistofjson.find("}");
	iplistofjson = iplistofjson.substr(beg,end-beg+1);
	Document doc;
	doc.Parse(iplistofjson.c_str());
	if (!doc.IsObject()) {
		::writelog(InfoType, "iplistofjson not json!");
		return;
	}
	bool valid = doc.HasMember("../fcgi/ip_list");
	if(!valid){
		::writelog(InfoType, "iplistofjson lost member!");
		return;
	}
	Value &iplistarray = doc["ip_list"];
	if(!iplistarray.IsArray()) {
		::writelog(InfoType, "ip_list not a array");
		return;
	}
	string content;
	for(int i = 0; i < iplistarray.Size(); ++i) {
		content = iplistarray[i].GetString();
		iplist.push_back(content);
	}
	ofstream ofile;
	ofile.open("iplist.dat", ios::app);
	if(!ofile.is_open()) {
		::writelog(InfoType, "open file iplist.dat failed!");
		return;
	}
	char buf[1024];
	gettimestr(buf, sizeof(buf));
	ofile << "=====" << buf << "=====" << endl;
	for(auto element: iplist){
		ofile << element << endl;
	}
	ofile << endl;
	ofile.close();
}
int ParseAlarmMsgFromJson(const string &msg, uint64_t *sceneid, string *alarmid, 
		string *alarmText, string *alarmTime, uint8_t *alarmEventType) {
    Document doc;
    doc.Parse(msg.c_str());
    if (!doc.IsObject()) {
		::writelog(InfoType, "msg not json!");
		return -1;
	}

	::writelog(InfoType, "alarmMsg: %s", msg.c_str());
    bool valid = doc.HasMember("id")
            && doc.HasMember("alarmId")
            && doc.HasMember("alarmText")
            && doc.HasMember("alarmTime")
            && doc.HasMember("alarmEventType");
    if (!valid) {
		::writelog(InfoType, "alarmMsg lost member!");
		return -1;
	}

    *sceneid = doc["id"].GetUint64();
    *alarmid = doc["alarmId"].GetString();
    *alarmText = doc["alarmText"].GetString();
    *alarmTime = doc["alarmTime"].GetString();
    *alarmEventType = (uint8_t)doc["alarmEventType"].GetUint();
	::writelog(InfoType, "alarmid:%s",alarmid->c_str());
	::writelog(InfoType, "alarmText:%s",alarmText->c_str());
	::writelog(InfoType, "alarmTime:%s",alarmTime->c_str());
	return 0;
}

int FindOpenidWithSceneid(IMysqlConnPool *m_mysqlpool, uint64_t sceneid, string *openid) {
	auto sql = m_mysqlpool->get();
	// 在表runningscene根据sceneid查phone
	string query = FormatSql("select wechartphone from runningscene where sceneid = %llu",sceneid);
	string phone;
	int ret = sql->Select(query); 
	if(ret == 0){
		auto res = sql->result();
		int c = res->rowCount();
		auto row = res->nextRow();
		if (c == 1){
			phone = row->getString(0);
		}
		else{
			::writelog(InfoType,"phone not unique:sceneid = %llu",sceneid);
			return -1;
		}
	}
	else if(ret == 1){
		::writelog(InfoType,"select wechartphone from runningscene return null!");
		return -1;
	}
	else{
		::writelog(InfoType, "select wechartphone from runningscene failed!");
		return -1;
	}
	// 在表phonewechartid根据phone查wechartid
	query = FormatSql("select wechartid from phonewechartid where phone = %s", phone.c_str());
	ret = sql->Select(query); 
	if(ret == 0){
		auto res = sql->result();
		int c = res->rowCount();
		auto row = res->nextRow();
		if (c == 1){
			*openid = row->getString(0);
		}
		else{
			::writelog(InfoType,"wechartid not unique:phone = %s",phone.c_str());
			return -1;
		}
	}
	else if(ret == 1){
		::writelog(InfoType,"select wechartid from phonewechartid return null!");
		return -1;
	}
	else{
		::writelog(InfoType, "select wechartid from phonewechartid failed!");
		return -1;
	}
	return 0;
}

int FindSceneNameWithSceneid(IMysqlConnPool *m_mysqlpool, uint64_t sceneid, string *scenename) {
	auto sql = m_mysqlpool->get();
	string query = FormatSql("select name from T_Scene where ID = %llu",sceneid);
	int ret = sql->Select(query); 
	if(ret == 0){
		auto res = sql->result();
		int c = res->rowCount();
		auto row = res->nextRow();
		if (c == 1){
			*scenename = row->getString(0);
		}
		else{
			::writelog(InfoType,"scenename not unique:sceneid = %llu",sceneid);
			return -1;
		}
	}
	else if(ret == 1){
		::writelog(InfoType,"select name from T_Scene return null!");
		return -1;
	}
	else{
		::writelog(InfoType, "select name from T_Scene failed!");
		return -1;
	}
	return 0;
}

int FindSceneOwnerWithSceneid(IMysqlConnPool *m_mysqlpool, uint64_t sceneid, string *sceneowner) {
	uint64_t ownerid;
	auto sql = m_mysqlpool->get();
	string query = FormatSql("select ownerID from T_Scene where ID = %llu",sceneid);
	int ret = sql->Select(query); 
	if(ret == 0){
		auto res = sql->result();
		int c = res->rowCount();
		auto row = res->nextRow();
		if (c == 1){
			ownerid = row->getUint64(0);
		}
		else{
			::writelog(InfoType,"ownerid not unique:sceneid = %llu",sceneid);
			return -1;
		}
	}
	else if(ret == 1){
		::writelog(InfoType,"select ownerID from T_Scene return null!");
		return -1;
	}
	else{
		::writelog(InfoType, "select ownerID from T_Scene failed!");
		return -1;
	}

	query = FormatSql("select account from T_User where ID = %llu",ownerid);
	ret = sql->Select(query); 
	if(ret == 0){
		auto res = sql->result();
		int c = res->rowCount();
		auto row = res->nextRow();
		if (c == 1){
			*sceneowner = row->getString(0);
		}
		else{
			::writelog(InfoType,"account not unique:ownerid = %llu",ownerid);
			return -1;
		}
	}
	else if(ret == 1){
		::writelog(InfoType,"select account from T_User return null!");
		return -1;
	}
	else{
		::writelog(InfoType, "select account from T_User failed!");
		return -1;
	}
	return 0;

}

int NotifyUserWX(IMysqlConnPool *m_mysqlpool, const string &accessToken, const string &alarmMsg){
	uint64_t sceneid;
	string alarmid;
	string alarmText, alarmTime, sceneowner;
	uint8_t alarmEventType;
	string template_id, host, strPostData, strRecvData, phone, openid;
	string scenename;
	if(0 != ParseAlarmMsgFromJson(alarmMsg, &sceneid, &alarmid, &alarmText, &alarmTime, &alarmEventType)) {
		::writelog(InfoType, "ParseAlarmMsgFromJson failed!");
		return -1;
	}

	if(0 != FindOpenidWithSceneid(m_mysqlpool, sceneid, &openid)) {
		::writelog(InfoType,"FindOpenidWithSceneid failed!");
		return -1;
	}

	if(0 != FindSceneNameWithSceneid(m_mysqlpool, sceneid, &scenename)) {
		::writelog(InfoType, "FindSceneNameWithSceneid failed!");
		return -1;
	}
	::writelog(InfoType, "scenenameeeeeee:%s",scenename.c_str());

	if(0 != FindSceneOwnerWithSceneid(m_mysqlpool, sceneid, &sceneowner)){
		::writelog(InfoType, "FindSceneOwnerWithSceneid");
		return -1;
	}
	// 发报警模板消息
	if(openid == ""){
		::writelog("openid is empty!");
		return -1;
	}
	WXCreateTemplateMsg(openid, alarmText, scenename, sceneowner, alarmid, alarmEventType, 
			alarmTime, &strPostData);
	host = "https://api.weixin.qq.com/cgi-bin/message/wxopen/template/uniform_send?access_token=";
	host += accessToken;
	if(0 != HttpsPostData(host, strPostData,strRecvData)){
		::writelog("HttpsPostData(host, strPostData,strRecvData) error!");
		return -1;
	}
	//alarmMsgContent = "";
	//::writelog(InfoType, "strRecvData response:%s" ,strRecvData.c_str());
	return 0;
}
}

