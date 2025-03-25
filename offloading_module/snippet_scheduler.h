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
#include <condition_variable>
#include <queue>
#include <thread>
#include <climits>
#include <stdlib.h>
#include <time.h>
#include <filesystem>

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

// namespace fs = std::filesystem;

//Tmax Lib
#include "tb_block.h"
#include "ex.h"
#define BUFF_SIZE 4096
#define BLOCK_UPPER_BOUND 10000
#define SST_UPPER_BOUND 16

using namespace std;
using namespace rapidjson;

using StorageEngineInstance::SnippetRequest;
using StorageEngineInstance::SnippetRequest_SstInfo;

using StorageEngineInstance::TmaxRequest;
using StorageEngineInstance::TmaxResponse;

class Scheduler{
  public: 
    static void PushQueue(SnippetRequest snippet){
      GetInstance().SnippetQueue_.push_work(snippet);
    }

    static SnippetRequest PopQueue(){
      SnippetRequest scheduling_target = GetInstance().SnippetQueue_.wait_and_pop();
      return scheduling_target;
    }

    static void T_snippet_scheduling(TmaxRequest request, TmaxResponse tResponse){
      GetInstance().t_snippet_scheduling(request, tResponse);
      return;
    }

    static void HandleSetAlgorithm(const httplib::Request& request, httplib::Response& response) {
        try {
            std::string algorithm = request.get_param_value("algorithm");
            GetInstance().setAlgorithm(algorithm);
            KETILOG::INFOLOG("Handle Set Algorithm", "snippet scheduling algorithm changed to "+algorithm);
            response.status = 200;
        }catch (std::exception &e) {
            KETILOG::INFOLOG("Handle Set Algorithm", e.what());
            response.status = 501;
        }
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

    void setAlgorithm(string algorithm){
      std::transform(algorithm.begin(), algorithm.end(), algorithm.begin(),[](unsigned char algorithm) { return std::tolower(algorithm); });

      if(algorithm == "dcs"){
        SCHEDULING_ALGORITHM = DCS;
      }else if(algorithm == "dsi"){
        SCHEDULING_ALGORITHM = DSI;
      }else if(algorithm == "dfa"){
        SCHEDULING_ALGORITHM = DFA;
      }else if(algorithm == "auto"){
        SCHEDULING_ALGORITHM = AUTO_SELECTION;
      }else{
        KETILOG::INFOLOG(LOGTAG, "set algorithm not available");
      }
    }

    void runScheduler();
    float dfs_for_dsi_algorithm(const google::protobuf::RepeatedPtrField<StorageEngineInstance::SnippetRequest_SstInfo>* sst_info, stack<string>& global_csd, stack<string>& selected_csd, int si);
    void getBestCSD(const google::protobuf::RepeatedPtrField<StorageEngineInstance::SnippetRequest_SstInfo>* sst_info, map<string,string>& bestcsd);
  
    void DCS_Algorithm(const google::protobuf::RepeatedPtrField<StorageEngineInstance::SnippetRequest_SstInfo>* sst_info, map<string,string>& bestcsd); //Depends on CSD Status
    void DSI_Algorithm(const google::protobuf::RepeatedPtrField<StorageEngineInstance::SnippetRequest_SstInfo>* sst_info, map<string,string>& bestcsd); //Depends on Snippet Information 
    void DFA_Algorithm(const google::protobuf::RepeatedPtrField<StorageEngineInstance::SnippetRequest_SstInfo>* sst_info, map<string,string>& bestcsd);
    void Auto_Selection(const google::protobuf::RepeatedPtrField<StorageEngineInstance::SnippetRequest_SstInfo>* sst_info, map<string,string>& bestcsd);
    
    void t_snippet_scheduling(TmaxRequest request, TmaxResponse tResponse);
    void t_offloading_snippet(TmaxRequest request, TmaxResponse tResponse, string csd_id, string file_name);
  
  public:

    inline const static std::string LOGTAG = "Offloading::Snippet Scheduler";

  private:
    thread SchedulerThread_;
    kQueue<SnippetRequest> SnippetQueue_;
    int SCHEDULING_ALGORITHM;
};

inline std::string Base64Encode(const std::string& input) {
    static const char encode_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    size_t padding = (3 - input.size() % 3) % 3;

    for (size_t i = 0; i < input.size(); i += 3) {
        uint32_t block = (static_cast<uint8_t>(input[i]) << 16);
        if (i + 1 < input.size()) block |= (static_cast<uint8_t>(input[i + 1]) << 8);
        if (i + 2 < input.size()) block |= static_cast<uint8_t>(input[i + 2]);

        output += encode_table[(block >> 18) & 0x3F];
        output += encode_table[(block >> 12) & 0x3F];
        output += (i + 1 < input.size() ? encode_table[(block >> 6) & 0x3F] : '=');
        output += (i + 2 < input.size() ? encode_table[block & 0x3F] : '=');
    }

    return output;
}