#pragma once
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <grpcpp/grpcpp.h>
#include "storage_engine_instance.grpc.pb.h"
 
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using StorageEngineInstance::OffloadingContainer;
using StorageEngineInstance::CSDStatusList;
using StorageEngineInstance::CSDStatusList_CSDStatus;
using StorageEngineInstance::Result;

using namespace std;

struct CSDStatus {
	string ip = "";
	float score = 0;
	int block_count = 0;
};

class Offloading_Container_Interface {
	public:
		Offloading_Container_Interface(std::shared_ptr<Channel> channel) : stub_(OffloadingContainer::NewStub(channel)) {}

		std::string PushCSDMetric(unordered_map<string,CSDStatus> csd_status_list) {
			CSDStatusList csdStatusList;
			Result result;
    		ClientContext context;

			for (const auto pair : csd_status_list) {
				string id = pair.first;
				CSDStatusList_CSDStatus csdStatus;
				csdStatus.set_ip(pair.second.ip);
				csdStatus.set_score(pair.second.score);
				// csdStatus.set_block_count(pair.second.block_count);
				csdStatusList.mutable_csd_status_map()->insert({id,csdStatus});
            }
			
			Status status = stub_->PushCSDMetric(&context, csdStatusList, &result);

	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}
			return result.value();
		}

	private:
		std::unique_ptr<OffloadingContainer::Stub> stub_;
		inline const static std::string LOGTAG = "Monitoring Container::Offloading Container Interface";
};
