#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// #include "MonitoringContainerInterface.h"
#include "SnippetScheduler.h"
#include "CSDStatusManager.h"
#include "keti_log.h"
#include "CalculateUsage.h"

#include <grpcpp/grpcpp.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;

using StorageEngineInstance::OffloadingContainer;
using StorageEngineInstance::Snippet;
using StorageEngineInstance::MetaDataResponse;
using StorageEngineInstance::Result;
using StorageEngineInstance::CSDStatusList;
using StorageEngineInstance::CSDStatusList_CSDStatus;

// Logic and data behind the server's behavior.
class OffloadingContainerServiceImpl final : public OffloadingContainer::Service {
  Status Schedule(ServerContext* context, const Snippet* snippet, Result* result) override {
    KETILOG::INFOLOG("Offloading Container", "==:Receive Snippet from Interface Container:==");
    // CalculateStart();
    
    Object schedulingTarget;
    
    // MetaDataResponse schedulerinfo;

    // Monitoring_Container_Interface mc(grpc::CreateChannel((std::string)LOCALHOST+":"+std::to_string(SE_MONITORING_CONTAINER_PORT), grpc::InsecureChannelCredentials()));
    // schedulerinfo = mc.GetMetaData(snippet->query_id(), snippet->work_id(), snippet->table_name(0));

    schedulingTarget.snippet = *snippet;
    // schedulingTarget.metadata = schedulerinfo;

    Scheduler::PushQueue(schedulingTarget);

    result->set_value("Snippet Scheduling OK");
    // CalculateEnd();
    return Status::OK;
  }
  Status PushCSDMetric(ServerContext* context, const CSDStatusList* csdStatusList, Result* result) override {
    // KETILOG("Offloading Container", "==:Push CSD Metric:==");

    const auto status = csdStatusList->csd_status_map();
    for (const auto entry : status) {
      CSDStatusManager::CSDInfo csdInfo;
      string id = entry.first;
      csdInfo.csd_ip = entry.second.ip();
      csdInfo.csd_score = entry.second.score();
      csdInfo.block_count = entry.second.block_count();

      {
      // Debugg Code
      cout << "CSD IP: " << csdInfo.csd_ip;
      cout << ", CSD Score: " << csdInfo.csd_score;
      cout << ", Block Count: " << csdInfo.block_count << endl;
      }

      CSDStatusManager::SetCSDInfo(id, csdInfo);
    }
    
    result->set_value("CSD Status Manager OK");

    return Status::OK;
  }
};

void RunServer() {
  std::string server_address((std::string)LOCALHOST+":"+std::to_string(SE_OFFLOADING_CONTAINER_PORT));
  OffloadingContainerServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());

  KETILOG::WARNLOG("Offloading Container", "Snippet Scheduler Server listening on "+server_address);
  
  server->Wait();
}


int main(int argc, char** argv) {
  if (argc >= 2) {
    KETILOG::SetLogLevel(stoi(argv[1]));
  }else{
    KETILOG::SetDefaultLogLevel();
  }
  
  RunServer();

  return 0;
}
