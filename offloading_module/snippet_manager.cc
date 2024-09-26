#include "snippet_manager.h"

//map key: sst name, value : bestcsd, ex) <1.sst,1>
void SnippetManager::setupSnippet(SnippetRequest snippet, map<string,string> bestcsd){ 
    string target_sst_name;
    string best_csd_id;

    for (const auto& csd : bestcsd) {
        target_sst_name = csd.first;  
        best_csd_id = csd.second;     
    }

    string json_str = serialize(snippet, best_csd_id, target_sst_name);
    sendSnippetToCSD(json_str);

}

string SnippetManager::serialize(SnippetRequest snippet, string best_csd_id, string target_sst_name) {
    std::string snippet_string;
    google::protobuf::util::JsonOptions options;
    options.always_print_enums_as_ints =true;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    google::protobuf::util::MessageToJsonString(snippet, &snippet_string,options);

    rapidjson::Document doc;
    doc.Parse(snippet_string.c_str());
    if (doc.HasParseError()) {
        std::cerr << "JSON parse error!" << std::endl;
        return;
    }

    if(doc.HasMember("workId")){
        rapidjson::Value& work_id = doc["workId"];
    }

    if (doc.HasMember("sstInfo") && doc["sstInfo"].IsArray()) {
        rapidjson::Value& sst_info = doc["sstInfo"];

        // sst_info 배열에서 해당 sst_name만 남기기
        for (rapidjson::SizeType i = 0; i < sst_info.Size(); ) { 
            if (sst_info[i].HasMember("sstName") && sst_info[i]["sstName"].IsString()) {

                std::string sst_name = sst_info[i]["sstName"].GetString();
                // sst_name이 target_sst_name과 일치하지 않으면 삭제
                if (sst_name != target_sst_name) {
                    sst_info.Erase(sst_info.Begin() + i);
                    continue; 
                } else {
                    // 일치하는 sst_name의 csd를 필터링
                    if (sst_info[i].HasMember("csd") && sst_info[i]["csd"].IsArray()) {

                        rapidjson::Value& csd_array = sst_info[i]["csd"];

                        for (rapidjson::SizeType j = 0; j < csd_array.Size(); ) {
                            if (csd_array[j].HasMember("csdId") && csd_array[j]["csdId"].IsString()) {

                                if (std::string(csd_array[j]["csdId"].GetString()) != best_csd_id) {
                                    csd_array.Erase(csd_array.Begin() + j);
                                    continue; 
                                }
                                
                            }
                            ++j;
                        }
                    }

                }
            }
            ++i; // 다음 sst_info 항목으로 이동
        }
    }

    if (doc.HasMember("sstInfo") && doc["sstInfo"].IsArray()) {
        rapidjson::Value& sst_info = doc["sstInfo"];

        // 새로운 blockInfo 객체를 만들기 위해 JsonAllocator를 사용
        rapidjson::Value block_info(rapidjson::kObjectType);

        // 첫 번째 sst_info의 partition 및 block 정보를 가져오기
        if (sst_info.Size() > 0 && sst_info[0].HasMember("csd") && sst_info[0]["csd"].IsArray()) {
            rapidjson::Value& csd_array = sst_info[0]["csd"];

            if (csd_array.Size() > 0 && csd_array[0].HasMember("partition") && csd_array[0].HasMember("block") && csd_array[0]["block"].IsArray()) {
                // partition 정보 추가
                block_info.AddMember("partition", csd_array[0]["partition"], doc.GetAllocator());

                // block 배열 추가
                rapidjson::Value block_array(rapidjson::kArrayType);
                rapidjson::Value& original_block_array = csd_array[0]["block"];
                
                for (rapidjson::SizeType i = 0; i < original_block_array.Size(); ++i) {
                    if (original_block_array[i].HasMember("offset") && original_block_array[i].HasMember("length")) {
                        rapidjson::Value block_obj(rapidjson::kObjectType);
                        block_obj.AddMember("offset", original_block_array[i]["offset"], doc.GetAllocator());
                        block_obj.AddMember("length", original_block_array[i]["length"], doc.GetAllocator());

                        block_array.PushBack(block_obj, doc.GetAllocator());
                    }
                }
                
                block_info.AddMember("block", block_array, doc.GetAllocator());
            }
        }

        // "sstInfo" 제거하고 "blockInfo"로 대체
        doc.RemoveMember("sstInfo");
        doc.AddMember("blockInfo", block_info, doc.GetAllocator());
    }
    
    std::string csd_ip_string = "10.1." + best_csd_id + ".2";
    rapidjson::Value csd_ip(csd_ip_string.c_str(), doc.GetAllocator());
    doc.AddMember("csd_ip", csd_ip ,doc.GetAllocator());
    
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    string json_str = buffer.GetString();

    return json_str;
}

void SnippetManager::sendSnippetToCSD(string snippet_json){
    KETILOG::DEBUGLOG(LOGTAG, "# send snippet to csd ");

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    std::string port_str = std::to_string(CSD_IDENTIFIER_PORT1);
    std::istringstream port_(port_str);
    std::uint16_t port{};
    port_ >> port;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(STORAGE_NODE_IP);
    serv_addr.sin_port = htons(port);

    // {
    //     cout << endl << snippet_json.c_str() << endl << endl;
    // }

    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    size_t len = strlen(snippet_json.c_str());
    send(sock, &len, sizeof(len), 0);
    send(sock, (char *)snippet_json.c_str(), strlen(snippet_json.c_str()), 0);

    close(sock);
}



