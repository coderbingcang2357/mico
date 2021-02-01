#include <QVariant>
#include <QDate>
#include <QDateTime>
#include <QTextCodec>
#include "projectrunner.h"
#include <string>
#include <vector>
#include "../../util/util.h"
#include "../../util/logwriter.h"
#include "../../timer/TimeElapsed.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

ProjectRunner::ProjectRunner(HMI::HmiProject *_prj)
    : prj(_prj)
{

}

bool ProjectRunner::init()
{
	//HMI::HmiProject *tmpPrj = prj->m_hmiservice.m_commServiceList[0]->m_connectList[0]->requestVarsList[0];
    for (const auto &v : prj->var) {
        this->idvar.insert(v.varId, v);
    }
    for (const auto &c : prj->connection) {
        this->idconn.insert(c.connid, c);
    }
    for (const auto &a : prj->alarm) {
        std::shared_ptr<HMI::Alarm> al = std::make_shared<HMI::Alarm>();
        *al = a;
        //this->idalarm.insert(a.alarmid, al);
        this->idalarm[a.alarmid].push_back(al);
        // 0：模拟量报警；1：离散量报警
        if (a.alarmType == 0)
        {
            //varidAnalogAlarm.insert(a.trigVarId, al);
            varidAnalogAlarm[a.trigVarId].push_back(al);
        }
        else if (a.alarmType == 1)
        {
            variddiscreteAlarm[a.trigVarId].push_back(al);
        }
        else
        {
            // what to do?
        }
    }


    return true;
}

bool ProjectRunner::start()
{
    if (m_alarmtimer != nullptr)
    {
        delete m_alarmtimer;
    }
    m_alarmtimer = new QTimer(this);
    m_alarmtimer->setInterval(5 * 1000);
    connect(m_alarmtimer, &QTimer::timeout,
            this, &ProjectRunner::doalarm);
    m_alarmtimer->start();
    return true;
}

void ProjectRunner::doalarm()
{
    //for (auto &a : this->idalarm)
    //{
    //    a.alarmid
    //}
    //m_alarmtimer->stop();//停止事件触发防止事件堵塞
    QList<struct ComMsg> sm_ComMsgList;
    int ret = getComMsg( sm_ComMsgList );

    if ( ret > 0 )
    {
        //分发消息
        foreach( struct ComMsg msg, sm_ComMsgList )
        {
            if ( msg.varId >= 0xffff )
            {
                continue;
            }
            //int rid = sm_externalTagIds.key( msg.varId );
             int rid = msg.varId;
            if ( rid < 1 )
            {
                continue;
            }
            if ( msg.type == 0 ) //读变量
            {
                if ( msg.err == VAR_DISCONNECT )
                {
                }
                else
                {
                    readExtTag( rid );
                }
            }
            else if ( msg.type == 1 ) //写消息
            {
                if ( msg.err !=  VAR_NO_ERR )
                {
                        //SysAlarmInfo sysAlarmInfo;
                        //sysAlarmInfo.id = 100008;
                        //QString text = tr( "Tag" ) + QString::number( rid ) + tr( "write to  PLC err" );
                        //sysAlarmInfo.text = text;
                        //sysAlarmInfo.connectName = "";

//                        SystemAlarmEvent *ev = new SystemAlarmEvent( );
//                        ev->setSysAlarmInfo( sysAlarmInfo );
//                        QApplication::postEvent( sm_ptrAlarmReceiver, ev );
                }
            }
        }
    }


    if ( ret > 0 )
    {
        //获得连接状态
        //int ret = CONNECT_NO_OPERATION;
        //for ( int i = 0; i < sm_connectCount; i++ )
        //{
        //    ret = getConnLinkStatus( i );
        //    int cid = sm_connectIds.value( i );//由连续注册Id得到连接Id
        //    quint8 conSta = sm_connectStatus[ i ];
        //    if ( ret == CONNECT_LINKED_OK )
        //    {
        //        if ( m_ConReTryCount.value( i ) < MAX_RETRY_COUNT )
        //            m_ConReTryCount.insert( i, MAX_RETRY_COUNT );

        //        if ( conSta != CONNECTED )
        //        {
        //            sm_connectStatus[i] = CONNECTED;// 将该连接状态改为连接
                    //SysAlarmInfo sysAlarmInfo;// 产生100003号系统报警
                    //sysAlarmInfo.id = 100003;
                    //sysAlarmInfo.text = tr( "Connect successfully: %1, Station %2." ).arg( sm_connectId_Name.value( cid ) ).arg( plcAddress( cid ) );

                    //emit signal_showConnectStatus(sysAlarmInfo.text, sm_connectId_Name.value( cid ), plcAddress( cid ));

                    //sysAlarmInfo.connectName = "";
//                    if ( sm_ptrAlarmReceiver )
//                    {
//                        SystemAlarmEvent *ev = new SystemAlarmEvent();
//                        ev->setSysAlarmInfo( sysAlarmInfo );
//                        QApplication::postEvent( sm_ptrAlarmReceiver, ev );
//                    }
        //        }
        //    }
        //}
    }
    m_alarmtimer->start( 5* 1000 );
}

