#pragma once
#include <grpcpp/grpcpp.h>
#include <google/protobuf/util/field_mask_util.h>
#include "storage_engine_instance.grpc.pb.h"
 
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using google::protobuf::FieldMask;
using StorageEngineInstance::MonitoringContainer;
using StorageEngineInstance::Response;
using StorageEngineInstance::Snippet;
using StorageEngineInstance::SnippetRequest;

class Monitoring_Container_Interface {
	public:
		Monitoring_Container_Interface(std::shared_ptr<Channel> channel) : stub_(MonitoringContainer::NewStub(channel)) {}

		std::string SetMetaData(SnippetRequest snippet_request) {
			FieldMask mask;

			if(snippet_request.type() == StorageEngineInstance::SnippetRequest::CSD_SCAN_SNIPPET) {
				mask.add_paths("query_ID");
				mask.add_paths("work_ID");
				mask.add_paths("table_name");
            } /*else if(snippet_request.type() == StorageEngineInstance::SnippetRequest_SnippetType_INDEX_SCAN_SNIPPET) {
				mask.add_paths("query_ID");
				mask.add_paths("work_ID");
				mask.add_paths("table_name");
				mask.add_paths("table_filter");
            }*/
            
            Snippet masked_snippet = snippet_request.snippet();
            google::protobuf::util::FieldMaskUtil::TrimMessage(mask, &masked_snippet);

            Response response;
    		ClientContext context;
			
			Status status = stub_->SetMetaData(&context, masked_snippet, &response);
			// std::cout << "result : " << result.value() << std::endl;

	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}
			return response.value();
		}
	private:
		std::unique_ptr<MonitoringContainer::Stub> stub_;
		inline const static std::string LOGTAG = "Interface Container::Monitoring Container Interface";
};
