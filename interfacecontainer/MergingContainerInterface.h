#pragma once
#include <string>  
#include <iostream> 
#include <sstream>   
#include <iomanip>

#include <grpcpp/grpcpp.h>
#include "storage_engine_instance.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using StorageEngineInstance::MergingContainer;
using StorageEngineInstance::SnippetRequest;
using StorageEngineInstance::Snippet;
using StorageEngineInstance::Request;
using StorageEngineInstance::Result;
using StorageEngineInstance::QueryResult;
using StorageEngineInstance::Column;

using namespace std;

class Merging_Container_Interface {
	public:
		Merging_Container_Interface(std::shared_ptr<Channel> channel) : stub_(MergingContainer::NewStub(channel)) {}

		string Aggregation(SnippetRequest snippet_request) {
			Result result;
    		ClientContext context;
			
			Status status = stub_->Aggregation(&context, snippet_request, &result);
			// cout << "result : " << result.value() << endl;

	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}
			return result.value();
		}

		string InitBuffer(Snippet masked_snippet, SnippetRequest::SnippetType type) {
			SnippetRequest request;
			request.mutable_snippet()->CopyFrom(masked_snippet);
			request.set_type(type);

			Result result;
    		ClientContext context;
			
			Status status = stub_->InitBuffer(&context, request, &result);
			// cout << "result : " << result.value() << endl;

			if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}
			return result.value();
		}

		string GetQueryResult(int qid, string tname) {
			Request request;
			request.set_query_id(qid);
			request.set_table_name(tname);
			
			QueryResult queryResult;
    		ClientContext context;
			
			Status status = stub_->GetQueryResult(&context, request, &queryResult);

	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}

			stringstream result_string;
			int length = 18;
			std::string line((length+1) * queryResult.query_result_size() - 1, '-');
			//Proto Buffer 형식 결과를 임시로 string으로 저장 -> 나중엔 DB Connector Instance 에서 출력		
			result_string << "+"+line+"+\n";
			const auto& my_map = queryResult.query_result();
			
			result_string << "|";
			for (const auto& entry : my_map) {
				result_string << setw(length) << left << entry.first << "|";
			}
			result_string << "\n";

			result_string <<  "+"+line+"+\n";

			for(int i=0; i<queryResult.row_count(); i++){
				result_string << "|";
				for (const auto& entry : my_map) {
					switch(entry.second.col_type()){
						case Column::TYPE_STRING:
							result_string << setw(length) << left << entry.second.string_col(i) << "|";
							break;
						case Column::TYPE_INT:
							result_string << setw(length) << left << to_string(entry.second.int_col(i)) << "|";
							break;
						case Column::TYPE_FLOAT:
							result_string << setw(length) << left << to_string(entry.second.double_col(i)) << "|";
							break;
						case Column::TYPE_EMPTY:
							result_string << setw(length) << left << " " << "|";
							break;
					}
				}
				result_string << "\n";
			}

			result_string <<  "+"+line+"+\n";

			return result_string.str();
		}

		string EndQuery(int qid) {
			Request request;
			request.set_query_id(qid);
			Result result;
    		ClientContext context;
			
			Status status = stub_->EndQuery(&context, request, &result);

	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}
			return result.value();
		}

	private:
		std::unique_ptr<MergingContainer::Stub> stub_;
		inline const static std::string LOGTAG = "Interface Container::Merging Container Interface";
};