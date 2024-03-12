#pragma once

#include <iostream>

#include <grpcpp/grpcpp.h>
#include "storage_engine_instance.grpc.pb.h"
#include "keti_log.h"

using namespace std;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using StorageEngineInstance::MonitoringModule;
using StorageEngineInstance::MetaDataRequest;
using StorageEngineInstance::SnippetMetaData;
using StorageEngineInstance::ScanInfo;
using StorageEngineInstance::ScanInfo_SSTInfo;

class MonitoringModuleConnector {
	public:
		MonitoringModuleConnector(std::shared_ptr<Channel> channel) : stub_(MonitoringModule::NewStub(channel)) {}

		SnippetMetaData GetSnippetMetaData(string dbname, string tname, ScanInfo scanInfo, map<string,string> sst_csd_map) {
			KETILOG::DEBUGLOG(LOGTAG,"# get snippet metadata");

			ClientContext context;
			MetaDataRequest request;
			SnippetMetaData response;
			
			request.set_db_name(dbname);
			request.set_table_name(tname);
			request.mutable_scan_info()->CopyFrom(scanInfo);

			for(int i=0; i<request.scan_info().block_info_size(); i++){
				string sst_name = request.scan_info().block_info(i).sst_name();
				request.mutable_scan_info()->mutable_block_info(i)->clear_csd_list();
				request.mutable_scan_info()->mutable_block_info(i)->add_csd_list(sst_csd_map[sst_name]);
			}
			
			Status status = stub_->GetSnippetMetaData(&context, request, &response);
			
	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}

			return response;
		}

	private:
		std::unique_ptr<MonitoringModule::Stub> stub_;
		inline const static std::string LOGTAG = "Offloading::Monitoring Connector";
};
