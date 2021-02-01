#include <iostream>
#include <stdio.h>
#include <assert.h>
#include "../runningscenedb.h"
#include "mysqlcli/mysqlconnection.h"
#include "mysqlcli/mysqlconnpool.h"
#include "mysqlcli//sqlconnpara.h"

int main() {
    SqlParam sp;
    sp.db = "CTCloud";
    sp.host = "127.0.0.1";
    sp.passwd = "123";
    sp.user = "wqs";
    sp.port = 3306;
    MysqlConnPool p(10, sp);
    RunningSceneDbaccess rd(&p);
    RunningSceneData rsd;
    rsd.sceneid = 3;
    rsd.startuserid = 4;
    rsd.runningserverid = 5;
    rsd.phone = "123";
    rsd.blobscenedata = "1234";
    int r = rd.insertRunningScene(rsd);
    std::cout << r << std::endl;
    
    rd.setServeridOfRunningScene(3, 7);

    // get
    RunningSceneData rsdget;
    r = rd.getRunningScene(3, &rsdget);
    printf("%llu, %d, %llu, %s, %s\n", rsdget.sceneid,
            rsdget.startuserid,
            rsdget.runningserverid,
            rsdget.phone.c_str(),
            rsdget.blobscenedata.c_str());
    assert(rsdget.runningserverid == 7);

    std::list<RunningSceneData> lrsd;
    rd.getRunningSceneOfServerid(5, &lrsd);
    for (auto &v : lrsd) {
        printf("get by serverid: %llu, %llu, %d, %s, %s\n",
                v.sceneid,
                v.startuserid,
                v.runningserverid,
                v.phone.c_str(),
                v.blobscenedata.c_str());
    }

    RunningSceneData rsdget2;
    rd.getRunningSceneWithoutScenedata(3, &rsdget2);
    printf("get without data : %llu, %llu, %d, %s, %s\n",
            rsdget2.sceneid,
            rsdget2.startuserid,
            rsdget2.runningserverid,
            rsdget2.phone.c_str(),
            rsdget2.blobscenedata.c_str());

    r = rd.removeRunningScene(3);
    std::cout << "remove result: " << r << std::endl;
    return 0;
}
