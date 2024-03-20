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

class WALHandler {
    
public:
    WALHandler(std::shared_ptr<Channel> channel) : stub_(WALManager::NewStub(channel)) {}

    void RequestWAL(WALRequest walRequset, int sst_count, string &wal_deleted_key_json, vector<string> &wal_inserted_row_json) {
        KETILOG::DEBUGLOG(LOGTAG, "# request wal");

        WALResponse walResponse;
        ClientContext context;
        Status status;

        Status status = stub_->RequestWAL(&context, walRequset, &walResponse);

        if (!status.ok()) {
            KETILOG::FATALLOG(LOGTAG,status.error_code() + ": " + status.error_message());
			KETILOG::FATALLOG(LOGTAG,"RPC failed");
        }

        return;
    }

private:
    std::unique_ptr<WALManager::Stub> stub_;
    inline const static std::string LOGTAG = "Monitoring::WAL Manager Connector";
};