void ProjectRunner::readExtTag( int id )
{
    int ret = VAR_NO_OPERATION;
    QVariant value;
    bool isCon = true;
    TagDataType dataType = tagDataType( id );
    if ( dataType == Char )
    {
        signed char i;
        ret = read_plc_var( id, &i, sizeof( i ) );//0:成功; -1:读取失败; -2:连接断开;-3:驱动加载失败
        if ( ret == VAR_NO_ERR )
            value = QVariant( i );
        else if ( ret != VAR_NO_OPERATION  && ret != -2 )
            isCon = false;
    }
    else if ( dataType == Byte || dataType == BitBlock4 || dataType == BitBlock8 )
    {
        unsigned char i = 0;
        ret = read_plc_var( id, &i, sizeof( i ) );
        if ( ret == VAR_NO_ERR )
            value = QVariant( i );
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }
    else if ( dataType == Int21 || dataType == Int )
    {
        signed short int i;

        ret = read_plc_var( id, &i, sizeof( i ) );
        char swap, *temp = (char *) &i;
        if(dataType == Int21)
        {
            swap = *temp;
            *temp = *(temp +1);
            *(temp +1) = swap;
        }
        if ( ret == VAR_NO_ERR )
            value = QVariant( i );
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }
    else if ( dataType == UInt21 || dataType == Word  || dataType == BitGroup || dataType == BitBlock12 || dataType == BitBlock16 )
    {
        unsigned short int i = 0;
        ret = read_plc_var( id, &i, sizeof( i ) );
        char swap, *temp = (char *) &i;
        if(dataType == UInt21)
        {
            swap = *temp;
            *temp = *(temp +1);
            *(temp +1) = swap;
        }
        if ( ret == VAR_NO_ERR )
        {
            if ( dataType == BitGroup )
            {
                i = (( i & 0x5555 ) << 1 ) | (( i >> 1 ) & 0x5555 );
                i = (( i & 0x3333 ) << 2 ) | (( i >> 2 ) & 0x3333 );
                i = (( i & 0x0f0f ) << 4 ) | (( i >> 4 ) & 0x0f0f );
                i = (( i & 0x00ff ) << 8 ) | (( i >> 8 ) & 0x00ff );
            }
            value = QVariant( i );
        }
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }
    else if ( dataType == Long4321 || dataType == Long3412 || dataType == Long2143 || dataType == DInt )
    {
        signed int i;
        ret = read_plc_var( id, &i, sizeof( i ) );
        char swap, *temp = (char *) &i;
        switch(dataType)
        {
        case Long4321:
            swap = *temp;
            *temp = *(temp +3);
            *(temp +3) = swap;
            swap = *(temp+1);
            *(temp+1) = *(temp +2);
            *(temp +2) = swap;
            break;
        case Long3412:
            swap = *temp;
            *temp = *(temp +2);
            *(temp +2) = swap;
            swap = *(temp+1);
            *(temp+1) = *(temp +3);
            *(temp +3) = swap;
            break;
        case Long2143:
            swap = *temp;
            *temp = *(temp +1);
            *(temp +1) = swap;
            swap = *(temp+3);
            *(temp+3) = *(temp +2);
            *(temp +2) = swap;
            break;
        default:
            break;
        }
        if ( ret == VAR_NO_ERR )
            value = QVariant( i );
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }
    else if ( dataType == ULong4321 || dataType == ULong3412 || dataType == ULong2143
              || dataType == DWord || dataType == BitBlock20 || dataType == BitBlock24 || dataType == BitBlock28 || dataType == BitBlock32 )
    {
        unsigned int i = 0; //BitBlock24 BitBlock20 must clear the first byte.
        ret = read_plc_var( id, &i, sizeof( i ) );
        char swap, *temp = (char *) &i;
        switch(dataType)
        {
        case ULong4321:
            swap = *temp;
            *temp = *(temp +3);
            *(temp +3) = swap;
            swap = *(temp+1);
            *(temp+1) = *(temp +2);
            *(temp +2) = swap;
            break;
        case ULong3412:
            swap = *temp;
            *temp = *(temp +2);
            *(temp +2) = swap;
            swap = *(temp+1);
            *(temp+1) = *(temp +3);
            *(temp +3) = swap;
            break;
        case ULong2143:
            swap = *temp;
            *temp = *(temp +1);
            *(temp +1) = swap;
            swap = *(temp+3);
            *(temp+3) = *(temp +2);
            *(temp +2) = swap;
            break;
        default:
            break;
        }
        if ( ret == VAR_NO_ERR )
            value = QVariant( i );
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }
    else if ( dataType == Real || dataType == Real4321 || dataType == Real3412 || dataType == Real2143)
    {
        float i;
        ret = read_plc_var( id, &i, sizeof( i ) );
        char swap, *temp = (char *) &i;
        switch(dataType)
        {
        case Real4321:
            swap = *temp;
            *temp = *(temp +3);
            *(temp +3) = swap;
            swap = *(temp+1);
            *(temp+1) = *(temp +2);
            *(temp +2) = swap;
            break;
        case Real3412:
            swap = *temp;
            *temp = *(temp +2);
            *(temp +2) = swap;
            swap = *(temp+1);
            *(temp+1) = *(temp +3);
            *(temp +3) = swap;
            break;
        case Real2143:
            swap = *temp;
            *temp = *(temp +1);
            *(temp +1) = swap;
            swap = *(temp+3);
            *(temp+3) = *(temp +2);
            *(temp +2) = swap;
            break;
        default:
            break;
        }
        if ( ret == VAR_NO_ERR )
        {
            //conv_endian_32bit(( unsigned long * )&i );
            //QString floatValue = QString( "%1" ).arg( i, 0, 'f', 6 );
            //value = QVariant( floatValue );
            //#####v1.31修改,当i = 3333.33,转换成字符串后再由QVariant转为double有问题,IO域显示nan(Not-a-Number)
            value = QVariant( i );
        }
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }
    else if ( dataType == Bool )
    {
        unsigned char i = 0;
        ret = read_plc_var( id, &i, sizeof( i ) );
        i = i & 0x01;
        if ( ret == VAR_NO_ERR )
        {
            value = QVariant( i );
        }
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }
    else if ( dataType == StringChar )
    {
        //ST_VARIABLE *v = sm_Tags.value( id );
        //Q_CHECK_PTR( v );
        //int len = v->len;
        int len = idvar[id].len;
        char data[4096];//data[len+1];
        memset( data, 0,  4096);//len + 1
        ret = read_plc_var( id, data, len + 1/*sizeof( data )*/ );
        if ( ret == VAR_NO_ERR )
        {
          //  QString strValue( data );
            //GB2312 ---------> UTF8 By Emoson
            QByteArray *ba_gb2312 = new QByteArray( len + 1, 0 );
            memcpy( ba_gb2312->data(), data, len + 1 );

            QTextCodec::ConverterState state;
            QTextCodec *gb = QTextCodec::codecForName("GB2312");
            QString strUnicode=gb->toUnicode(ba_gb2312->data(), ba_gb2312->length(), &state);
            QTextCodec *xcode = QTextCodec::codecForLocale();
            QByteArray utf8_bytes= xcode->fromUnicode(strUnicode);

            delete ba_gb2312;
            value = QVariant( utf8_bytes );

        }
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }
    else if ( dataType == Timer )
    {
        unsigned int i;
        ret = read_plc_var( id, &i, sizeof( i ) );
        if ( VAR_NO_ERR == ret )
        {
            value = QVariant( i );
        }
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }
    else if ( dataType == S7300_Counter )/*300MPI 计数器dd by tao 2013.03.28 */
    {
        unsigned short i;
        ret = read_plc_var( id, &i, sizeof( i ) );
        if ( ret == VAR_NO_ERR )
            value = QVariant( i );
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }
    else if ( dataType == S7300_Timer )/*300MPI 定时器dd by tao 2013.03.28 */
    {
        unsigned int i;
        ret = read_plc_var( id, &i, sizeof( i ) );
        if ( ret == VAR_NO_ERR )
            value = QVariant( i );
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }
    else if ( dataType == S7300_Date)/*300MPI 日期add by tao 2013.03.28 */
    {
        unsigned short i;
        ret = read_plc_var( id, &i, sizeof( i ) );

        if ( ret == VAR_NO_ERR )
        {
            QDate date(1990,1,1);
            date = date.addDays(i);
            QDateTime dtime( date );
           // qDebug()<<"Date "<<dtime<<" i "<<i;
            value = QVariant( dtime );
        }
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }
    else if ( dataType == S7300_Time )/*300MPI时间add by tao 2013.03.28 */
    {
        unsigned int i;
        ret = read_plc_var( id, &i, sizeof( i ) );
        if ( ret == VAR_NO_ERR )
            value = QVariant( i );
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }
    else if ( dataType == S7300_TimeofDay )/*300MPI 当天时间add by tao 2013.03.28 */
    {
        unsigned int i;
        ret = read_plc_var( id, &i, sizeof( i ) );

        if ( ret == VAR_NO_ERR )
        {
            QDate date(1990,1,1);
            i = i/1000;
            QTime time(i/60/60, (i/60)%60, i%60);
            //qDebug()<<"Time "<<time<<" i "<<i;
            QDateTime dtime;//(&date, &time);
            dtime.setDate(date);
            dtime.setTime(time);
            value = QVariant( dtime );
           // qDebug()<<"TimeOfDay "<<dtime;
        }
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }
    else if ( dataType == DateTime )
    {
        unsigned char i[8];
        ret = read_plc_var( id, i, 8 );

        if ( ret == VAR_NO_ERR )
        {
            int y,M,d,h,m,s;
            //qDebug()<<hex<<i[0]<<i[1]<<i[2]<<i[3]<<i[4]<<i[5]<<i[6]<<i[7];
            y = (i[0]>>4)*10 + ((i[0]>>0) & 0x0F);
            if (y>=90)
            {
                y += 1900;
            }
            else
            {
                y += 2000;
            }
            M = (i[1]>>4)*10 + ((i[1]>>0)&0x0F);
            d = (i[2]>>4)*10 + ((i[2]>>0)&0x0F);
            h = (i[3]>>4)*10 + ((i[3]>>0)&0x0F);
            m = (i[4]>>4)*10 + ((i[4]>>0)&0x0F);
            s = (i[5]>>4)*10 + ((i[5]>>0)&0x0F);
            //qDebug()<<y<<" "<<M<<" "<<d<<" "<<h<<" "<<m<<" "<<s<<" ";
            QDate date(y,M,d);
            QTime time(h, m, s);
            QDateTime dtime;//(&date, &time);
            dtime.setDate(date);
            dtime.setTime(time);
            //qDebug()<<"DateTime "<<dtime;
            value = QVariant( dtime );
        }
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }
    else if ( dataType == S7300_String)/*300MPI字符串类型dd by tao 2013.03.28 */
    {
        //ST_VARIABLE *v = sm_Tags.value( id );
        //Q_CHECK_PTR( v );
        //int len = v->len;
        int len = idvar[id].len;
        char data[len+1];
        memset( data, 0, len + 1 );
        ret = read_plc_var( id, data, sizeof( data ) );
        if ( ret == VAR_NO_ERR )
        {
            QString strValue( &data[2] );
            value = QVariant( strValue );
        }
        else if ( ret != VAR_NO_OPERATION && ret != -2 )
            isCon = false;
    }

    if (( ret == VAR_NO_ERR && value.isValid() ) || !isCon )//更新HMI变量值
        updateTagValue( id, value, isCon );
}

