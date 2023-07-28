#pragma once

#include <thread>
#include <unordered_map>

#include <google/protobuf/util/field_mask_util.h>

#include "ip_config.h"
#include "keti_log.h"
#include "internal_queue.h"

#include "MonitoringContainerInterface.h"
#include "MergingContainerInterface.h"
#include "OffloadingContainerInterface.h"

using google::protobuf::FieldMask;
using StorageEngineInstance::SnippetRequest;
using StorageEngineInstance::Snippet;

class SnippetManager {
/* Variables */
public:
    inline const static std::string LOGTAG = "Interface Container::Snippet Manager";

private:
    kQueue<SnippetRequest> SnippetQueue;
    unordered_map<int,string> QueryList; //key:queryID, value:table name

/* Methods */
public:
    static void PushQueue(SnippetRequest snippet){
      GetInstance().SnippetQueue.push_work(snippet);
    }

    static SnippetRequest PopQueue(){
      SnippetRequest blockResult = GetInstance().SnippetQueue.wait_and_pop();
      return blockResult;
    }

    static void InsertQueryID(int qid){
        GetInstance().QueryList.insert({qid,""});
    }

    static void EraseQueryID(int qid){
        GetInstance().QueryList.erase(qid);
    }

    static void SetTableName(int qid, string tname){
        GetInstance().QueryList[qid] = tname;
    }

    static string GetTableName(int qid){
        return GetInstance().QueryList[qid];
    }

private:
    SnippetManager() {
        /* Init Snippet Container */
        /* Start Thread */
        std::thread thd(&SnippetManager::runSnippetManager,this);
        thd.detach();
    };
    SnippetManager(const SnippetManager&);
    SnippetManager& operator=(const SnippetManager&){
        return *this;
    };
    
    static SnippetManager& GetInstance() {
        static SnippetManager _instance;
        return _instance;
    }

    void runSnippetManager(){
        while(1) {
            SnippetRequest snippetrequest = PopQueue();

            // Check Recv Snippet
            {
                // std::string test_json;
                // google::protobuf::util::JsonPrintOptions options;
                // options.always_print_primitive_fields = true;
                // options.always_print_enums_as_ints = true;
                // google::protobuf::util::MessageToJsonString(snippetrequest,&test_json,options);
                // std::cout << "Popped Snippet to JSON" << std::endl;
                // std::cout << test_json << std::endl << std::endl;
            }

            /* Request Set MetaData */
            if(snippetrequest.type() == StorageEngineInstance::SnippetRequest::CSD_SCAN_SNIPPET) {
                //async call
                KETILOG::DEBUGLOG(LOGTAG,"# Request Monitoring Container Setting Metadata");
                Monitoring_Container_Interface moc(grpc::CreateChannel((std::string)LOCALHOST+":"+std::to_string(SE_MONITORING_CONTAINER_PORT), grpc::InsecureChannelCredentials()));
                moc.SetMetaData(snippetrequest);
            } 

            /* Init Buffer Manager */
            KETILOG::DEBUGLOG(LOGTAG,"# Init Buffer Manager");                
            
            FieldMask mask;
            mask.add_paths("query_ID");
            mask.add_paths("work_ID");
            mask.add_paths("table_alias");
            mask.add_paths("column_alias");
            
            // Snippet masked_snippet;
            // masked_snippet.CopyFrom(snippetrequest.snippet());
            Snippet masked_snippet = snippetrequest.snippet();
            google::protobuf::util::FieldMaskUtil::TrimMessage(mask, &masked_snippet);

            Merging_Container_Interface mc(grpc::CreateChannel((std::string)LOCALHOST+":"+std::to_string(SE_MERGING_CONTAINER_PORT), grpc::InsecureChannelCredentials()));
            mc.InitBuffer(masked_snippet, snippetrequest.type());

            {
            // // 마스킹 스니펫 내용 확인 - Debug Code   
            // std::string test_json;
            // google::protobuf::util::JsonPrintOptions options;
            // options.always_print_primitive_fields = true;
            // options.always_print_enums_as_ints = true;
            // google::protobuf::util::MessageToJsonString(masked_snippet,&test_json,options);
            // std::cout << "Masked Snippet to JSON" << std::endl;
            // std::cout << test_json << std::endl << std::endl;
            }

            /* Send Snippet to Container */
            if(snippetrequest.type() == StorageEngineInstance::SnippetRequest::CSD_SCAN_SNIPPET) {
                KETILOG::DEBUGLOG(LOGTAG,"# Send Snippet to Offloading Container");
                Offloading_Container_Interface oc(grpc::CreateChannel((std::string)LOCALHOST+":"+std::to_string(SE_OFFLOADING_CONTAINER_PORT), grpc::InsecureChannelCredentials()));
                oc.Schedule(snippetrequest.snippet());
            } else {
                KETILOG::DEBUGLOG(LOGTAG,"# Send Snippet to Merging Container");
                Merging_Container_Interface mc(grpc::CreateChannel((std::string)LOCALHOST+":"+std::to_string(SE_MERGING_CONTAINER_PORT), grpc::InsecureChannelCredentials()));
                mc.Aggregation(snippetrequest);
            }
        }
    }
};