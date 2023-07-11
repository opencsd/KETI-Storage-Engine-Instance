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
#include "TableManager.h"
#include "internal_queue.h"

using namespace rapidjson;
using namespace std;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using StorageEngineInstance::LBA2PBAManager;
using StorageEngineInstance::PBAResponse;
using StorageEngineInstance::LBARequest;
using StorageEngineInstance::PBAList;
using StorageEngineInstance::LBA;
using StorageEngineInstance::PBA;
using StorageEngineInstance::Chunk;
using StorageEngineInstance::Request;
using StorageEngineInstance::MetaDataResponse;
using StorageEngineInstance::MetaDataResponse_PBAInfo;

#define BUFF_SIZE 4096

class LBA2PBAManager_Interface {

public:
    LBA2PBAManager_Interface(std::shared_ptr<Channel> channel) : stub_(LBA2PBAManager::NewStub(channel)) {}

    PBAResponse RequestPBA(LBARequest lbaRequset) {
        PBAResponse pbaResponse;
        ClientContext context;
        
        Status status = stub_->RequestPBA(&context, lbaRequset, &pbaResponse);
        // cout << "result : " << result.value() << endl;

        if (!status.ok()) {
            KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
			KETILOG::FATALLOG(LOGTAG,"RPC failed");
        }
        return pbaResponse;
    }

private:
    std::unique_ptr<LBA2PBAManager::Stub> stub_;
    inline const static std::string LOGTAG = "Monitoring Container::LBA2PBA Manager Interface";
};

class LBA2PBAQueryAgent {

public:
    inline const static std::string LOGTAG = "Monitoring Container::LBA2PBA Query Agent";

    static void InitLBA2PBAQueryAgent(){
      GetInstance().initLBA2PBAQueryAgent();
    }

    static void PushQueue(Request request){
      GetInstance().pushQueue(request);
    }

private:
    LBA2PBAQueryAgent(){};
    LBA2PBAQueryAgent(const LBA2PBAQueryAgent&);
    LBA2PBAQueryAgent& operator=(const LBA2PBAQueryAgent&){
        return *this;
    }

    static LBA2PBAQueryAgent& GetInstance(){
        static LBA2PBAQueryAgent _instance;
        return _instance;
    }

    static Request pushQueue(Request request){
        GetInstance().lba2pbaQueue.push_work(request);
    }

    static Request popQueue(){
        Request request = GetInstance().lba2pbaQueue.wait_and_pop();
        return request;
    }

    void initLBA2PBAQueryAgent();
    void requestPBA();

    //make request proto
    static LBARequest generate_lba_request(Request request){
        LBARequest lbaRequest;
        // lbaRequest.set_ordering("Chunk | CSD");

        TableManager::Table table = TableManager::GetTable(request.table_name());

        for(int i=0; i<table.SSTList.size(); i++){
            TableManager::SSTFile sst = table.SSTList[i];//인덱스 테이블인 경우 해당 lba만

            LBA file_lba;
            file_lba.set_file_name(sst.filename);

            for(int j=0; j<sst.BlockList.size(); j++){
                Chunk chunk;
                chunk.set_offset(sst.BlockList[j].Offset);
                chunk.set_length(sst.BlockList[j].Length);
                file_lba.add_chunks()->CopyFrom(chunk);
            }

            lbaRequest.add_file_lba_list()->CopyFrom(file_lba);
        }

        {
        // // LBA 요청 내용 확인 - Debug Code   
        // std::string test_json;
        // google::protobuf::util::JsonPrintOptions options;
        // options.always_print_primitive_fields = true;
        // options.always_print_enums_as_ints = true;
        // google::protobuf::util::MessageToJsonString(lbaRequest,&test_json,options);
        // std::cout << "LBA Request to JSON" << std::endl;
        // std::cout << test_json << std::endl << std::endl;
        }
        
        return lbaRequest;
    }

    //send grpc request
    static PBAResponse request_lba2pba_manager(LBARequest lbaRequest){
        LBA2PBAManager_Interface lbm(grpc::CreateChannel((string)CSD_LBA2PBA_IP+":"+to_string(CSD_LBA2PBA_PORT), grpc::InsecureChannelCredentials()));
        PBAResponse pbaResponse = lbm.RequestPBA(lbaRequest);
        return pbaResponse;
    }

    kQueue<Request> lba2pbaQueue;
    thread LBA2PBA_Thread;
};

void LBA2PBAQueryAgent::initLBA2PBAQueryAgent(){
    std::thread LBA2PBA_Thread(&LBA2PBAQueryAgent::requestPBA,this);
    LBA2PBA_Thread.detach();
}

void LBA2PBAQueryAgent::requestPBA(){
    while(1){
        Request request = popQueue();

        int qid = request.query_id();
        int wid = request.work_id();
        string tname = request.table_name();
        
        string key = TableManager::makeKey(qid,wid);
        Response* data = TableManager::GetReturnData(key);
        unique_lock<mutex> lock(data->mu);

        KETILOG::DEBUGLOG(LOGTAG, "# Request PBA Called");

        MetaDataResponse response_;

        LBARequest lbaRequest = generate_lba_request(request);
        PBAResponse pbaResponse = request_lba2pba_manager(lbaRequest);

        int block_count = 0;
        
        for(int i=0; i<pbaResponse.file_csd_list_size(); i++){
            MetaDataResponse_PBAInfo pba_info;
            string file_name = pbaResponse.file_csd_list(i).file_name();

            for(int j=0; j<pbaResponse.file_csd_list(i).csd_pba_list_size(); j++){
                PBA csd_pba = pbaResponse.file_csd_list(i).csd_pba_list(j);
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

                block_count += csd_pba.chunks_size();

                string pba_string = buffer.GetString();
                pba_info.mutable_csd_pba_map()->insert({csd_id,pba_string});
            }
            data->metadataResponse->mutable_sst_csd_map()->insert({file_name,pba_info});
        }
        data->block_count = block_count;

        data->lba2pba_done = true;
        data->cond.notify_all();
    }
}