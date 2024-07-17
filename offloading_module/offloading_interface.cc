#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "snippet_scheduler.h"

#include <grpcpp/grpcpp.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::Status;

using StorageEngineInstance::OffloadingModule;
using StorageEngineInstance::SnippetRequest;
using StorageEngineInstance::Response;
using StorageEngineInstance::CSDMetricList;
using StorageEngineInstance::TmaxRequest;
using StorageEngineInstance::Empty;
using namespace std;

// Logic and data behind the server's behavior.
class OffloadingModuleServiceImpl final : public OffloadingModule::Service {
  public:
  Status Scheduling(ServerContext *context, const SnippetRequest *snippet, Response *response) override {
    KETILOG::INFOLOG("Interface", "# receive scan snippet");

    {
      std::string test_json;
      google::protobuf::util::JsonPrintOptions options;
      options.always_print_primitive_fields = true;
      options.always_print_enums_as_ints = true;
      google::protobuf::util::MessageToJsonString(*snippet,&test_json,options);
      std::cout << endl << test_json << std::endl << std::endl; 
    }

    Scheduler::PushQueue(*snippet);

    return Status::OK;
  }

  Status PushCSDMetric(ServerContext* context, const CSDMetricList* csdMetricList, Response* response) override {
    KETILOG::DEBUGLOG("Offloading", "Push CSD Metric");

    const auto metric = csdMetricList->csd_metric_list();
    for (const auto entry : metric) {
      CSDStatusManager::CSDInfo csdInfo;
      csdInfo.cpu_usage = entry.cpu_usage();
      csdInfo.memory_usage = entry.memory_usage();
      csdInfo.disk_usage = entry.disk_usage();
      csdInfo.network = entry.network();
      csdInfo.working_block_count = entry.working_block_count();
      csdInfo.analysis_score = entry.score();

      CSDStatusManager::SetCSDInfo(entry.id(), csdInfo);
    }
    
    response->set_value("Success");

    return Status::OK;
  }

  Status t_snippet_scheduling(ServerContext *context, const TmaxRequest *request, Response* response) override {
    KETILOG::DEBUGLOG("Offloading", "<T> called t_snippet_scheduling");

    Scheduler::T_snippet_scheduling(*request);

    return Status::OK;
  }

  Status t_get_csd_status(ServerContext *context, const Empty *request, CSDMetricList* response) override {
    KETILOG::DEBUGLOG("Offloading", "<T> called t_get_server_metric");

    CSDMetricList csd_metric_list = CSDStatusManager::T_get_csd_status();
    response->CopyFrom(csd_metric_list);

    return Status::OK;
  }

};

void RunGRPCServer() {
  std::string server_address((std::string)LOCALHOST+":"+std::to_string(SE_OFFLOADING_PORT));
  OffloadingModuleServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());

  sleep(1);

  KETILOG::FATALLOG("Offloading", "Offloading Server listening on "+server_address);
  
  server->Wait();
}


int main(int argc, char** argv) {
  Scheduler& scheduler = Scheduler::GetInstance();
  if (argc >= 2) {
      KETILOG::SetLogLevel(stoi(argv[1]));
  }else if (getenv("LOG_LEVEL") != NULL){
      string env = getenv("LOG_LEVEL");
      int log_level;
      if (env == "TRACE"){
          log_level = DEBUGG_LEVEL::TRACE;
      }else if (env == "DEBUG"){
          log_level = DEBUGG_LEVEL::DEBUG;
      }else if (env == "INFO"){
          log_level = DEBUGG_LEVEL::INFO;
      }else if (env == "WARN"){
          log_level = DEBUGG_LEVEL::WARN;
      }else if (env == "ERROR"){
          log_level = DEBUGG_LEVEL::ERROR;
      }else if (env == "FATAL"){
          log_level = DEBUGG_LEVEL::FATAL;
      }else{
          log_level = DEBUGG_LEVEL::INFO;
      }
      KETILOG::SetLogLevel(log_level);
  }else{
      KETILOG::SetDefaultLogLevel();
  }

  std::thread grpc_thread(RunGRPCServer);

  httplib::Server server;
  server.Get("/log-level", KETILOG::HandleSetLogLevel);
  server.listen("0.0.0.0", 40206);
  
  grpc_thread.join();
  
  return 0;
}
