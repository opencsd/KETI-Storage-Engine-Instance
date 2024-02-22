#include <grpcpp/grpcpp.h>
#include <google/protobuf/util/field_mask_util.h>
#include "storage_engine_instance.grpc.pb.h"

#include "table_manager.h"
#include "wal_handler.h"
#include "storage_manager_connector.h"
// #include "IndexManager.h"

using google::protobuf::FieldMask;
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
using StorageEngineInstance::LBA2PBARequest;
using StorageEngineInstance::WALRequest;
using StorageEngineInstance::DataFileInfo;
using StorageEngineInstance::DataFileInfo_CSD;
using StorageEngineInstance::SnippetMetaData;
using StorageEngineInstance::DBInfo;

class MonitoringModuleServiceImpl final : public MonitoringModule::Service {
  Status GetDataFileInfo(ServerContext *context, const Request *request, DataFileInfo *response) override {
    KETILOG::DEBUGLOG("Monitoring", "# called get data file info");

    while(true){
      if(TableManager::IsMetaDataInitialized()){
        break;
      }
      KETILOG::WARNLOG("Monitoring", "Table Manager Not Initialized!");
      sleep(1);
    }
    
    string db_name = request->db_name();
    string table_name = request->table_name();
    vector<string> table_sst_list = TableManager::GetSST(db_name, table_name);

    StorageManagerConnector smc(grpc::CreateChannel((string)STORAGE_CLUSTER_MASTER_IP+":"+(string)STORAGE_MANAGER_PORT, grpc::InsecureChannelCredentials()));
    DataFileInfo dataFileInfo = smc.GetDataFileInfo(table_sst_list);

    {
    // std::string test_json;
    // google::protobuf::util::JsonPrintOptions options;
    // options.always_print_primitive_fields = true;
    // options.always_print_enums_as_ints = true;
    // google::protobuf::util::MessageToJsonString(dataFileInfo,&test_json,options);
    // std::cout << endl << test_json << std::endl << std::endl; 
    }

    response->CopyFrom(dataFileInfo);

    return Status::OK;
  }

  Status GetSnippetMetaData(ServerContext *context, const Request *request, SnippetMetaData *response) override {
    KETILOG::DEBUGLOG("Monitoring", "# called get snippet metadata");

    while(true){
      if(TableManager::IsMetaDataInitialized()){
        break;
      }
      KETILOG::WARNLOG("Monitoring", "Table Manager Not Initialized!");
      sleep(1);
    }
    
    {
    // std::string test_json;
    // google::protobuf::util::JsonPrintOptions options;
    // options.always_print_primitive_fields = true;
    // options.always_print_enums_as_ints = true;
    // google::protobuf::util::MessageToJsonString(*request,&test_json,options);
    // std::cout << endl << test_json << std::endl << std::endl; 
    }

    string db_name = request->db_name();
    string table_name = request->table_name();
    int table_index_number = TableManager::GetTableIndexNumber(db_name, table_name);

    LBA2PBARequest lbaRequest;
    int total_block_count = 0;
    map<string,string> sst_pba_map;

    lbaRequest.set_table_index_number(table_index_number);
    for (const auto entry : request->sst_csd_map()) {
        lbaRequest.mutable_sst_csd_map()->insert({entry.first,entry.second});
    }

    WALRequest walRequest;
    string wal_deleted_key_json;
    vector<string> wal_inserted_row_json;
    int sst_count = request->sst_csd_map_size();
    
    walRequest.set_db_name(db_name);
    walRequest.set_table_name(table_name);

    CompletionQueue *cq;

    StorageManagerConnector smc(grpc::CreateChannel((string)STORAGE_CLUSTER_MASTER_IP+":"+(string)STORAGE_MANAGER_PORT, grpc::InsecureChannelCredentials()));
    smc.RequestPBA(lbaRequest, total_block_count, sst_pba_map, cq); //async로 구현할것!

    // WALHandler wh(grpc::CreateChannel((string)STORAGE_CLUSTER_MASTER_IP+":"+(string)WAL_MANAGER_PORT, grpc::InsecureChannelCredentials()));
    // wh.RequestWAL(walRequest, sst_count, wal_deleted_key_json, wal_inserted_row_json, cq); //async

    // cout << "request wal" << endl;

    // void* got_tag;
    // bool ok = false;
    // GPR_ASSERT(cq->Next(&got_tag, &ok));
    // GPR_ASSERT(got_tag == (void*)1);
    // GPR_ASSERT(ok);

    int index = 0;
    for(const auto entry : sst_pba_map){
      string sst_name = entry.first;
      string pba = entry.second;
      response->mutable_sst_pba_map()->insert({sst_name, pba});
      // response->add_wal_inserted_row_json(wal_inserted_row_json[index]);
      index++;
    }
    response->set_table_total_block_count(total_block_count);
    // response->set_wal_deleted_key_json(wal_deleted_key_json);

    return Status::OK;
  }

  Status SyncMetaDataManager(ServerContext *context, const DBInfo *request, Response *response) override {
    KETILOG::INFOLOG("Monitoring", "metadata sync success");

    for(const auto db : request->db_list()){
      string db_name = db.first;

      TableManager::DB new_db;
      for(const auto table : db.second.table_list()){
        TableManager::Table new_table;
        new_table.sst_list.assign(table.second.sst_list().begin(),table.second.sst_list().end());
        new_table.table_index_number = table.second.table_index_number();

        new_db.table[table.first] = new_table;
      }
      
      TableManager::SetDBInfo(db_name, new_db);
    }

    if(!TableManager::IsMetaDataInitialized()){
      TableManager::SetMetaDataInitialized();
    }

    // TableManager::DumpTableManager();

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

  KETILOG::WARNLOG("Monitoring", "Monitoring Container Server listening on "+server_address);

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

  std::thread grpc_thread(RunGRPCServer);

  httplib::Server server;
  server.Get("/log-level", KETILOG::HandleSetLogLevel);
  server.listen("0.0.0.0", 40208);

  grpc_thread.join();

  return 0;
}