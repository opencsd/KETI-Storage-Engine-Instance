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
using StorageEngineInstance::Request;
using StorageEngineInstance::SnippetMetaData;
using StorageEngineInstance::DataFileInfo;

class Monitoring_Module_Connector {
	public:
		Monitoring_Module_Connector(std::shared_ptr<Channel> channel) : stub_(MonitoringModule::NewStub(channel)) {}

		DataFileInfo GetDataFileInfo(string dbname, string tname) {
			ClientContext context;
			Request request;
			DataFileInfo response;
			
			request.set_db_name(dbname);
			request.set_table_name(tname);
			Status status = stub_->GetDataFileInfo(&context, request, &response);
			
	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}

			return response;
		}

		SnippetMetaData GetSnippetMetaData(string dbname, string tname, map<string,string> sst_csd_map) {
			ClientContext context;
			Request request;
			SnippetMetaData response;
			
			request.set_db_name(dbname);
			request.set_table_name(tname);

			for (const auto& [key, value] : sst_csd_map) {
				request.mutable_sst_csd_map()->insert({key, value});
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
		inline const static std::string LOGTAG = "Offloading Container::Monitoring Container Interface";
};