void ProjectRunner::updateTagValue( int id, const QVariant &newValue, bool isCon )
{
    Q_ASSERT( id > 0 );
    if(!idvar.contains(id))
       return;
    //ST_VARIABLE *v = sm_Tags.value( id );
    //if ( !v )
    //    return;

    HMI::Variable *v = &idvar[id];
    QVariant value = newValue;

    bool oldIsCon = v->isConnected;
    v->isConnected = isCon;
    if ( isCon == false )// 如果连接状态为断开
    {
        if ( oldIsCon != isCon )// 状态由连接该变成断开
        {
            // limitDisposal( v, value );
        }
        return;
    }


//#if 0
    //LinearScaling *ls = sm_tagLinearScalingHash.value( id );// 线性转换处理
    LinearScaling *ls = nullptr;
    if ( ls )
    {
        double lPLCValue = ls->lowerPLCValue;
        double uPLCValue = ls->upperPLCValue;
        double lHMIvalue = ls->lowerHMIValue;
        double uHMIValue = ls->upperHMIValue;
        double st = (( uHMIValue - lHMIvalue ) / ( uPLCValue - lPLCValue ) );
        double addvalue = ( newValue.toDouble() - lPLCValue ) * st;

        signed int add;
        if ( st < 1 )
            add = (( signed int )( addvalue * 10 ) ) % ( signed int )10;
        else
            add = 0;

        if ( v->dataType == Float ||
             v->dataType == Real4321 || v->dataType == Real3412 || v->dataType == Real2143)
        {
            value = addvalue + lPLCValue;
        }
        else
        {
            if ( add > 0 )
            {
               // 进行四舍五入转换
                if ( add >=  5 )
                    value = ( signed int )addvalue + lHMIvalue + 1;
                else
                    value = ( signed int )addvalue + lHMIvalue;
            }
            else
            {
                if ( add <=  -5 )
                    value = ( signed int )addvalue + lHMIvalue - 1;
                else
                    value = ( signed int )addvalue + lHMIvalue;
            }
        }
    }

    if ( v->value != value )// 判断值是否改变
    {
        QVariant oldValue = v->value;
        bool isSet = false;
        if ( !v->isOnline )
        //if ( v->isOnline )
            isSet = setHMITagValue( v, value, true );
        if ( isSet )
        {
            //if ( v->dataLogMode == 0 )// 值变化时进行数据记录
            //    dataLog( v, value );
            alarmDisposal( v, oldValue );
            //limitDisposal( v, oldValue );
        }
        else
        {
            //limitDisposal( v, oldValue );//设值不成功,使用用原值刷新
        }

    }
    else if ( oldIsCon != isCon )
    {
        //limitDisposal( v, value );
        alarmDisposal( v, value );

    }

//#endif
//        alarmDisposal( v, value );
}

