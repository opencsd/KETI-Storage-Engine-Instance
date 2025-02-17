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

inline string formatTable(const QueryResult& result){
	map<string, size_t> max_col_width;
	vector<string> column_names;

	for (const auto& [col_name, col] : result.query_result()) {
        column_names.push_back(col_name);
        max_col_width[col_name] = col_name.length(); 
    }

	for (const auto& [col_name, col] : result.query_result()) {
        if (col.col_type() == QueryResult_Column::TYPE_STRING) {
            for (const auto& val : col.string_col()) {
                max_col_width[col_name] = std::max(max_col_width[col_name], val.length());
            }
        } else if (col.col_type() == QueryResult_Column::TYPE_INT) {
            for (const auto& val : col.int_col()) {
                max_col_width[col_name] = std::max(max_col_width[col_name], std::to_string(val).length());
            }
        } else if (col.col_type() == QueryResult_Column::TYPE_FLOAT) {
            for (const auto& val : col.double_col()) {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(col.real_size()) << val;
                max_col_width[col_name] = std::max(max_col_width[col_name], oss.str().length());
            }
        } else if (col.col_type() == QueryResult_Column::TYPE_DATE) {
            max_col_width[col_name] = std::max(max_col_width[col_name], size_t(12));
        }
    }

	std::ostringstream table;

	table << "+";
    for (const auto& col_name : column_names) {
        table << std::string(max_col_width[col_name] + 2, '-') << "+";
    }
    table << "\n|";
    for (const auto& col_name : column_names) {
        table << " " << std::setw(max_col_width[col_name]) << std::left << col_name << " |";
    }
    table << "\n+";
    for (const auto& col_name : column_names) {
        table << std::string(max_col_width[col_name] + 2, '-') << "+";
    }
    table << "\n";

	for (int i = 0; i < result.row_count(); ++i) {
        table << "|";
        for (const auto& col_name : column_names) {
            const auto& col = result.query_result().at(col_name);
            std::ostringstream cell;
            if (col.col_type() == QueryResult_Column::TYPE_STRING) {
                cell << (i < col.string_col_size() ? col.string_col(i) : "");
            } else if (col.col_type() == QueryResult_Column::TYPE_INT) {
                cell << (i < col.int_col_size() ? std::to_string(col.int_col(i)) : "");
            } else if (col.col_type() == QueryResult_Column::TYPE_FLOAT) {
                if (i < col.double_col_size()) {
                    cell << std::fixed << std::setprecision(col.real_size()) << col.double_col(i);
                }
            } else if (col.col_type() == QueryResult_Column::TYPE_DATE) {
				if(i < col.int_col_size()){
					int year = col.int_col(i) / (32 * 16);
					int month = (col.int_col(i) % (32 * 16)) / 32;
					std::string formattedMonth = (month < 10 ? "0" : "") + std::to_string(month);
					int day = (col.int_col(i) % (32 * 16)) % 32;
					std::string formattedDay = (day < 10 ? "0" : "") + std::to_string(day);
					string date = to_string(year) + "-" + formattedMonth + "-" + formattedDay;
					cell << date ;
				}else{
					cell << "" ;
				}
            }
            table << " " << std::setw(max_col_width[col_name]) << std::left << cell.str() << " |";
        }
        table << "\n";
    }

	table << "+";
    for (const auto& col_name : column_names) {
        table << std::string(max_col_width[col_name] + 2, '-') << "+";
    }
    table << "\n";

    return table.str();
}

class MergingModuleConnector {
	public:
		MergingModuleConnector(std::shared_ptr<Channel> channel) : stub_(MergingModule::NewStub(channel)) {}

		string Aggregation(SnippetRequest snippet_request) {
			KETILOG::DEBUGLOG(LOGTAG, "# send aggregation snippet");

			Response response;
    		ClientContext context;
			
			Status status = stub_->Aggregation(&context, snippet_request, &response);

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

			string result_string = formatTable(query_result);

			QueryStringResult result;
			result.set_query_result(result_string);
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