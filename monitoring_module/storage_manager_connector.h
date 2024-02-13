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
using StorageEngineInstance::StorageManager;
using StorageEngineInstance::LBA2PBARequest;
using StorageEngineInstance::LBA2PBAResponse;
using StorageEngineInstance::LBA2PBAResponse_PBA;
using StorageEngineInstance::LBA2PBAResponse_Chunk;
using StorageEngineInstance::DataFileInfo;
using StorageEngineInstance::SSTList;

#define BUFF_SIZE 4096

class StorageManagerConnector {

public:
    StorageManagerConnector(std::shared_ptr<Channel> channel) : stub_(StorageManager::NewStub(channel)) {}

    void RequestPBA(LBA2PBARequest lba2pbaRequest, int &total_block_count, map<string,string> &sst_pba_map) {
        LBA2PBAResponse pbaResponse;
        // ClientContext context;
        // CompletionQueue cq;

        // std::unique_ptr<ClientAsyncResponseReader<LBA2PBAResponse> > rpc(
        //     stub_->RequestPBA(&context, lba2pbaRequest, &cq));

        // Status status;
        // rpc->Finish(&pbaResponse, &status, (void*)1);
        
        // Status status = stub_->RequestPBA(&context, lba2pbaRequest, &pbaResponse);

        // if (!status.ok()) {
        //     KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
		// 	KETILOG::FATALLOG(LOGTAG,"RPC failed");
        // }

        for (const auto entry : pbaResponse.sst_pba_map()) {
            string sst_name = entry.first;
            LBA2PBAResponse_PBA csd_pba = entry.second;
            string csd_id = csd_pba.csd_id();

            //json 구성
            StringBuffer buffer;
            buffer.Clear();
            Writer<StringBuffer> writer(buffer);
            writer.StartObject();
            writer.Key("blockList");
            writer.StartArray();
            writer.StartObject();
            for(int l=0; l<csd_pba.chunks_size(); l++){
                if(l == 0){
                    writer.Key("offset");
                    writer.Int64(csd_pba.chunks(l).offset());
                    writer.Key("length");
                    writer.StartArray();
                }else{
                    if(csd_pba.chunks(l-1).offset()+csd_pba.chunks(l-1).length() != csd_pba.chunks(l).offset()){
                        writer.EndArray();
                        writer.EndObject();
                        writer.StartObject();
                        writer.Key("offset");
                        writer.Int64(csd_pba.chunks(l).offset());
                        writer.Key("length");
                        writer.StartArray();
                    }
                }
                writer.Int(csd_pba.chunks(l).length());

                if(l == csd_pba.chunks_size() - 1){
                    writer.EndArray();
                    writer.EndObject();
                }
            }
            writer.EndArray();
            writer.EndObject();

            total_block_count += csd_pba.chunks_size();

            string pba_string = buffer.GetString();
            sst_pba_map.insert({sst_name, pba_string});
        }
    }

    DataFileInfo GetDataFileInfo(vector<string> sstList_){
        SSTList request;
        ClientContext context;
        DataFileInfo response;

        request.mutable_sst_list()->Add(sstList_.begin(), sstList_.end());
        Status status = stub_->GetDataFileInfo(&context, request, &response);
        
        if (!status.ok()) {
            KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
            KETILOG::FATALLOG(LOGTAG,"RPC failed");
        }

        return response;    
    }

private:
    std::unique_ptr<StorageManager::Stub> stub_;
    inline const static std::string LOGTAG = "Monitoring::Storage Manager Connector";
};