/*!
    \brief 设置HMI系统内存中变量的值,根据各种数据类型进行越界判定。
    \param 指向系统变量结构体的指针
    \param 系统变量新值
*/
bool ProjectRunner::setHMITagValue( HMI::Variable *v, const QVariant &value, bool isUpdateFromPLC )
{
    if ( !v )
        return false;

    TagDataType dt = (TagDataType)v->dataType;
    if ( dt == Char )
    {
        int tmp = ( int )value.toDouble();
        if ( tmp < -128 || tmp > 127 )
            return false;

        v->value = QVariant( tmp );
    }
    else if ( dt == Byte )
    {
        int tmp = ( int )value.toDouble();
        if ( tmp < 0 || tmp > 255 )
            return false;

        v->value = QVariant( tmp );
    }
    else if ( dt == Int || dt == Int21 )
    {
        int tmp = ( int )value.toDouble();
        if ( tmp < -32768 || tmp > 32767 )
            return false;

        v->value = QVariant( tmp );
    }
    else if ( dt == Word || dt == UInt || dt == UInt21)
    {
        int tmp = ( int )value.toDouble();
        if ( tmp < 0 || tmp > 65535 )
            return false;

        v->value = QVariant( tmp );
    }
    else if ( dt == DInt || dt == Long
              || dt == Long4321 || dt == Long2143 || dt == Long3412)
    {
        qlonglong tmp = ( qlonglong )value.toDouble();
        if ( tmp + 1 < -2147483647 || tmp > 2147483647 )
            return false;

        v->value = QVariant( tmp );
    }
    else if ( dt == DWord || dt ==  ULong || dt == Timer
              || dt == ULong4321 || dt == ULong2143 || dt == ULong3412)
    {
        qlonglong tmp = ( qlonglong )value.toDouble();
        if ( tmp  < 0 || tmp > 4294967295UL )
            return false;

        v->value = QVariant( tmp );
    }
    else if ( dt == Real || dt == Float || dt == Double
              || dt == Real4321 || dt == Real2143 || dt == Real3412)
    {
        v->value = QVariant( value.toDouble() );
    }
    else if ( dt == Bool )
    {
        int tmp = ( int )value.toDouble();
        if ( tmp != 0 && tmp != 1 )
            return false;

        v->value = QVariant( tmp );
    }
    else if ( dt == StringChar || dt == String )
    {
        uint len = v->len;
        if (( unsigned int )( value.toString().length() ) > len )
        {
            v->value = value.toString().left( len );
        }
        else
            v->value = value;
    }
    else
        v->value = value;

    return true;
}


