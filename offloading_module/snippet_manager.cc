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
                                if(block_obj["offset"].Size() == 1){
                                    csd_block_count += block_obj["length"].Size();
                                }else{
                                    csd_block_count += 1;
                                }
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

    vector<int> return_column_length_;
    vector<int> return_column_type_;

    calcul_return_column_type(snippet, return_column_length_, return_column_type_);

    rapidjson::Value return_column_length(rapidjson::kArrayType);
    for (int length : return_column_length_) {
        return_column_length.PushBack(length, doc.GetAllocator());
    }
    rapidjson::Value return_column_type(rapidjson::kArrayType);
    for (int type : return_column_type_) {
        return_column_type.PushBack(type, doc.GetAllocator()); 
    }

    result_info.AddMember("csd_block_count", csd_block_count, doc.GetAllocator());
    result_info.AddMember("return_column_length", return_column_length, doc.GetAllocator());
    result_info.AddMember("return_column_type", return_column_type, doc.GetAllocator());

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

void SnippetManager::calcul_return_column_type(SnippetRequest& snippet, vector<int>& return_column_length, vector<int>& return_column_type){// *임시tpc-h 쿼리 동작만 수행하도록 작성 => 수정필요
    unordered_map<string, int> column_type, column_length;
    for (int i = 0; i < snippet.schema_info().column_list_size(); i++){
        column_type.insert(make_pair(snippet.schema_info().column_list(i).name(), snippet.schema_info().column_list(i).type()));
        column_length.insert(make_pair(snippet.schema_info().column_list(i).name(), snippet.schema_info().column_list(i).length()));
    }
    
    for (int i = 0; i < snippet.query_info().projection_size(); i++){
        if(snippet.query_info().projection(i).value(0) == "CASE"){
            return_column_type.push_back(2);
            return_column_length.push_back(4);
        }else if(snippet.query_info().projection(i).value(0) == "EXTRACT"){
            return_column_type.push_back(2);
            return_column_length.push_back(4);
        }else if(snippet.query_info().projection(i).value(0) == "SUBSTRING"){
            return_column_type.push_back(254);
            return_column_length.push_back(2);
        }else{
            if(snippet.query_info().projection(i).value_size() == 1){
                return_column_type.push_back(column_type[snippet.query_info().projection(i).value(0)]);
                return_column_length.push_back(column_length[snippet.query_info().projection(i).value(0)]);
            }else{
                int multiple_count = 0;
                for (int j = 0; j < snippet.query_info().projection(i).value_size(); j++){
                    if(snippet.query_info().projection(i).value(j) == "*"){
                      if(snippet.query_info().projection(i).value(j-1) == "ps_availqty"){//임시로 작성!!!!
                        multiple_count--;
                      }else{
                        multiple_count++;
                      }
                    }
                }
                if(multiple_count == 1){
                    return_column_type.push_back(246);
                    return_column_length.push_back(8);
                }else if(multiple_count == 2){
                    return_column_type.push_back(246);
                    return_column_length.push_back(9);
                }else{
                    return_column_type.push_back(column_type[snippet.query_info().projection(i).value(0)]);
                    return_column_length.push_back(column_length[snippet.query_info().projection(i).value(0)]);
                }
            }
        }
    }
}



