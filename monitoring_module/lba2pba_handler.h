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

    StorageEngineInstance::PBAResponse RequestPBA(StorageEngineInstance::LBARequest lbaRequest) {
        KETILOG::DEBUGLOG(LOGTAG, "# request pba");

        StorageEngineInstance::PBAResponse pbaResponse;
        ClientContext context;
        
        Status status = stub_->RequestPBA(&context, lbaRequest, &pbaResponse);

        if (!status.ok()) {
            KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
			KETILOG::FATALLOG(LOGTAG,"RPC failed");
        }

        return pbaResponse;
    }

private:
    std::unique_ptr<StorageManager::Stub> stub_;
    inline const static std::string LOGTAG = "Monitoring::LBA2PBA Manager Connector";
};