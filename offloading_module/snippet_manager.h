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

#include "keti_log.h"
#include "keti_type.h"
#include "ip_config.h"

#define BUFF_SIZE 4096

using namespace std;
using namespace rapidjson;

using StorageEngineInstance::Snippet;
using StorageEngineInstance::SnippetRequest;
using StorageEngineInstance::ScanInfo;

class SnippetManager{
  public: 
    static void SetupSnippet(SnippetRequest snippet, map<string,string> bestcsd){
      return GetInstance().setupSnippet(snippet, bestcsd);
    }

    static SnippetManager& GetInstance(){
        static SnippetManager snippetManager;
        return snippetManager;
    }

  private:
    SnippetManager() {};
    SnippetManager(const SnippetManager&);
    ~SnippetManager() {};
    SnippetManager& operator=(const SnippetManager&){
        return *this;
    }

    void setupSnippet(SnippetRequest snippet, map<string,string> bestcsd);
    void serialize(StringBuffer &snippetbuf, Snippet snippet, string csd, string pba, int csd_block_count, int table_total_block_count); // snippet -> json 구성
    void sendSnippetToCSD(string snippet_json); // CSD 전달
  
  public:
    inline const static std::string LOGTAG = "Offloading::Snippet Scheduler";

};
