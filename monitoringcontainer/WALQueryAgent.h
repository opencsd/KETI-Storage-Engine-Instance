#pragma once

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h" 

#include <grpcpp/grpcpp.h>
#include "storage_engine_instance.grpc.pb.h"

#include "keti_log.h"
#include "ip_config.h"
#include "internal_queue.h"

using namespace rapidjson;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using StorageEngineInstance::Request;
using StorageEngineInstance::WALManager;
using StorageEngineInstance::WALRequest;
using StorageEngineInstance::WALResponse;

#define BUFF_SIZE 4096

class WALManager_Interface {
    
public:
    WALManager_Interface(std::shared_ptr<Channel> channel) : stub_(WALManager::NewStub(channel)) {}

    WALResponse RequestWAL(WALRequest walRequset) {
        WALResponse walResponse;
        ClientContext context;
        
        Status status = stub_->RequestWAL(&context, walRequset, &walResponse);
        // cout << "result : " << result.value() << endl;

        if (!status.ok()) {
            KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
			KETILOG::FATALLOG(LOGTAG,"RPC failed");
        }

        return walResponse;
    }

private:
    std::unique_ptr<WALManager::Stub> stub_;
    inline const static std::string LOGTAG = "Interface Container::WAL Manager Interface";
};

class WALQueryAgent {

public:
    inline const static std::string LOGTAG = "Monitoring Container::WAL Query Agent";

    static void InitWALQueryAgent(){
      GetInstance().initWALQueryAgent();
    }

    static void PushQueue(Request request){
      GetInstance().pushQueue(request);
    }

private:
    WALQueryAgent(){};
    WALQueryAgent(const WALQueryAgent&);
    WALQueryAgent& operator=(const WALQueryAgent&){
        return *this;
    }

    static WALQueryAgent& GetInstance(){
        static WALQueryAgent _instance;
        return _instance;
    }

    static void pushQueue(Request request){
        GetInstance().walQueue.push_work(request);
    }

    static Request popQueue(){
        Request request = GetInstance().walQueue.wait_and_pop();
        return request;
    }

    void initWALQueryAgent();
    void requestWAL();

    kQueue<Request> walQueue;
    thread WAL_Thread;
};

void WALQueryAgent::initWALQueryAgent(){
    std::thread WAL_Thread(&WALQueryAgent::requestWAL, this);
    WAL_Thread.detach();
}

void WALQueryAgent::requestWAL(){
    while(1){
        Request request = popQueue();

        int qid = request.query_id();
        int wid = request.work_id();
        string tname = request.table_name();

        string key = TableManager::makeKey(qid,wid);
        MetaDataResponse_* data = TableManager::GetReturnData(key);
        unique_lock<mutex> lock(data->mu);

        KETILOG::DEBUGLOG(LOGTAG, "# Request WAL Called");

        WALRequest walRequest;
        walRequest.set_req_key(key);
        walRequest.set_type("table");
        walRequest.set_value(tname);

        WALManager_Interface walm(grpc::CreateChannel((string)CSD_WAL_MANAGER_IP+":"+to_string(CSD_WAL_MANAGER_PORT), grpc::InsecureChannelCredentials()));
        WALResponse walResponse = walm.RequestWAL(walRequest);

        StringBuffer buffer;
        buffer.Clear();
        Writer<StringBuffer> writer(buffer);
        writer.StartObject();
        writer.Key("blockList");
        
        for(int i=0; i<walResponse.deleted_key_size(); i++){

        }

        for(int i=0; i<walResponse.inserted_key_size(); i++){

        }

        data->metadataResponse->set_wal_json(buffer.GetString());
        data->wal_done = true;
        data->cond.notify_all();
    }
}