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

    // string message = ">Scheduling and Send Snippet ID:" + to_string(snippet.query_id()) + "-" + to_string(snippet.work_id()) + " Complete";
    // logToFile(message);
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
    rapidjson::Value sst_list(rapidjson::kArrayType);

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
                            
                            if(!block_info.HasMember("partition")){
                                block_info.AddMember("partition", csd_array[j]["partition"], doc.GetAllocator());
                            }

                            if(csd_array[j]["block"].Size() > 0){
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
                            }else{
                                rapidjson::Value sst_name_;
                                sst_name_.SetString(sst_name.c_str(), doc.GetAllocator());
                                sst_list.PushBack(sst_name_, doc.GetAllocator());
                                csd_block_count += sst_info[i]["sst_block_count"].GetInt();
                            }

                            break;
                        }
                    }

                    break;
                }
            }
        }
    }

    if (block.Size() > 0){
        block_info.AddMember("block", block, doc.GetAllocator());
    }else{
        block_info.AddMember("sst_list", sst_list, doc.GetAllocator());
    }

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
        // THEN 다음에 있는 결과값의 type과 length로 결정 
        if(snippet.query_info().projection(i).value(0) == "CASE"){  // OK
            for(int j=0;j < snippet.query_info().projection(i).value_size();j++){
                if(snippet.query_info().projection(i).value(j) == "THEN"){
                    
                    int tempReturnType = snippet.query_info().projection(i).value_type(j+1);
                    int tempReturnLength = getLengthByType(tempReturnType); //그냥 숫자 인경우 length 계산
                    if(tempReturnLength == 0){ // string이나 varchar인 경우 그냥 그거 길이 만큼
                        tempReturnLength = snippet.query_info().projection(i).value(j+1).length();
                    }
                    
                    return_column_type.push_back(getValueTypeToMysqlType(tempReturnType));
                    return_column_length.push_back(tempReturnLength);
                    break;
                }
            }

        // EXTRACT의 경우 연월일시간을 추출할때 사용하므로 value와 length 변경할 필요 음슴
        }else if(snippet.query_info().projection(i).value(0) == "EXTRACT"){ // 보니까 연,월,일을 뽑아서 INT16 length 2 
            return_column_type.push_back(3); // INT32
            return_column_length.push_back(4);

        }else if(snippet.query_info().projection(i).value(0) == "SUBSTRING"){ // 뒤에 나오는 int의 a, b b-a+1로 length 설정
            int substringLength = stoi(snippet.query_info().projection(i).value(3)) - stoi(snippet.query_info().projection(i).value(2)) + 1;
            int substringType = column_type.find(snippet.query_info().projection(i).value(1)) -> second; // type은 타겟이 되는 컬럼 그대로
            return_column_type.push_back(substringType);
            return_column_length.push_back(substringLength);

        }else{
            if(snippet.query_info().projection(i).value_size() == 1){ // 그냥 하나인 경우 그대로 간다
                return_column_type.push_back(column_type[snippet.query_info().projection(i).value(0)]);
                return_column_length.push_back(column_length[snippet.query_info().projection(i).value(0)]);
            }else{
                int resultType = 0;
                int resultLength = 0;
                int multiple_count = 0;
                std::stack<int> postfixStack; // 곱하기는 int 80, 더하기는 int 120 빼기는 int 110 나누기는 int 50 아니다 필요없네
                int decimalCount = 0, intCount = 0;
                for (int j = 0; j < snippet.query_info().projection(i).value_size(); j++){
                    string token = snippet.query_info().projection(i).value(j);
                    int tokenType = 0;
                    if (token == "+" || token == "-" || token == "*" || token == "/"){
                        int op2 = postfixStack.top(); postfixStack.pop();
                        int op1 = postfixStack.top(); postfixStack.pop();
                        if(op1 == 246 || op2 == 246){
                            tokenType = 246;
                        }
                        else{
                            tokenType = 3;
                        }
                        if(token == "*" || token == "/"){
                            if(decimalCount == 0){
                                if(op1 == 246) decimalCount++;
                                if(op2 == 246) decimalCount++;
                            }
                            else{
                                if(op1 == 246) decimalCount++;
                            }
                        }
                        postfixStack.push(tokenType);
                    }
                    else{
                        if(snippet.query_info().projection(i).value_type(j) == 10){
                            tokenType = column_type.find(snippet.query_info().projection(i).value(j)) -> second;
                        }
                        else{
                            tokenType = snippet.query_info().projection(i).value_type(j);
                        }
                        postfixStack.push(tokenType);
                    }
                }
                if(decimalCount == 0){ // 오직 int 연산인 경우 
                    return_column_type.push_back(3);
                    return_column_length.push_back(4);
                }
                else{
                    return_column_type.push_back(246);
                    return_column_length.push_back(6+decimalCount);
                }
            }
        }
    }
}


int SnippetManager::getLengthByType(int valueType){
    int returnVal = 0;
    switch (valueType){
        case 0:
            returnVal = 1;
            break;
        case 1:
            returnVal = 2;
            break;
        case 2:
            returnVal = 4;
            break;
        case 3:
            returnVal = 8;
            break;
        case 4:
            returnVal = 4;
            break;
        case 5:
            returnVal = 8;
            break;
        case 7:
            returnVal = 4;
            break;
        case 8:
            returnVal = 8;
            break;
        case 14:
            returnVal = 3;
            break;
        case 246:
            returnVal = 7;
            break;
        default:
            returnVal = 0;
            break;
    }
    return returnVal;
}

int SnippetManager::getValueTypeToMysqlType(int valueType){
    int returnVal = 0;
    switch (valueType){
        case 0:
            returnVal = 2;
            break;
        case 1:
            returnVal = 2;
            break;
        case 2:
            returnVal = 3;
            break;
        case 3:
            returnVal = 8;
            break;
        case 4:
            returnVal = 4;
            break;
        case 5:
            returnVal = 5;
            break;
        case 6:
            returnVal = 246;
            break;
        case 7:
            returnVal = 14;
            break;
        case 8:
            returnVal = 7;
            break;
        case 9:
            returnVal = 254;
            break;

        default:
            returnVal = 0;
            break;
    }
    return returnVal;
}
