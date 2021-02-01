
#ifdef Q_WS_QWS
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/sem.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#endif
#include "commservice.h"

#define MAX_MS_DAY_TIME       (24*60*60*1000)  // 最长时间24小时
#define MAX_LOOP_TIME         2000
#define MIN_LOOP_TIME         500

CommService::CommService()
{
    m_time = QTime::currentTime();
    //m_time.start();
    m_timeTicks = 0;
}

CommService::~CommService()
{   //在此处释放线程有风险，有可能释放的过程被主线程打断，并且线程的退出exit后要用wait，等待它完全退出
    //因此将线程的释放挪到创建它的对象里，以解决反复启动场景运行会死机的问题，以下代码不要
    //this->thread()->exit();
    //this->thread()->disconnect();
    //this->thread()->deleteLater();
}

int CommService::getCurrTime()
{
#ifdef Q_WS_QWS
    //int currTime = 0;
    //struct timeval tv;
    //gettimeofday(&tv, NULL);
    //currTime = (1000*(tv.tv_sec%3600) + tv.tv_usec/1000);
#endif

    //return m_time.elapsed();
    QTime currTime = QTime::currentTime();
    int msecond = m_time.msecsTo( currTime );

    if ( msecond >= 0 && msecond <= MAX_LOOP_TIME )
        m_timeTicks += msecond;
    else if (( MAX_MS_DAY_TIME + msecond ) > 0 && ( MAX_MS_DAY_TIME + msecond ) <  MAX_LOOP_TIME )
        m_timeTicks += ( MAX_MS_DAY_TIME + msecond );
    else
        m_timeTicks += MIN_LOOP_TIME ;

    if ( m_timeTicks > MAX_MS_DAY_TIME )
        m_timeTicks -= MAX_MS_DAY_TIME;

    m_time = currTime;

    return m_timeTicks;
}

int CommService::calIntervalTime( int lastTime, int currTime )
{
    if ( lastTime == -1 )
        return MAX_MS_DAY_TIME;
    else
    {
        if ( currTime >= lastTime )
            return ( currTime - lastTime );
        else
            return ( MAX_MS_DAY_TIME + currTime - lastTime );
    }
}


int CommService::getDataTypeLen( INT8U type )
{
    if ( type == Com_Bool || type == Com_BitGroup || type == Com_Char || type == Com_BitBlock4 || type == Com_BitBlock8
                    || type == Com_Byte || type == Com_String || type == Com_StringChar || type == Com_DateTime
                    || type == Com_S7300_String || type == Com_PointArea )
        return 1;
    else if ( type == Com_Int || type == Com_UInt || type == Com_Word || type == Com_BitBlock12 || type == Com_BitBlock16
            || type == Com_S7300_Counter || type == Com_S7300_Timer || type == Com_S7300_Date || type == Com_Int21 || type == Com_UInt21)
        return 2;
    else if ( type == Com_Long || type == Com_DInt || type == Com_ULong || type == Com_DWord || type == Com_BitBlock20 || type == Com_BitBlock24
                    || type == Com_BitBlock28 || type == Com_BitBlock32 || type == Com_Float || type == Com_Real ||  type == Com_Timer
                    || type == Com_S7300_Time || type == Com_S7300_TimeofDay ||
              type == Com_Long4321 || type == Com_Long2143 || type == Com_Long3412 || type == Com_ULong4321 || type == Com_ULong2143 || type == Com_ULong3412 || type == Com_Real4321 || type == Com_Real2143 ||  type == Com_Real3412)
        return 4;
    else if ( type == Com_Double || type == Com_DateTime)
        return 8;
    else
        return -1;
}

/*void CommService::combineVars(QList<struct HmiVar*>& waitUpdateVarsList, QList<struct HmiVar*>& combinVarsList,
                              struct HmiVar& combinVar, int maxDistance, int maxLen)
{
    combinVarsList.clear();
    if (waitUpdateVarsList.count() >= 1)
    {
        struct HmiVar* pHmiVar = waitUpdateVarsList[0];
        combinVarsList.append(pHmiVar);
        combinVar = *pHmiVar;
        qDebug() << "combineVars" << pHmiVar->varId;
    }
    for (int i = 1; i < waitUpdateVarsList.count(); i++)
    {
        struct HmiVar* pHmiVar = waitUpdateVarsList[i];
        if (pHmiVar->area == combinVar.area) //Bool型长度不对
        {
            int startOffset = qMin(combinVar.addOffset, pHmiVar->addOffset);
            int endOffset = qMax(combinVar.addOffset+combinVar.len, pHmiVar->addOffset+pHmiVar->len);

            int newLen = endOffset - startOffset;
            if (newLen <= maxLen)
            {
                if (newLen <= combinVar.len + pHmiVar->len + maxDistance)
                {
                    combinVar.addOffset = startOffset;
                    combinVar.len = newLen;
                    combinVarsList.append(pHmiVar);
                }
            }
            else
                break;
        }
    }
}*/
