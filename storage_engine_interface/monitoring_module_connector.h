#pragma once
#include <string>  
#include <iostream> 
#include <sstream>   
#include <iomanip>

#include <grpcpp/grpcpp.h>
#include "storage_engine_instance.grpc.pb.h"
#include "keti_log.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using StorageEngineInstance::MonitoringModule;
using StorageEngineInstance::DBInfo;
using StorageEngineInstance::DataFileInfo;
using StorageEngineInstance::Response;  

using namespace std;

class MonitoringModuleConnector {
	public:
		MonitoringModuleConnector(std::shared_ptr<Channel> channel) : stub_(MonitoringModule::NewStub(channel)) {}

		void SyncMetaDataManager(DBInfo dbInfo) {
			Response response;
    		ClientContext context;
			
			Status status = stub_->SyncMetaDataManager(&context, dbInfo, &response);

	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}
			return;
		}

	private:
		std::unique_ptr<MonitoringModule::Stub> stub_;
		inline const static std::string LOGTAG = "Interface";
};