/*!
    \brief 产生模拟量报警和离散量报警,以事件的形式发送给报警管理器。
    \param 指向系统变量的结构体的指针
    \param 系统变量新值
*/
void ProjectRunner::alarmDisposal( HMI::Variable *v, const QVariant &oldVarValue )
{
    Q_CHECK_PTR( v );
    if ( !v->isConnected || v->dataType == String  || v->dataType == DateTime )
        return;

    double newValue = v->value.toDouble();
    //AlarmInfo alarmInfo;
    int variableId = v->varId;
	int alarm_open_or_close = INVALID;	// 0开启，1解除
	// 只用于第一次连接，旧值无效时，对报警产生解除的判断
	static bool firstConnect = true; 
    if ( varidAnalogAlarm.contains( variableId ) ) // 模拟量报警处理
    {
        QList<std::shared_ptr<HMI::Alarm>> list = varidAnalogAlarm.value( variableId );
        foreach( std::shared_ptr<HMI::Alarm> analogAlarm, list )
        {
			alarm_open_or_close = INVALID;	
            double limitValue = 0;
            signed char limitFlag = analogAlarm->limitFlag;
            double limit = analogAlarm->limitValue;
			//::writelog(InfoType,"limitttttttttt:%lf",limit);
            if ( limitFlag == 0 )
                continue;
            // 限制值为常量时,limitValue为常量的限制值
            else if ( limitFlag == 1 )
                limitValue = limit;
            // 限制值为变量时,limitValue为触发变量的id，限制值要通过此id读取
            else if ( limitFlag == 2 )
                limitValue = tagValue( (int)limit ).toDouble();

            bool trigModal = analogAlarm->trigMode;
            int id = 0;
#if 0
			qDebug() << "alarmid = " << analogAlarm->alarmid;
			qDebug() << "oldValue = " << oldVarValue.toDouble();
			qDebug() << "newValue = " << newValue;
			qDebug() << "limitValue = " << analogAlarm->limitValue;
			qDebug() << "trigVarId= " << analogAlarm->trigVarId;
			qDebug() << "=============";
#endif
            if ( trigModal == 1 )// 产生上升沿报警
            {
                int alarmMode = analogAlarm->alarmMode;
                if (alarmMode == 0)  //产生和消除时各提醒一次
                {
                    //if ( (firstConnect && newValue > limitValue) ||
							//(( oldVarValue.toDouble() <= limitValue ) && ( newValue > limitValue )) ) //报警产生
					if ( (firstConnect && SmallerOfDouble(limitValue,newValue)) ||
							((SmallerAndEqualOfDouble(oldVarValue.toDouble(),limitValue) &&
							  SmallerOfDouble(limitValue,newValue))))
                    {
                        // 模拟量报警id向左移一位,离散量报警id向左移一位再加一,产生唯一id
						alarm_open_or_close = PRODUCE;
                        id = analogAlarm->alarmid;
                        //qDebug() << "模拟量上升沿报警 id = " << id;
                        //qDebug() << "报警文本: " << analogAlarm->alarmText;
                    }
					else if ( (firstConnect && SmallerAndEqualOfDouble(limitValue,newValue)) ||
							((SmallerOfDouble(limitValue,oldVarValue.toDouble()) &&
							  SmallerAndEqualOfDouble(newValue,limitValue))))
                    //else if ((firstConnect && newValue <= limitValue) ||
						//(( oldVarValue.toDouble() > limitValue ) && ( newValue <= limitValue ) )) //报警解除
                    {
                        // 模拟量报警id向左移一位,离散量报警id向左移一位再加一,产生唯一id
						alarm_open_or_close = RELEASE;
                        id = analogAlarm->alarmid;
                        //qDebug() << "模拟量上升沿报警解除 id = " << id;
                        //qDebug() << "报警文本: " << analogAlarm->alarmText;
                    }
                }
                else if (alarmMode == 1) //周期报警
                {
                    //if (newValue > limitValue) //报警产生
					if(SmallerOfDouble(limitValue,newValue))
                    {
                        // 模拟量报警id向左移一位,离散量报警id向左移一位再加一,产生唯一id
						alarm_open_or_close = PRODUCE;
                        id = analogAlarm->alarmid;
                        //qDebug() << "模拟量上升沿报警 id = " << id;
                        //qDebug() << "报警文本: " << analogAlarm->alarmText;
                    }
                }
            }
            else  // 产生下降沿报警
            {
                int alarmMode = analogAlarm->alarmMode;
                if (alarmMode == 0)  //产生和消除时各提醒一次
                {
                    //if ((firstConnect && newValue < limitValue) ||
							//(( oldVarValue.toDouble() >= limitValue ) && ( newValue < limitValue ) )) //报警产生
					if ( (firstConnect && SmallerOfDouble(newValue,limitValue)) ||
							((SmallerAndEqualOfDouble(limitValue,oldVarValue.toDouble()) &&
							  SmallerOfDouble(newValue,limitValue))))
                    {
                        // 模拟量报警id向左移一位,离散量报警id向左移一位再加一,产生唯一id
						alarm_open_or_close = PRODUCE;
                        id = analogAlarm->alarmid;
                        //qDebug() << "模拟量下降沿报警 id = " << id;
                        //qDebug() << "报警文本: " << analogAlarm->alarmText;
                    }
                    //else if ((firstConnect && newValue >= limitValue) ||
							//(( oldVarValue.toDouble() < limitValue ) && ( newValue >= limitValue ) )) //报警解除
					if ( (firstConnect && SmallerAndEqualOfDouble(limitValue,newValue)) ||
							((SmallerOfDouble(oldVarValue.toDouble(),limitValue) &&
							  SmallerAndEqualOfDouble(limitValue,newValue))))
                    {
                        // 模拟量报警id向左移一位,离散量报警id向左移一位再加一,产生唯一id
						alarm_open_or_close = RELEASE;
                        id = analogAlarm->alarmid;
                        //qDebug() << "模拟量下降沿报警解除 id = " << id;
                        //qDebug() << "报警文本: " << analogAlarm->alarmText;
                    }
                }
                else if (alarmMode == 1) //周期报警
                {
                    //if (newValue < limitValue) //报警产生
					if(SmallerOfDouble(newValue,limitValue))
                    {
                        // 模拟量报警id向左移一位,离散量报警id向左移一位再加一,产生唯一id
						alarm_open_or_close = PRODUCE;	// 0开启，1解除
                        id = analogAlarm->alarmid;
                        //qDebug() << "模拟量下降沿报警 id = " << id;
                        //qDebug() << "报警文本: " << analogAlarm->alarmText;
                    }
                }
            }
			if(alarm_open_or_close != INVALID){
				// 报警数据发给微信公众号服务器
				alarmPublish(prj->sceneid,analogAlarm->alarmid,analogAlarm->alarmText, uint8_t(alarm_open_or_close), ANAGOLY); 
				// 报警通知发到mico客户端
				alarmNotifyPublish(prj->sceneid,analogAlarm->alarmid,analogAlarm->alarmText,uint8_t(alarm_open_or_close)); 
			}
        }
    }

    // 离散量报警处理
    if ( variddiscreteAlarm.contains( variableId ) )
    {
        unsigned short int newValue = ( unsigned short int )v->value.toInt();
        unsigned short int oldValue = ( unsigned short int )oldVarValue.toInt();
        QList<std::shared_ptr<HMI::Alarm>> list = variddiscreteAlarm.value( variableId );
        foreach( std::shared_ptr<HMI::Alarm> discreteAlarm, list )
        {
			alarm_open_or_close = INVALID;	// 0开启，1解除
            signed char trigBit = discreteAlarm->trigBit;
            if ( trigBit == -1 )
                return;

            //bool trigModal = discreteAlarm->trigMode;
            int id = 0;

			//signed int index = ( signed int )trigBit;
			//short int p = ( short int )pow( 2, index );

			//if (( varValue & p ) != 0 && ( oldValue & p ) == 0 )
#if 0
			qDebug() << "*************";
			qDebug() << "alarmid = " << discreteAlarm->alarmid;
			qDebug() << "oldValue = " << oldVarValue.toDouble();
			qDebug() << "newValue = " << newValue;
			qDebug() << "trigBit = " << trigBit;
			qDebug() << "oldlimitvalue = " << discreteAlarm->limitValue;
			qDebug() << "trigVarId= " << discreteAlarm->trigVarId;
			qDebug() << "*************";
			unsigned short int limitValue = (unsigned short int)tagValue((int)discreteAlarm->trigVarId).toInt(); 
			qDebug() << "reallimitvalue = " << limitValue;
#endif
			int alarmMode = discreteAlarm->alarmMode;
			if (alarmMode == 0)  //产生和消除时各提醒一次
			{
				if ( (firstConnect && TESTBIT(newValue,trigBit)) ||
						(TESTBIT( newValue, trigBit ) && !TESTBIT( oldValue, trigBit ) )) //报警产生
				{
					// 触发位发生改变,新值为1
					alarm_open_or_close = PRODUCE;	// 0开启，1解除
					id = discreteAlarm->alarmid;
					//qDebug() << "离散量报警 id = " << id;
					//qDebug() << "报警文本: " << discreteAlarm->alarmText;
				}
				else if (  (firstConnect && !TESTBIT(newValue,trigBit)) ||
						(!TESTBIT( newValue, trigBit ) && TESTBIT( oldValue, trigBit ) )) //报警解除
				{
					// 触发位发生改变,新值为0
					alarm_open_or_close = RELEASE;	// 0开启，1解除
					id = discreteAlarm->alarmid;
					//qDebug() << "离散量报警解除 id = " << id;
					//qDebug() << "报警文本: " << discreteAlarm->alarmText;
				}
			}
			else if (alarmMode == 1) //周期报警
			{
				if (TESTBIT( newValue, trigBit )) //报警产生
				{
					// 触发位新值为1
					alarm_open_or_close = PRODUCE;	// 0开启，1解除
					id = discreteAlarm->alarmid;
					//qDebug() << "离散量上升沿报警 id = " << id;
					//qDebug() << "报警文本: " << discreteAlarm->alarmText;
				}
			}
            
			if(alarm_open_or_close != INVALID){
				// 报警数据发给微信公众号服务器
				alarmPublish(prj->sceneid,discreteAlarm->alarmid,discreteAlarm->alarmText, uint8_t(alarm_open_or_close), 
						DISCRETE); 
				// 报警通知发到mico客户端
				alarmNotifyPublish(prj->sceneid,discreteAlarm->alarmid,discreteAlarm->alarmText,uint8_t(alarm_open_or_close)); 
			}
        }
    }
	// 只用于断线重连或者第一次连接读取PLC内存值 
	firstConnect = false;
}

