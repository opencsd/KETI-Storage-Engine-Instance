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
#include <climits>
#include <stdlib.h>
#include <time.h>

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

using StorageEngineInstance::SnippetRequest;
using StorageEngineInstance::SnippetRequest_SstInfo;

using StorageEngineInstance::TmaxRequest;

class Scheduler{
  public: 
    static void PushQueue(SnippetRequest snippet){
      GetInstance().SnippetQueue_.push_work(snippet);
    }

    static SnippetRequest PopQueue(){
      SnippetRequest scheduling_target = GetInstance().SnippetQueue_.wait_and_pop();
      return scheduling_target;
    }

    static void T_snippet_scheduling(TmaxRequest request){
      GetInstance().t_snippet_scheduling(request);
      return;
    }

    static Scheduler& GetInstance(){
      static Scheduler scheduler;
      return scheduler;
    }
  private:
    Scheduler() {
      SCHEDULING_ALGORITHM = DCS;
      std::thread SchedulerThread_(&Scheduler::runScheduler,this);
      SchedulerThread_.detach();
    };
    Scheduler(const Scheduler&);
    ~Scheduler() {};
    Scheduler& operator=(const Scheduler&){
        return *this;
    }

    void runScheduler();
    void getBestCSD(const StorageEngineInstance::SnippetRequest_SstInfo* sst_info, map<string,string>& bestcsd);
  
    string DCS_algorithm(vector<string> dcs_candidate_csd); //DSI용
    void DCS_Algorithm(const StorageEngineInstance::SnippetRequest_SstInfo* sst_info, map<string,string>& bestcsd); //Depends on CSD Status
    void DSI_Algorithm(const StorageEngineInstance::SnippetRequest_SstInfo* sst_info, map<string,string>& bestcsd); //Depends on Snippet Information 
    void Random(const StorageEngineInstance::SnippetRequest_SstInfo* sst_info, map<string,string>& bestcsd);
    void Auto_Selection(const StorageEngineInstance::SnippetRequest_SstInfo* sst_info, map<string,string>& bestcsd);
    
    void t_snippet_scheduling(TmaxRequest request);
    void t_offloading_snippet(TmaxRequest request, string csd_id);
  
  public:

    inline const static std::string LOGTAG = "Offloading::Snippet Scheduler";

  private:
    thread SchedulerThread_;
    kQueue<SnippetRequest> SnippetQueue_;
    int SCHEDULING_ALGORITHM;
};
