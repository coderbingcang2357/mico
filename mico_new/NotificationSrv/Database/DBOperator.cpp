#include <stdlib.h>

#include "DBOperator.h"
#include "util/util.h"
#include "Message/Notification.h"
#include "config/configreader.h"
//#include "util/logwriter.h"

using namespace DB;

static const string EMPTY_STRING = "";

//构造
DBOperator::DBOperator()
{
    Configure *conf = Configure::getConfig();

    SqlParam par;
    par.host = conf->notifysrv_sql_host.c_str();
    par.user = conf->notifysrv_sql_user.c_str();
    par.passwd = conf->notifysrv_sql_pwd.c_str();
    par.db = conf->notifysrv_sql_db.c_str();
    par.port = conf->notifysrv_sql_port;

    m_sql.connect(par);
}

//析构
DBOperator::~DBOperator()
{

}

bool DBOperator::SaveNotification(const Notification& notification, uint64_t *insertid)
{
    char* container = new char[2 * notification.GetMsgLen() + 1];
    memset(container, 0, 2 * notification.GetMsgLen() + 1);
    int l = 0;
    if (notification.GetMsgLen() != 0)
    {
        l = m_sql.RealEscapeString(container,
               const_cast<char *>(notification.GetMsgBuf()),
                               notification.GetMsgLen());
    }
    //insert into 
    //char buf[1024 * 10];
    std::vector<char> buf(l + 1024, '\0');
    snprintf(buf.data(), buf.size() - 1, //sizeof(buf),
        "insert into T_OfflineMsg (subjectID, notifyNum, priority, recvTime, msgLen, msg, answercode) "
        "values (%lu, %u, %u, FROM_UNIXTIME(%lu), %u, '%s', %d);",
        notification.GetObjectID(),
        notification.getNotifyType(),
        0,
        notification.GetCreateTime(),
        notification.GetMsgLen(),
        std::string(container, l).c_str(),
        notification.answercode());

    int ret = m_sql.Insert(buf.data());

    delete [] container;

    if (ret < 0)
    {
        return false;
    }
    else
    {
        // ok, 返回插入的ID, 这个会用来作为通知号
        *insertid = m_sql.getAutoIncID();
    }
    return true;
}

bool DBOperator::GetNotification(uint64_t objectID,
                            std::vector<Notification *> *vNotification)
{
    char statement[1024];
    snprintf(statement, sizeof(statement), 
        "select ID, subjectID, notifyNum, recvTime, msgLen, msg, answercode "
        "from T_OfflineMsg "
        "where subjectID=%lu",
        objectID);
    int ret = m_sql.Select(statement);
    if(ret < 0)
    {
        return false;
    }

    MYSQL_RES *res = m_sql.Res();
    uint64_t rowcount = mysql_num_rows(res);
    for(my_ulonglong i = 0; i != rowcount; i++)
    {
        MYSQL_ROW row = m_sql.FetchRow(res);
        unsigned long *len = mysql_fetch_lengths(res);(void)len;

        Notification *notification = new Notification;
        notification->setSerialNumber(StrtoU64(row[0])); // serial
        notification->SetObjectID(StrtoU64(row[1])); // objectid
        notification->setNotifyType(StrtoU16(row[2])); // notify type
        notification->SetCreateTime(StrtoTime(row[3])); // time
        uint64_t msglen = StrtoU16(row[4]); // msg len
        assert(msglen == len[5]);
        notification->SetMsg(row[5], msglen); // msg
        notification->setAnswcCode(StrtoU8(row[6]));
        vNotification->push_back(notification);
    }

    return true;
}

bool DBOperator::DeleteNotification(uint64_t objectID, uint64_t notifNumber)
{
    //string statement = EMPTY_STRING
    //    + "DELETE "
    //    + " FROM "  + TOFFL
    //    + " WHERE " + TOFFL_OBJECT_ID + "=\'" + U64toStr(objectID) + "\'"
    //    + " AND " + TOFFL_NOTIF_NUM + "=\'" + U32toStr(notifNumber) + "\'";
    std::string statement;
    statement.append("delete from ");
    statement.append(TOFFL);
    statement.append(" where ");
    statement.append(TOFFL_OBJECT_ID);
    statement.append(" = ");
    statement.append(U64toStr(objectID));
    statement.append(" and ");
    statement.append(TOFFL_ID);
    statement.append(" = ");
    statement.append(U64toStr(notifNumber));
    statement.append(";");

    return m_sql.Delete(statement) != -1;
}

bool DBOperator::DelOfflineMsgIfMoreThan300(uint64_t objectID, uint32_t maxnum){
	std::string statement;
	statement.append("select count(1) as sum from T_OfflineMsg where subjectID = ");	
	statement.append(U64toStr(objectID));
    statement.append(";");
	int count;
    int ret = m_sql.Select(statement);
    if(ret < 0)
    {
        return false;
    }
	if(ret == 0){
		auto res = m_sql.result();
		int c = res->rowCount();
		auto row = res->nextRow();
		if (c == 1){
			count = row->getInt32(0);
			printf("objectIDDDDDDDDDD = %llu\n",objectID);
			printf("counttttttttt = %d\n",count);

		}
		else{
			return false;
		}
	}
	else{
		return false;
	}
	if(count > maxnum){
		statement = "";
		statement.append("delete from T_OfflineMsg where ID in (select ID from (\ 
select ID from T_OfflineMsg where subjectID = ");
		statement.append(U64toStr(objectID));
		statement.append(" order by recvTime limit ");
		statement.append(U32toStr(count-maxnum));
		statement.append(") as a);");
		return m_sql.Delete(statement) != -1;
	}
	return true;
}