/*!
  \brief 接口函数,获取变量的值
  \param 变量Id
  \return 对应变量的值
  */
QVariant ProjectRunner::tagValue( int tagId )
{
	Q_ASSERT( tagId > 0 );
	QVariant value = 0;

	if(!idvar.contains(tagId))
		return value;
	auto v = &idvar[tagId];
	if ( v )
	{
		if ( v->isConnected || !v->isOnline )
			value = v->value;
		else
			value = "#####";
	}
	else
		value = 0;

	return  value;
}

TagDataType ProjectRunner::tagDataType(quint32 id)
{
	auto it = idvar.find(id);
	if (it != idvar.end())
		return (TagDataType)it->dataType;
	else
		return TagDataType::NoType;
}

int ProjectRunner::read_plc_var( int id, void *buf, int len)
{
	if (!idvar.contains(id))
		return -2;

	int rid = id;
	return readValue( rid, (char * )buf, len );//变量的id号是一组从0开始的且连续的自然数.
}

int ProjectRunner::readValue(int id, char *buf, int sizeofbuf)
{
	return prj->m_hmiservice.readDevVar(id, (uchar *)buf, sizeofbuf);
	//return VAR_NO_ERR;
}

int ProjectRunner::getComMsg(QList<struct ComMsg> &c)
{
	return prj->m_hmiservice.getComMsg(c);
	//return 1;
}

