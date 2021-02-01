#include <assert.h>
#include "util/logwriter.h"
#include "testclusterdal.h"
#include "../dbaccess/cluster/clusterdal.h"

void TestClusterDal::testLimit()
{
    ::writelog("\n");
    ::writelog("--TestClusterDal");
    ClusterDal cd;
    assert(cd.userCount(144115188075955873ll) == 3);
    assert(cd.deviceCount(144115188075955873ll) == 4);
    // user
    cd.m_maxuser=1;
    assert(!cd.canAddUser(144115188075955873ll));
    cd.m_maxuser=3;
    assert(!cd.canAddUser(144115188075955873ll));
    cd.m_maxuser=4;
    assert(cd.canAddUser(144115188075955873ll));

    // dev
    cd.m_maxDevice = 1;
    assert(!cd.canAddDevice(144115188075955873ll, 1));
    cd.m_maxDevice = 4;
    assert(!cd.canAddDevice(144115188075955873ll, 1));
    cd.m_maxDevice = 5;
    assert(cd.canAddDevice(144115188075955873ll, 1));
    ::writelog("ok");
}
