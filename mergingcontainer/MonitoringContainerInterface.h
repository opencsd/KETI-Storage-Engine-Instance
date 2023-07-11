#pragma once
#include <grpcpp/grpcpp.h>
#include "storage_engine_instance.grpc.pb.h"
#include <iostream>

using namespace std;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using StorageEngineInstance::Request;
using StorageEngineInstance::TableBlockCount;
using StorageEngineInstance::MonitoringContainer;

class Monitoring_Container_Interface {
	public:
		Monitoring_Container_Interface(std::shared_ptr<Channel> channel) : stub_(MonitoringContainer::NewStub(channel)) {}

		int GetCSDBlockInfo(int qid, int wid) {
			Request request;
			TableBlockCount result;
    		ClientContext context;
			
            request.set_query_id(qid);
            request.set_work_id(wid);
			Status status = stub_->GetCSDBlockInfo(&context, request, &result);
			// cout << "result : " << result.value() << endl;
			
	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}

			return result.table_block_count();
		}

	private:
		std::unique_ptr<MonitoringContainer::Stub> stub_;
		inline const static std::string LOGTAG = "Merging Container::Monitoring Container Interface";
};
