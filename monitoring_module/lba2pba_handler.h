#pragma once

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <thread>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h" 

#include <grpcpp/grpcpp.h>
#include "storage_engine_instance.grpc.pb.h"

#include "keti_log.h"
#include "ip_config.h"
#include "table_manager.h"

using namespace rapidjson;
using namespace std;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientAsyncResponseReader;
using grpc::CompletionQueue;

using StorageEngineInstance::StorageManager;

#define BUFF_SIZE 4096

class StorageManagerConnector {

public:
    StorageManagerConnector(std::shared_ptr<Channel> channel) : stub_(StorageManager::NewStub(channel)) {}

    void RequestPBA(StorageEngineInstance::LBARequest lbaRequest) {
        KETILOG::DEBUGLOG(LOGTAG, "# request pba");

        StorageEngineInstance::PBAResponse pbaResponse;
        ClientContext context;
        
        Status status = stub_->RequestPBA(&context, lbaRequest, &pbaResponse);

        if (!status.ok()) {
            KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
			KETILOG::FATALLOG(LOGTAG,"RPC failed");
        }

        for (const auto res_sst : pbaResponse.sst_list()) {
            string sst_name = res_sst.first;
            map<string, TableManager::TableBlock> csd_pba_list;

            for(const auto res_csd : res_sst.second.table_pba_list()){
                string csd_name = res_csd.first;
                TableManager::TableBlock table_block;

                for(const auto res_pba : res_csd.second.table_block_list()){
                    int table_index_number = res_pba.first;
                    TableManager::BlockList block_list;

                    for(const auto res_chunk : res_pba.second.chunks()){
                        TableManager::Block block;
                        string block_handle = res_chunk.block_handle();
                        block.offset = res_chunk.offset();
                        block.length = res_chunk.length();
                        block_list.block_list.push_back({block_handle, block});
                    }
                    
                    table_block.table_block_list.insert({table_index_number,block_list});
                }

                csd_pba_list.insert({csd_name, table_block});
            }

            TableManager::SetSSTPBAInfo(sst_name, csd_pba_list);
        }
            
    }

private:
    std::unique_ptr<StorageManager::Stub> stub_;
    inline const static std::string LOGTAG = "Monitoring::LBA2PBA Manager Connector";
};