// 打包通知客户端的报警消息
void ProjectRunner::alarmNotifyPublish(quint64 id,quint16 alarmId,QString alarmText,quint8 alarmEventType){
    rapidjson::Document v;
    v.SetObject();
    v.AddMember("id", (uint64_t)id, v.GetAllocator());
    v.AddMember("alarmId", (uint32_t)alarmId, v.GetAllocator());
	std::string tmp = alarmText.toStdString();
    v.AddMember("alarmText", rapidjson::StringRef(tmp.c_str()), v.GetAllocator());
    //v.AddMember("alarmText", rapidjson::StringRef(alarmText.toStdString().c_str()), v.GetAllocator());
	// 当前时间
	//std::string alarmTime = TimeOfNow::GetTimeOfNow();
	uint64_t currenttime = time(NULL);
	//::writelog(InfoType,"currenttime:%llu", currenttime);
    v.AddMember("alarmTime", currenttime, v.GetAllocator());
    v.AddMember("alarmEventType", (uint32_t)alarmEventType, v.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    v.Accept(writer);
    const char* output = buffer.GetString();
    ::writelog(InfoType, "alarmNotifyPublish: %s", output);

	RabbitMqService::get_alarm_notify()->publish(output);
}

void ProjectRunner::alarmPublish(quint64 id,quint16 alarmId,QString alarmText, quint8 alarmEventType, int alarmtype){
	rapidjson::Document v;
	v.SetObject();
	v.AddMember("id",(uint64_t)id,v.GetAllocator());
	std::string stralarmid = std::to_string((int)alarmId);
	if(alarmtype == ANAGOLY){
		stralarmid += "(模拟量)";
	}
	else{
		stralarmid += "(离散量)";
	}
	//v.AddMember("alarmId",(uint32_t)alarmId, v.GetAllocator());
	v.AddMember("alarmId",rapidjson::StringRef(stralarmid.c_str()), v.GetAllocator());
	std::string tmp = alarmText.toStdString();
    v.AddMember("alarmText", rapidjson::StringRef(tmp.c_str()), v.GetAllocator());
	//v.AddMember("alarmText",rapidjson::StringRef(alarmText.toStdString().c_str()), v.GetAllocator());
	std::string alarmTime = TimeOfNow::GetTimeOfNow();
	v.AddMember("alarmTime",rapidjson::StringRef(alarmTime.c_str()), v.GetAllocator());
	v.AddMember("alarmEventType",(uint32_t)alarmEventType, v.GetAllocator());

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	v.Accept(writer);
	const char* output = buffer.GetString();

	RabbitMqService::get_alarm()->publish(output);
}

