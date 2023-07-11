#pragma once
#include <grpcpp/grpcpp.h>
#include "storage_engine_instance.grpc.pb.h"
#include <iostream>
#include "keti_log.h"

using namespace std;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using StorageEngineInstance::Request;
using StorageEngineInstance::MetaDataResponse;
using StorageEngineInstance::MonitoringContainer;

class Monitoring_Container_Interface {
	public:
		Monitoring_Container_Interface(std::shared_ptr<Channel> channel) : stub_(MonitoringContainer::NewStub(channel)) {}

		MetaDataResponse GetMetaData(int qid, int wid, string tname) {
			ClientContext context;
			Request request;
			MetaDataResponse response;
			
            request.set_query_id(qid);
            request.set_work_id(wid);
			request.set_table_name(tname);
			Status status = stub_->GetMetaData(&context, request, &response);
			// cout << "result : " << result.value() << endl;
			
	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}

			return response;
		}

	private:
		std::unique_ptr<MonitoringContainer::Stub> stub_;
		inline const static std::string LOGTAG = "Offloading Container::Monitoring Container Interface";
};
