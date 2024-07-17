#include <grpcpp/grpcpp.h>
#include "storage_engine_instance.grpc.pb.h"

#include "table_manager.h"
#include "wal_handler.h"
#include "lba2pba_handler.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;
using grpc::ServerAsyncResponseWriter;
using grpc::CompletionQueue;

using StorageEngineInstance::MonitoringModule;
using StorageEngineInstance::Request;
using StorageEngineInstance::Response;
using StorageEngineInstance::WALRequest;
using StorageEngineInstance::MetaDataRequest;
using StorageEngineInstance::SnippetMetaData;
using StorageEngineInstance::SnippetMetaData_PBAInfo;

class MonitoringModuleServiceImpl final : public MonitoringModule::Service {
  Status GetSnippetMetaData(ServerContext *context, const MetaDataRequest *request, SnippetMetaData *response) override {
    KETILOG::INFOLOG("Monitoring", "# get snippet metadata");

    int total_block_count = 0;
    map<string,SnippetMetaData_PBAInfo> sst_pba_map;

    TableManager::RequestTablePBA(*request, total_block_count, sst_pba_map);

    // TableManager::TempRequestFilteredPBA(*request, total_block_count, sst_pba_map);

    WALRequest walRequest;
    string wal_deleted_key_json;
    vector<string> wal_inserted_row_json;
    int sst_count = request->scan_info().sst_csd_map_size();
    
    walRequest.set_db_name(request->db_name());
    walRequest.set_table_name(request->table_name());

    // WALHandler wh(grpc::CreateChannel((string)STORAGE_NODE_IP+":"+(string)WAL_MANAGER_PORT, grpc::InsecureChannelCredentials()));
    // wh.RequestWAL(walRequest, sst_count, wal_deleted_key_json, wal_inserted_row_json); 

    int index = 0;
    for(const auto entry : sst_pba_map){
      response->mutable_sst_pba_map()->insert({entry.first, entry.second});
      // response->add_wal_inserted_row_json(wal_inserted_row_json[index]);
      index++;
    }
    response->set_table_total_block_count(total_block_count);
    // response->set_wal_deleted_key_json(wal_deleted_key_json);

    return Status::OK;
  }
};

void RunGRPCServer() {
  std::string server_address((std::string)LOCALHOST+":"+std::to_string(SE_MONITORING_PORT));
  MonitoringModuleServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());

  KETILOG::FATALLOG("Monitoring", "Monitoring Server listening on "+server_address);

  server->Wait();
}

int main(int argc, char** argv) {
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

  TableManager::InitTableManager();

  std::thread grpc_thread(RunGRPCServer);

  httplib::Server server;
  server.Get("/log-level", KETILOG::HandleSetLogLevel);
  server.listen("0.0.0.0", 40208);

  grpc_thread.join();

  return 0;
}