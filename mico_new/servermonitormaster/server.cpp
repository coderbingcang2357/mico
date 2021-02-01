// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// A server to receive EchoRequest and send back EchoResponse.

#include <butil/logging.h>
#include <brpc/server.h>
#include "serverrpc.h"
#include "runnerregisterserviceimp.h"
#include "runnernodemanager.h"
#include "mysqlcli/mysqlconnpool.h"
#include "config/configreader.h"
#include "util/logwriter.h"

int main(int argc, char* argv[]) {
	bool runasdaemon = false;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-daemon") == 0)
        {
            runasdaemon = true;
        }
        else if (strcmp(argv[i], "-test") == 0)
        {
            //return test();
        }
	}

    if (runasdaemon)
    {
        loginit(mointormaster);
        int err = daemon(1, // don't change dic
                1); // don't redirect stdin, stdout, stderr
        if (err == -1)
        {
            ::writelog(InfoType, "Daemon Failed: %s", strerror(errno));
            exit(1);
        }
    } 
    logSetPrefix("Mointormaster Server");

	//read config
	char configpath[1024];
	Configure *config = Configure::getConfig();
	Configure::getConfigFilePath(configpath, sizeof(configpath));

	if (config->read(configpath) != 0)
	{
		exit(-1);
	}
	// Generally you only need one Server.
    brpc::Server server;
    RunnernodeManager nodes;
    RunnerRegisterServiceImp runnerregister(&nodes);

    SqlParam sp;
	sp.db = Configure::getConfig()->notifysrv_sql_db;
    sp.host = Configure::getConfig()->notifysrv_sql_host;
    sp.port = Configure::getConfig()->notifysrv_sql_port;
    sp.user = Configure::getConfig()->notifysrv_sql_user;
    sp.passwd = Configure::getConfig()->notifysrv_sql_pwd;
    MysqlConnPool pool(100, sp);
    RunningSceneDbaccess dbaccess(&pool);
    SceneRepo scenerepo(&nodes, &dbaccess);
    // Instance of your service.
    SceneRunner::SceneManagerRpc serverrpc(&scenerepo);

    // Add the service into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use brpc::SERVER_OWNS_SERVICE.
    if (server.AddService(&serverrpc,
                          brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }
    if (server.AddService(&runnerregister,
                          brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }

    // Start the server.
    brpc::ServerOptions options;
    options.idle_timeout_sec = -1;
    if (server.Start(stoi(Configure::getConfig()->masternode_port), &options) != 0) {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
