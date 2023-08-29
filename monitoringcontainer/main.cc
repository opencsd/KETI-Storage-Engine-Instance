#include <grpcpp/grpcpp.h>
#include <google/protobuf/util/field_mask_util.h>
#include "storage_engine_instance.grpc.pb.h"

#include "TableManager.h"
#include "LBA2PBAQueryAgent.h"
#include "WALQueryAgent.h"
#include "MetricAnalysisModule.h"
#include "IndexManager.h"

#include "CalculateUsage.h"

using google::protobuf::FieldMask;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;
using grpc::ServerAsyncResponseWriter;
using grpc::CompletionQueue;

using StorageEngineInstance::MonitoringContainer;
using StorageEngineInstance::Request;
using StorageEngineInstance::Result;
using StorageEngineInstance::MetaDataResponse;
using StorageEngineInstance::TableBlockCount;
using StorageEngineInstance::CSDMetricList;
using StorageEngineInstance::CSDMetricList_CSDMetric;
using StorageEngineInstance::Snippet;

// Logic and data behind the server's behavior.
class MonitoringContainerServiceImpl final : public MonitoringContainer::Service {
  Status SetMetaData(ServerContext* context, const Snippet* snippet, Result* result) override {
    //PBA 정보 요청, WAL 정보 요청, 결과 구성
    KETILOG::INFOLOG("Monitoring Container", "=: Set Meta Data :=");
    // this_thread::sleep_for(std::chrono::milliseconds(100));
    
    
    string key = TableManager::makeKey(snippet->query_id(),snippet->work_id());
    TableManager::SetReturnData(key);

    Request request_;
    request_.set_query_id(snippet->query_id());
    request_.set_work_id(snippet->work_id());
    request_.set_table_name(snippet->table_name(0));

    InternalRequest internal_request;
    internal_request.query_id = snippet->query_id();
    internal_request.work_id = snippet->work_id();
    internal_request.table_name = snippet->table_name(0);

    bool temp_index_scan = false;

    // Snippet snippet_;

    // TableManager::SetResponseSnippet(snippet_,key);
    // TableManager::GetReturnData(key)->snippet->CopyFrom(new_snippet);
    TableManager::GetReturnData(key)->wal_done = true;
    // WALManager::PushQueue(request_);

    if(!temp_index_scan){ //full scan
      TableManager::GetReturnData(key)->index_scan_done = true;
      LBA2PBAQueryAgent::PushQueue(request_);
    }else{ //index scan
      IndexManager::PushQueue(request_);
    }
    
    result->set_value("Set Meta Data Start");

    return Status::OK;
  }
  Status GetMetaData(ServerContext* context, const Request* request, MetaDataResponse* result) override {
    // call from offloading container; 저장된 메타 정보 전달, 구성 완료 전이면 대기
    KETILOG::INFOLOG("Monitoring Container", "=: Get Meta Data :=");
    
    string key = TableManager::makeKey(request->query_id(),request->work_id());
    Response* data = TableManager::GetReturnData(key);
    
    unique_lock<mutex> lock(data->mu);
    while (!data->lba2pba_done || !data->wal_done){
      data->cond.wait(lock);
    }
    
    result->CopyFrom(*data->metadataResponse);
    return Status::OK;
  }
  Status GetCSDBlockInfo(ServerContext* context, const Request* request, TableBlockCount* result) override {
    // call from merging container; 저장된 블록 수 정보 전달, 구성 완료 전이면 대기
    KETILOG::INFOLOG("Monitoring Container", "=: Get CSD Block Info :=");
    
    string key = TableManager::makeKey(request->query_id(),request->work_id());
    Response* data = TableManager::GetReturnData(key);

    unique_lock<mutex> lock(data->mu);
    while (!data->lba2pba_done || !data->wal_done){
      data->cond.wait(lock);
    }
    result->set_table_block_count(data->block_count);
    
    return Status::OK;
  }
  Status SetCSDMetricsInfo(ServerContext *context, const CSDMetricList *csdMetricList, Result *result) override {
    KETILOG::INFOLOG("Monitoring Container", "=: Set CSD Metrics Info :=");
    for (int i = 0; i < csdMetricList->csd_metric_list_size(); i++){
      StorageEngineMetricCollector::CSDMetric newCSD;

      string csd_id =  csdMetricList->csd_metric_list(i).id();
      newCSD.ip = csdMetricList->csd_metric_list(i).ip();
      newCSD.cpu_usage = csdMetricList->csd_metric_list(i).cpu_usage();
      newCSD.mem_usage = csdMetricList->csd_metric_list(i).memory_usage();
      newCSD.disk_usage = csdMetricList->csd_metric_list(i).disk_usage();
      newCSD.network = csdMetricList->csd_metric_list(i).network();
      newCSD.block_count = csdMetricList->csd_metric_list(i).working_block_count();

      {
      // // Debugg Code
      // cout << "CSD IP: " << newCSD.ip;
      // cout << ", CPU Usage: " << newCSD.cpu_usage;
      // cout << "%, Memory Usage: " << newCSD.mem_usage;
      // cout << "%, disk Usage: " << newCSD.disk_usage;
      // cout << "%, network: " <<  newCSD.network << endl;
      }
      
      StorageEngineMetricCollector::SetCSDMetric(csd_id,newCSD);
    }

    result->set_value("metric set success");

    return Status::OK;
  }
};

void RunServer() {
  std::string server_address((std::string)LOCALHOST+":"+std::to_string(SE_MONITORING_CONTAINER_PORT));
  MonitoringContainerServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());

  KETILOG::WARNLOG("Monitoring Container", "Monitoring Container Server listening on "+server_address);

  server->Wait();
}

int main(int argc, char** argv) {
  if (argc >= 2) {
    KETILOG::SetLogLevel(stoi(argv[1]));
  }else{
    KETILOG::SetDefaultLogLevel();
  }

  MetricAnalysisModule& instance = MetricAnalysisModule::GetInstance();
  TableManager::InitTableManager();
  WALQueryAgent::InitWALQueryAgent();
  LBA2PBAQueryAgent::InitLBA2PBAQueryAgent();
  IndexManager::InitIndexManager();
  RunServer();

  return 0;
}