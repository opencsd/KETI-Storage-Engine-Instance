#pragma once
#include <vector>
#include <iostream>
#include <sstream>
#include <string.h>
#include <unordered_map>
#include <sys/socket.h>
#include <cstdlib>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <condition_variable>
#include <queue>
#include <thread>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h" 

#include "storage_engine_instance.grpc.pb.h"

#include "internal_queue.h"
#include "keti_log.h"
#include "keti_type.h"
#include "ip_config.h"
#include "csd_status_manager.h"
#include "snippet_manager.h"

#define BUFF_SIZE 4096

using namespace std;
using namespace rapidjson;

using StorageEngineInstance::Snippet;
using StorageEngineInstance::SnippetRequest;
using StorageEngineInstance::ScanInfo;

class Scheduler{
  public: 
    static void PushQueue(SnippetRequest snippet){
      GetInstance().SnippetQueue_.push_work(snippet);
    }

    static SnippetRequest PopQueue(){
      SnippetRequest scheduling_target = GetInstance().SnippetQueue_.wait_and_pop();
      return scheduling_target;
    }

  private:
    Scheduler() {
      SCHEDULING_ALGORITHM = ROUND_ROBBIN;
      std::thread SchedulerThread_(&Scheduler::runScheduler,this);
      SchedulerThread_.detach();
    };
    Scheduler(const Scheduler&);
    ~Scheduler() {};
    Scheduler& operator=(const Scheduler&){
        return *this;
    }

    static Scheduler& GetInstance(){
        static Scheduler scheduler;
        return scheduler;
    }

    void runScheduler();
    map<string,string> getBestCSD(ScanInfo scanInfo);
    map<string,string> CSDMetricScore(ScanInfo scanInfo); //CSD 병렬 처리 우선
    map<string,string> FileDistribution(ScanInfo scanInfo); //CSD 순서대로
    map<string,string> RoundRobbin(ScanInfo scanInfo); //CSD 병렬 처리 우선
    map<string,string> AlgorithmAutoSelection(ScanInfo scanInfo); //CSD 순서대로
  
  public:
    inline const static std::string LOGTAG = "Offloading::Snippet Scheduler";

  private:
    thread SchedulerThread_;
    kQueue<SnippetRequest> SnippetQueue_;
    int SCHEDULING_ALGORITHM;
};
