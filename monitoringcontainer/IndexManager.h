#pragma once

#include <vector>
#include <unordered_map>
#include <mutex>
#include <fcntl.h>
#include <unistd.h>
#include <iomanip>
#include <algorithm>
#include <condition_variable>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h" 

#include "storage_engine_instance.grpc.pb.h"

#include "keti_log.h"
#include "ip_config.h"
#include "internal_queue.h"
#include "TableManager.h"

using namespace std;
using StorageEngineInstance::Request;

class IndexManager { /* modify as singleton class */

/* Methods */
public:

	static string makeKey(int qid, int wid){
		string key = to_string(qid)+"|"+to_string(wid);
        return key;
    }

	static void InitIndexManager(){
		GetInstance().initIndexManager();
	}

    static void PushQueue(Request request){
        GetInstance().pushQueue(request);
    }

private:
	IndexManager() {};
    IndexManager(const IndexManager&);
    IndexManager& operator=(const IndexManager&){
        return *this;
    };

    static IndexManager& GetInstance() {
        static IndexManager _instance;
        return _instance;
    }

    static void pushQueue(Request request){
        GetInstance().indexManagerQueue.push_work(request);
    }

    static Request popQueue(){
        Request request = GetInstance().indexManagerQueue.wait_and_pop();
        return request;
    }

	void initIndexManager();
    void requestIndexScan();

/* Variables */
public:
    inline const static string LOGTAG = "Monitoring Container::Index Manager";

private:
    mutex Index_Mutex;
	kQueue<Request> indexManagerQueue;
    thread IndexManager_Thread;
};