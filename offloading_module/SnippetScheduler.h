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
#include "CSDStatusManager.h"
#include "MonitoringModuleConnector.h"

#define BUFF_SIZE 4096

using namespace std;
using namespace rapidjson;

using StorageEngineInstance::Snippet;
using StorageEngineInstance::DataFileInfo;
using StorageEngineInstance::DataFileInfo_CSD;

class Scheduler{
  public: 
    static void PushQueue(Snippet snippet){
      GetInstance().SnippetQueue_.push_work(snippet);
    }

    static Snippet PopQueue(){
      Snippet scheduling_target = GetInstance().SnippetQueue_.wait_and_pop();
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
    void scheduling(Snippet snippetToSchedule, map<string,string> bestcsd);
    map<string,string> getBestCSD(DataFileInfo dataFileInfo);
    map<string,string> CSDMetricScore(DataFileInfo dataFileInfo); //CSD 병렬 처리 우선
    map<string,string> FileDistribution(DataFileInfo dataFileInfo); //CSD 순서대로
    map<string,string> RoundRobbin(DataFileInfo dataFileInfo); //CSD 병렬 처리 우선
    map<string,string> AlgorithmAutoSelection(DataFileInfo dataFileInfo); //CSD 순서대로
    void serialize(StringBuffer &snippetbuf, Snippet &snippet, string pba, string csd); // snippet -> json 구성
    void sendSnippetToCSD(string snippet_json); // CSD 전달
  
  public:
    inline const static std::string LOGTAG = "Offloading Container::Snippet Scheduler";

  private:
    thread SchedulerThread_;
    kQueue<Snippet> SnippetQueue_;
    int SCHEDULING_ALGORITHM;
};
