#include "snippet_manager.h"

//map key: sst name, value : bestcsd, ex) <1.sst,1>, <2.sst,1>, <3.sst,2>
void SnippetManager::setupSnippet(SnippetRequest snippet, map<string,string> bestcsd){ 
    map<string, vector<string>> sst_group; // key: csd id, value: targer_sst_name
    
    for (const auto& csd : bestcsd) {
        sst_group[csd.second].push_back(csd.first);
    }

    for(const auto& it : sst_group){
        string json_str = serialize(snippet, it.first, it.second);
        sendSnippetToCSD(json_str);
    }
}

string SnippetManager::serialize(SnippetRequest snippet, string best_csd_id, vector<string> target_sst_list) {
    std::string snippet_string;
    google::protobuf::util::JsonOptions options;
    options.always_print_enums_as_ints =true;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    google::protobuf::util::MessageToJsonString(snippet, &snippet_string,options);

    int csd_block_count = 0;
    
    rapidjson::Document doc;
    doc.Parse(snippet_string.c_str());
    if (doc.HasParseError()) {
        std::cerr << "JSON parse error!" << std::endl;
        return "";
    }

    rapidjson::Value block_info(rapidjson::kObjectType);
    rapidjson::Value block(rapidjson::kArrayType);

    if (doc.HasMember("sst_info") && doc["sst_info"].IsArray()) {
        rapidjson::Value& sst_info = doc["sst_info"];

        for(string target_sst : target_sst_list){


            for (rapidjson::SizeType i = 0; i < sst_info.Size(); i++) { 
                std::string sst_name = sst_info[i]["sst_name"].GetString();

                if (sst_name == target_sst) {
                    rapidjson::Value& csd_array = sst_info[i]["csd"];

                    for (rapidjson::SizeType j = 0; j < csd_array.Size(); j++) { 
                        std::string csd_id = csd_array[j]["csd_id"].GetString();

                        if (csd_id == best_csd_id) {

                            block_info.AddMember("partition", csd_array[j]["partition"], doc.GetAllocator());
                        
                            rapidjson::Value& block_array = csd_array[j]["block"];
                            for (rapidjson::SizeType k = 0; k < block_array.Size(); k++) { 
                                rapidjson::Value& block_obj = block_array[k];
                                csd_block_count += block_obj["length"].Size();
                                block.PushBack(block_obj, doc.GetAllocator());
                            }
                        }
                    }
                }
            }
        }
    }

    block_info.AddMember("block", block, doc.GetAllocator());
    doc.AddMember("block_info",block_info,doc.GetAllocator());

    doc.RemoveMember("sst_info");

    rapidjson::Value& result_info = doc["result_info"];

    result_info.AddMember("csd_block_count", csd_block_count, doc.GetAllocator());

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

    std::string port_str = CSD_IDENTIFIER_PORT;
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



