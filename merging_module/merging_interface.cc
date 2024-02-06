#include "merge_query_manager.h"
#include <grpcpp/grpcpp.h>
#include "CalculateUsage.h"
#include "httplib.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;

using StorageEngineInstance::MergingModule;
using StorageEngineInstance::Snippet;
using StorageEngineInstance::SnippetRequest;
using StorageEngineInstance::Request;
using StorageEngineInstance::Response;
using StorageEngineInstance::QueryResult;
using StorageEngineInstance::QueryResult_Column;
using StorageEngineInstance::QueryResult_Column_ColType;


class MergingModuleServiceImpl final : public MergingModule::Service {
  Status Aggregation(ServerContext* context, const SnippetRequest* request, Response* response) override {  
    string msg = "==:Aggregation:== {" + to_string(request->snippet().query_id()) + "|" + to_string(request->snippet().work_id()) + "}";
    KETILOG::INFOLOG("Merging",msg);

    // merge query manager instance & run snippet work
    thread MergeQueryInstance = thread(&MergeQueryManager::RunSnippetWork,MergeQueryManager(*request));
    MergeQueryInstance.detach();  
    response->set_value("Aggregation Start");

    return Status::OK;
  }

  Status GetQueryResult(ServerContext* context, const Request* request, QueryResult* result) override {
    string msg = "==:Get Query Result:== {" + to_string(request->query_id()) + "|" + request->table_name() + "}";
    KETILOG::INFOLOG("Merging", msg);

    string key = to_string(request->query_id()) + "|" + request->table_name();
     
    TableData queryResult = BufferManager::GetTableData(request->query_id(),request->table_name());//이걸 막기

    if(KETILOG::IsLogLevelUnder(TRACE)){
      // // 테이블 데이터 로우 수 확인 - Debug Code   
      cout << "<<result table row>>" << endl;
      for(auto i : queryResult.table_data){
          if(i.second.type == TYPE_STRING){
            cout << i.first << "|" << i.second.strvec.size() << endl;
            // cout << i.first << "|" << i.second.strvec[0] << endl;
          }else if(i.second.type == TYPE_INT){
            cout << i.first << "|" << i.second.intvec.size() << endl;
            // cout << i.first << "|" << i.second.intvec[0] << endl;
          }else if(i.second.type == TYPE_FLOAT){
            cout << i.first << "|" << i.second.floatvec.size() << endl;
            // cout << i.first << "|" << i.second.floatvec[0] << endl;
          }else if(i.second.type == TYPE_EMPTY){
            cout << i.first << "| empty row" << endl;
          }else{
            KETILOG::FATALLOG("","target table row else ?");
          }
      }
    }

    if(queryResult.valid){
      for (auto td : queryResult.table_data){
        string col_name = td.first;
        QueryResult_Column col;

        switch (td.second.type){
          case TYPE_STRING:{
            col.set_col_type(QueryResult_Column_ColType::QueryResult_Column_ColType_TYPE_STRING);  
            for(size_t i=0; i<td.second.strvec.size(); i++){
              col.add_string_col(td.second.strvec[i]);
            }
            break;
          }case TYPE_INT:{
            col.set_col_type(QueryResult_Column_ColType::QueryResult_Column_ColType_TYPE_INT);    
            for(size_t i=0; i<td.second.intvec.size(); i++){
              col.add_int_col(td.second.intvec[i]);
            }
            break;
          }case TYPE_FLOAT:{
            col.set_col_type(QueryResult_Column_ColType::QueryResult_Column_ColType_TYPE_FLOAT);  
            for(size_t i=0; i<td.second.floatvec.size(); i++){
              col.add_double_col(td.second.floatvec[i]);
            }
            break;
          }case TYPE_EMPTY:{
            col.set_col_type(QueryResult_Column_ColType::QueryResult_Column_ColType_TYPE_EMPTY);
            break;
          }default:{
            KETILOG::FATALLOG("Merging","GetQueryResult Type Error");
          }
        }
        result->mutable_query_result()->insert({col_name, col});
      }
      result->set_row_count(queryResult.row_count);
      result->set_scanned_row_count(queryResult.scanned_row_count);
      result->set_filtered_row_count(queryResult.filtered_row_count);
    }

    BufferManager::EndQuery(*request);

    return Status::OK;
  }
  
  // Status InitBuffer(ServerContext* context, const SnippetRequest* request, Response* response) override {  
  //   string msg = "==:Init Buffer:== {" + to_string(request->snippet().query_id()) + "|" + to_string(request->snippet().work_id()) + "}";
  //   KETILOG::INFOLOG("Merging Container",msg);

  //   // thread BufferInitWorkInstance = thread(&BufferManager::InitWork,request->type(), request->snippet());
  //   // BufferInitWorkInstance.detach();  
  //   BufferManager::InitWork(request->type(), request->snippet());
 
  //   response->set_value("Init Success");

  //   return Status::OK;
  // }

  // Status EndQuery(ServerContext* context, const Request* request, Response* response) override {
  //   string msg = "==:End Query:== {" + to_string(request->query_id()) + "}";
  //   KETILOG::INFOLOG("Merging Container",msg);

  //   BufferManager::EndQuery(*request);

  //   response->set_value("End Query");
  //   return Status::OK;
  // }
};

void RunGRPCServer() {
  std::string server_address((std::string)LOCALHOST+":"+std::to_string(SE_MERGING_PORT));
  MergingModuleServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());

  KETILOG::WARNLOG("Merging","Merging Module Server Listening on "+server_address);

  server->Wait();
}

int main(int argc, char** argv){
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
  
  BufferManager::InitBufferManager(); //Run Buffer Manager TCP/IP Server

  std::thread grpc_thread(RunGRPCServer);//Run Merge Query Manager & Buffer Manager gRPC Server

  httplib::Server server;
  server.Get("/log-level", KETILOG::HandleSetLogLevel);
  server.listen("0.0.0.0", 40207);

  grpc_thread.join();

  return 0;
}