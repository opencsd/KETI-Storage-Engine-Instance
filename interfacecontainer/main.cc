#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>

#include "ip_config.h"
#include "SnippetManager.h"
#include "httplib.h"

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
using StorageEngineInstance::Response;

using namespace std;

void SendQueryStatus(const char* message){
  int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // Create socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        // return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        // return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        // return -1;
    }

    // Send message to the server
    send(sock, message, strlen(message), 0);
    std::cout << "Query info sent" << std::endl;

    // Receive message from the server
    valread = read(sock, buffer, 1024);
    std::cout << buffer << std::endl;
}

// Logic and data behind the server's behavior.
class InterfaceContainerServiceImpl final : public InterfaceContainer::Service {
  Status SetSnippet(ServerContext* context, ServerReaderWriter<Response, SnippetRequest>* stream) override {
    SnippetRequest snippet_request;
    bool flag = true;

    if(KETILOG::IsLogLevel(METRIC)){
      const char* Qstart = "Query Start";
      SendQueryStatus(Qstart);
    }

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
    int query_id = request->query_id();
    string table_name = SnippetManager::GetTableName(query_id);

    KETILOG::INFOLOG("Interface Container", "==:RUN:== {" + to_string(query_id) + "}");

    Merging_Container_Interface mc(grpc::CreateChannel((std::string)LOCALHOST+":"+std::to_string(SE_MERGING_CONTAINER_PORT), grpc::InsecureChannelCredentials()));
    Result result_ = mc.GetQueryResult(query_id, table_name);
    
    result->set_query_result(result_.query_result());
    result->set_scanned_row_count(result_.scanned_row_count());
    result->set_filtered_row_count(result_.filtered_row_count());

    SnippetManager::EraseQueryID(query_id); //이후 캐시 사용시 고려 필요
    mc.EndQuery(query_id);

    if(KETILOG::IsLogLevel(METRIC)){
      const char* Qend = "Query End";
      SendQueryStatus(Qend);
    }

    return Status::OK;
  }

  Status SetSnippetAndRun(ServerContext* context, grpc::ServerReader<SnippetRequest>* reader, Result* result) override {
    SnippetRequest snippet_request;
    bool flag = true;

    if(KETILOG::IsLogLevel(METRIC)){
      const char* Qstart = "Query Start";
      SendQueryStatus(Qstart);
    }

    while (reader->Read(&snippet_request)) {      
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

    int query_id = snippet_request.snippet().query_id();
    string table_name = SnippetManager::GetTableName(query_id);

    KETILOG::INFOLOG("Interface Container", "==:RUN:== {" + to_string(query_id) + "}");

    Merging_Container_Interface mc(grpc::CreateChannel((std::string)LOCALHOST+":"+std::to_string(SE_MERGING_CONTAINER_PORT), grpc::InsecureChannelCredentials()));
    Result result_ = mc.GetQueryResult(query_id, table_name);
    
    result->set_query_result(result_.query_result());
    result->set_scanned_row_count(result_.scanned_row_count());
    result->set_filtered_row_count(result_.filtered_row_count());

    SnippetManager::EraseQueryID(query_id); //이후 캐시 사용시 고려 필요
    mc.EndQuery(query_id);

    if(KETILOG::IsLogLevel(METRIC)){
      const char* Qend = "Query End";
      SendQueryStatus(Qend);
    }

    return Status::OK;
  }
};

void RunGRPCServer() {
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
          log_level = DEBUGG_LEVEL::TRACE;
      }
      KETILOG::SetLogLevel(log_level);
  }else{
      KETILOG::SetDefaultLogLevel();
  }

  std::thread grpc_thread(RunGRPCServer);

  httplib::Server server;
  server.Get("/log-level", KETILOG::HandleSetLogLevel);
  server.listen("0.0.0.0", 40205);

  grpc_thread.join();

  return 0;
}