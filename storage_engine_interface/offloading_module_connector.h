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
using StorageEngineInstance::OffloadingModule;
using StorageEngineInstance::SnippetRequest;
using StorageEngineInstance::Response; 
using StorageEngineInstance::CSDMetricList; 
using StorageEngineInstance::TmaxRequest; 
using StorageEngineInstance::TmaxResponse; 
using StorageEngineInstance::Empty; 

using namespace std;

class OffloadingModuleConnector {
	public:
		OffloadingModuleConnector(std::shared_ptr<Channel> channel) : stub_(OffloadingModule::NewStub(channel)) {}

		void Scheduling(SnippetRequest snippet) {
			KETILOG::DEBUGLOG(LOGTAG, "# send scan snippet");

			Response response;
    		ClientContext context;
			
			Status status = stub_->Scheduling(&context, snippet, &response);

	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}
			return;
		}

		void PushCSDMetric(CSDMetricList csdMetricList) {
			Response response;
    		ClientContext context;
			
			Status status = stub_->PushCSDMetric(&context, csdMetricList, &response);

	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}
			return;
		}

		TmaxResponse t_send_snippet(TmaxRequest tRequest){
			TmaxResponse tResponse;
    		ClientContext context;

			KETILOG::DEBUGLOG(LOGTAG, "<T> send parsed snippet to scheduling");
			
			Status status = stub_->t_snippet_scheduling(&context, tRequest, &tResponse);

	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}

			return tResponse;
		}

		CSDMetricList t_get_csd_status(){
			CSDMetricList response;
    		ClientContext context;
			Empty empty;

			KETILOG::DEBUGLOG(LOGTAG, "<T> send parsed snippet to scheduling");
			
			Status status = stub_->t_get_csd_status(&context, empty, &response);

	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}

			return response;
		}

	private:
		std::unique_ptr<OffloadingModule::Stub> stub_;
		inline const static std::string LOGTAG = "Interface::Offloading Connector";
};