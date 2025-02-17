#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "merging_module_connector.h"
#include "offloading_module_connector.h"
#include "ip_config.h"

//Tmax Lib
#include "tb_block.h"
#include "ex.h"

#include <grpcpp/grpcpp.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::Status;
using StorageEngineInstance::StorageEngineInterface;
using StorageEngineInstance::SnippetRequest;
using StorageEngineInstance::QueryStringResult;
using StorageEngineInstance::Response;
using StorageEngineInstance::CSDMetricList;
using StorageEngineInstance::TmaxRequest;
using StorageEngineInstance::TmaxResponse;
using StorageEngineInstance::Empty;
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
class StorageEngineInterfaceServiceImpl final : public StorageEngineInterface::Service {
  Status OffloadingQueryInterface(ServerContext* context, ServerReader<SnippetRequest>* stream, QueryStringResult* result) override {
    KETILOG::INFOLOG("Interface", "# receive offload query from query engine");
    
    SnippetRequest snippet_request;

    if(KETILOG::IsLogLevel(METRIC)){
      const char* Qstart = "Query Start";
      SendQueryStatus(Qstart);
    }

    bool flag = true;
    while (stream->Read(&snippet_request)) {      
      if(flag){
        cout << "[Interface] received offloading snippet {ID:" << snippet_request.query_id() << "}" << endl;
        flag = false;
      }
      // {
      //   std::string test_json;
      //   google::protobuf::util::JsonPrintOptions options;
      //   options.always_print_primitive_fields = true;
      //   options.always_print_enums_as_ints = true;
      //   google::protobuf::util::MessageToJsonString(snippet_request,&test_json,options);
      //   std::cout << endl << test_json << std::endl << std::endl; 
      // }
      if((snippet_request.type() == StorageEngineInstance::SnippetRequest::FULL_SCAN) \
          ||(snippet_request.type() == StorageEngineInstance::SnippetRequest::INDEX_SCAN) \
          || (snippet_request.type() == StorageEngineInstance::SnippetRequest::INDEX_TABLE_SCAN)){
        KETILOG::DEBUGLOG("Interface","# send snippet to offloading module");
        cout << "[Interface] Type:" << snippet_request.type() <<  " > send snippet to offloading module {ID:" << snippet_request.query_id() << "|" << snippet_request.work_id() << "}" << endl;
        OffloadingModuleConnector offloadingModule(grpc::CreateChannel((std::string)LOCALHOST+":"+to_string(SE_OFFLOADING_PORT), grpc::InsecureChannelCredentials()));
        offloadingModule.Scheduling(snippet_request);
      }else{
        KETILOG::DEBUGLOG("Interface","# send snippet to merging module");
        cout << "[Interface] Type:" << snippet_request.type() <<  " > send snippet to merging module {ID:" << snippet_request.query_id() << "|" << snippet_request.work_id() << "}" << endl;
        MergingModuleConnector mergingModule(grpc::CreateChannel((std::string)LOCALHOST+":"+to_string(SE_MERGING_PORT), grpc::InsecureChannelCredentials()));
        mergingModule.Aggregation(snippet_request);
      }
    }
    MergingModuleConnector mergingModule(grpc::CreateChannel((std::string)LOCALHOST+":"+to_string(SE_MERGING_PORT), grpc::InsecureChannelCredentials()));
    QueryStringResult result_ = mergingModule.GetQueryResult(snippet_request.query_id(), snippet_request.work_id(), snippet_request.result_info().table_alias());

    result->CopyFrom(result_);   
      
    return Status::OK;
    
  }

  Status PushCSDMetric(ServerContext *context, const CSDMetricList *request, Response *response) override {
    KETILOG::DEBUGLOG("Interface", "Push CSD Metric");

    OffloadingModuleConnector offloadingModule(grpc::CreateChannel((std::string)LOCALHOST+":"+to_string(SE_OFFLOADING_PORT), grpc::InsecureChannelCredentials()));
    offloadingModule.PushCSDMetric(*request);

    return Status::OK;
  }

  Status keti_send_snippet(ServerContext *context, const TmaxRequest *request, TmaxResponse *response) override {
    KETILOG::DEBUGLOG("Interface", "<T> Tmax DB Server called keti_send_snipet");
    KETILOG::DEBUGLOG("Interface", "<T> parsing tmax snippet request");

    TmaxResponse tResponse;
    
    if(request->type()==TmaxRequest::FO_REQUEST){
      OffloadingModuleConnector offloadingModule(grpc::CreateChannel((std::string)LOCALHOST+":"+to_string(SE_OFFLOADING_PORT), grpc::InsecureChannelCredentials()));
      tResponse = offloadingModule.t_send_snippet(*request);
    }else if(request->type()==TmaxRequest::FETCH_REQUEST){
      MergingModuleConnector mergingModule(grpc::CreateChannel((std::string)LOCALHOST+":"+to_string(SE_MERGING_PORT), grpc::InsecureChannelCredentials()));
      tResponse = mergingModule.GetTmaxQueryResult(*request);
    }

    response->CopyFrom(tResponse);  

    return Status::OK;
  }

  Status keti_get_csd_status(ServerContext *context, const Empty* request, CSDMetricList *response) override {
    KETILOG::DEBUGLOG("Interface", "<T> Tmax DB Server called keti_get_csd_status");

    OffloadingModuleConnector offloadingModule(grpc::CreateChannel((std::string)LOCALHOST+":"+to_string(SE_OFFLOADING_PORT), grpc::InsecureChannelCredentials()));
    offloadingModule.t_get_csd_status();

    return Status::OK;
  }
};

void RunGRPCServer() {
  std::string server_address("0.0.0.0:"+std::to_string(SE_INTERFACE_PORT));
  StorageEngineInterfaceServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  builder.SetMaxSendMessageSize(-1);
  builder.SetMaxReceiveMessageSize(-1);
  std::unique_ptr<Server> server(builder.BuildAndStart());

  KETILOG::FATALLOG("Interface", "Interface Server listening on "+server_address);
  
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
  server.listen("0.0.0.0", 40205);
  
  grpc_thread.join();

  return 0;
}
