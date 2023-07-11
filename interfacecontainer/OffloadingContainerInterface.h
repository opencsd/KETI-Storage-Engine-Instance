#pragma once
#include <grpcpp/grpcpp.h>
#include "storage_engine_instance.grpc.pb.h"
 
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using StorageEngineInstance::OffloadingContainer;
using StorageEngineInstance::Snippet;
using StorageEngineInstance::Result;

class Offloading_Container_Interface {
	public:
		Offloading_Container_Interface(std::shared_ptr<Channel> channel) : stub_(OffloadingContainer::NewStub(channel)) {}

		std::string Schedule(Snippet snippet) {
			Result result;
    		ClientContext context;
			
			Status status = stub_->Schedule(&context, snippet, &result);
			// cout << "result : " << result.value() << endl;

	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}
			return result.value();
		}

	private:
		std::unique_ptr<OffloadingContainer::Stub> stub_;
		inline const static std::string LOGTAG = "Interface Container::Offloading Container Interface";
};
