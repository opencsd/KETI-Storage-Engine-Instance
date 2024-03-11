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
using StorageEngineInstance::ScanInfo;
using StorageEngineInstance::PBAResponse;
using StorageEngineInstance::PBAResponse_PBA;
using StorageEngineInstance::Chunk;

#define BUFF_SIZE 4096

class StorageManagerConnector {

public:
    StorageManagerConnector(std::shared_ptr<Channel> channel) : stub_(StorageManager::NewStub(channel)) {}

    void RequestPBA(ScanInfo lbaRequest, int &total_block_count, map<string,string> &sst_pba_map, CompletionQueue *cq) {
        KETILOG::DEBUGLOG(LOGTAG, "# request pba");

        PBAResponse pbaResponse;
        ClientContext context;

        // std::unique_ptr<ClientAsyncResponseReader<LBA2PBAResponse>> rpc(stub_->AsyncRequestPBA(&context,lba2pbaRequest,&test));        
        // rpc->Finish(&pbaResponse, &status, (void*)1);
        
        Status status = stub_->RequestPBA(&context, lbaRequest, &pbaResponse);

        if (!status.ok()) {
            KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
			KETILOG::FATALLOG(LOGTAG,"RPC failed");
        }

        for (const auto entry : pbaResponse.pba_chunks()) {
            string sst_name = entry.first;
            PBAResponse_PBA csd_pba = entry.second;

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
            sst_pba_map[sst_name] = pba_string;
        }
    }

private:
    std::unique_ptr<StorageManager::Stub> stub_;
    inline const static std::string LOGTAG = "Monitoring::LBA2PBA Manager Connector";
};