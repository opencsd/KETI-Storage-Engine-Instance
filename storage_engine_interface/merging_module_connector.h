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
using StorageEngineInstance::MergingModule;
using StorageEngineInstance::SnippetRequest;
using StorageEngineInstance::Request;
using StorageEngineInstance::QueryStringResult;
using StorageEngineInstance::Response;
using StorageEngineInstance::QueryResult;
using StorageEngineInstance::QueryResult_Column;
using StorageEngineInstance::TmaxRequest;
using StorageEngineInstance::TmaxResponse;

using namespace std;

class MergingModuleConnector {
	public:
		MergingModuleConnector(std::shared_ptr<Channel> channel) : stub_(MergingModule::NewStub(channel)) {}

		string Aggregation(SnippetRequest snippet_request) {
			KETILOG::DEBUGLOG(LOGTAG, "# send aggregation snippet");

			Response response;
    		ClientContext context;
			
			Status status = stub_->Aggregation(&context, snippet_request, &response);
			// cout << "result : " << result.value() << endl;

	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}
			return response.value();
		}

		QueryStringResult GetQueryResult(int qid, int wid, string tname) {
			KETILOG::DEBUGLOG(LOGTAG, "# get query result");

			Request request;
			request.set_query_id(qid);
			request.set_work_id(wid);
			request.set_table_name(tname);
			
			QueryResult query_result;
    		ClientContext context;
			
			Status status = stub_->GetQueryResult(&context, request, &query_result);

	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}

			stringstream result_string;
			int length = 18;
			std::string line((length+1) * query_result.query_result_size() - 1, '-');
			//Proto Buffer 형식 결과를 임시로 string으로 저장 -> 나중엔 DB Connector Instance 에서 출력		
			result_string << "+"+line+"+\n";
			const auto& my_map = query_result.query_result();
			
			result_string << "|";
			for (const auto& entry : my_map) {
				result_string << setw(length) << left << entry.first << "|";
			}
			result_string << "\n";

			result_string <<  "+"+line+"+\n";

			for(int i=0; i<query_result.row_count(); i++){
				result_string << "|";
				for (const auto& entry : my_map) {
					switch(entry.second.col_type()){
						case QueryResult_Column::TYPE_STRING:
							result_string << setw(length) << left << entry.second.string_col(i) << "|";
							break;
						case QueryResult_Column::TYPE_INT:
							result_string << setw(length) << left << to_string(entry.second.int_col(i)) << "|";
							break;
						case QueryResult_Column::TYPE_FLOAT:
							// result_string <<  std::fixed << std::setprecision(2) << setw(length) << left << entry.second.double_col(i) << "|";
							result_string << setw(length) << left << to_string(entry.second.double_col(i)) << "|";
							break;
						case QueryResult_Column::TYPE_EMPTY:
							result_string << setw(length) << left << " " << "|";
							break;
					}
				}
				result_string << "\n";
			}

			result_string <<  "+"+line+"+\n";

			QueryStringResult result;
			result.set_query_result(result_string.str());
			result.set_scanned_row_count(query_result.scanned_row_count());
			result.set_filtered_row_count(query_result.filtered_row_count());

			return result;
		}

		TmaxResponse GetTmaxQueryResult(TmaxRequest tRequest) {
			KETILOG::DEBUGLOG("Interface::Merging Connector", "<T> GetTmaxQueryResult");

			ClientContext context;
			TmaxResponse tResponse;
			
			Status status = stub_->GetTmaxQueryResult(&context, tRequest, &tResponse);

			if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}

			return tResponse;
		}

		string EndQuery(int qid) {
			Request request;
			request.set_query_id(qid);
			Response response;
    		ClientContext context;
			
			Status status = stub_->EndQuery(&context, request, &response);

	  		if (!status.ok()) {
				KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
				KETILOG::FATALLOG(LOGTAG,"RPC failed");
			}
			return response.value();
		}

	private:
		std::unique_ptr<MergingModule::Stub> stub_;
		inline const static std::string LOGTAG = "Interface::Merging Connector";
};