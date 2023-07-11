#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ip_config.h"
#include "SnippetManager.h"
#include "CalculateUsage.h"

#include <grpcpp/grpcpp.h>
#include "storage_engine_instance.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;
using StorageEngineInstance::InterfaceContainer;
using StorageEngineInstance::Snippet;
using StorageEngineInstance::SnippetRequest;
using StorageEngineInstance::Result;
using StorageEngineInstance::Request;

using namespace std;

// Logic and data behind the server's behavior.
class InterfaceContainerServiceImpl final : public InterfaceContainer::Service {
  Status SetSnippet(ServerContext* context,
                   ServerReaderWriter<Result, SnippetRequest>* stream) override {
    SnippetRequest snippet_request;
    bool flag = true;

    while (stream->Read(&snippet_request)) {      
      if(flag){
        string msg = "==:Set Snippet:== {" + to_string(snippet_request.snippet().query_id()) + "}";
        KETILOG::INFOLOG("Interface Container", msg);
        SnippetManager::InsertQueryID(snippet_request.snippet().query_id());
        flag = false;
      } 

      // Check Recv Snippet
      {
        // std::string test_json;
        // google::protobuf::util::JsonPrintOptions options;
        // options.always_print_primitive_fields = true;
        // options.always_print_enums_as_ints = true;
        // google::protobuf::util::MessageToJsonString(snippet_request,&test_json,options);
        // std::cout << endl << test_json << std::endl << std::endl; 
      }

      SnippetManager::PushQueue(snippet_request);
    }

    SnippetManager::SetTableName(snippet_request.snippet().query_id(), snippet_request.snippet().table_alias());

    return Status::OK;
  }

  Status Run(ServerContext* context, const Request* request, Result* result) override {
    // CalculateStart();
    int query_id = request->query_id();
    string table_name = SnippetManager::GetTableName(query_id);

    KETILOG::INFOLOG("Interface Container", "==:RUN:== {" + to_string(query_id) + "}");
    // sleep(1+rand()%5);

    Merging_Container_Interface mc(grpc::CreateChannel((std::string)LOCALHOST+":"+std::to_string(SE_MERGING_CONTAINER_PORT), grpc::InsecureChannelCredentials()));
    string query_result = mc.GetQueryResult(query_id, table_name);
    
    result->set_value(query_result);

    SnippetManager::EraseQueryID(query_id);
    mc.EndQuery(query_id);
    // CalculateEnd();
    return Status::OK;
  }
};


void RunServer() {
  std::string server_address((std::string)LOCALHOST+":"+std::to_string(SE_INTERFACE_CONTAINER_PORT));
  InterfaceContainerServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());

  KETILOG::WARNLOG("Interface Container","Storage Engine Interface Server Listening on "+server_address);

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