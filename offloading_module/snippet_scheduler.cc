#include "snippet_scheduler.h"

void Scheduler::runScheduler(){ 
    while (1){
        SnippetRequest snippet = Scheduler::PopQueue();
        cout << "[Scheduler] start snippet scheduling {ID:" << snippet.query_id() << "|" << snippet.work_id() << "}" << endl;;
        
        //map sst가 key, 선정 csd가 value 
        map<string,string> bestcsd; 
        Scheduler::getBestCSD(&snippet.sst_info(), bestcsd);
        
        SnippetManager::SetupSnippet(snippet, bestcsd);
    }
}

void Scheduler::getBestCSD(const google::protobuf::RepeatedPtrField<StorageEngineInstance::SnippetRequest_SstInfo>* sst_info, map<string,string>& bestcsd){
    switch(SCHEDULING_ALGORITHM){
        case(DCS):{
            cout << "[Scheduler] scheduling algorithm : DCS(Depend on CSD Status)" << endl;
            DCS_Algorithm(sst_info, bestcsd);
            break;
        }case(DSI):{
            cout << "[Scheduler] scheduling algorithm : DSI(Depend on SST Info)" << endl;
            DSI_Algorithm(sst_info, bestcsd);
            break;
        }case(DFA):{
            cout << "[Scheduler] scheduling algorithm : DFA(Drop First Available)" << endl;
            DFA_Algorithm(sst_info, bestcsd);
            break;
        }case(AUTO_SELECTION):{
            Auto_Selection(sst_info, bestcsd);
            break;
        }
    }

    cout << "[Scheduler] scheduling result: " << endl;
    for(const auto sst_name : bestcsd){   
        cout << "            L " << sst_name.first << " -> csd#" << sst_name.second << endl;
    }
    
    return;
}

void Scheduler::DCS_Algorithm(const google::protobuf::RepeatedPtrField<StorageEngineInstance::SnippetRequest_SstInfo>* sst_info, map<string,string>& bestcsd){
    // csd analysis score를 보고 선택 + 선택된 csd에 대해 점수 패널티 적용
    KETILOG::DEBUGLOG(LOGTAG, "# DCS(Depend on CSD Status) Algorithm");

    unordered_map<string,int> csd_penalty_map;

    for (int i = 0; i < sst_info->size(); ++i) {
        string sst_name = sst_info->Get(i).sst_name();
        string selected_csd = "";
        float csd_max_score = -1;

        if(sst_info->Get(i).csd_size() == 1){
            string csd_id = sst_info->Get(i).csd().Get(0).csd_id();
            bestcsd[sst_name] = csd_id;

            if(csd_penalty_map.find(csd_id) == csd_penalty_map.end()){
                csd_penalty_map[csd_id] = 2;
            }else{
                csd_penalty_map[csd_id] += 1;
            }

        }else{
            for(int j=0; j<sst_info->Get(i).csd_size(); ++j){
                string csd_id = sst_info->Get(i).csd().Get(j).csd_id();
                string csd_key = "csd" + csd_id;
                float csd_score = CSDStatusManager::GetCSDInfo(csd_key).analysis_score;

                if(csd_penalty_map.find(csd_id) != csd_penalty_map.end()){
                    csd_score = csd_score / csd_penalty_map[csd_id];
                }
                
                selected_csd = (csd_score > csd_max_score) ? csd_id : selected_csd;
            }
            bestcsd[sst_name] = selected_csd;
        }
    }

    return;
}

void Scheduler::DSI_Algorithm(const google::protobuf::RepeatedPtrField<StorageEngineInstance::SnippetRequest_SstInfo>* sst_info, map<string,string>& bestcsd){
    // 선택되는 CSD가 최소 중복되도록 선택
    KETILOG::DEBUGLOG(LOGTAG, "# DSI(Depend on SST Info) Algorithm");

    // unordered_map<string,int> csd_index_map;
    // unordered_map<string,int> sst_index_map;
    // int si, ci = 0;

    // // allocation_map에서의 sst와 csd의 인덱스 설정
    // for(int i=0; i < sst_info->size(); ++i){
    //     string sst_name = sst_info->Get(i).sst_name();
    //     if(sst_index_map.find(sst_name) == sst_index_map.end()){
    //         sst_index_map.insert({sst_name, si});
    //         si++;
    //     }

    //     for(int j=0; j < sst_info->Get(i).csd_size(); ++j){
    //         string csd_id = sst_info->Get(i).csd().Get(j).csd_id();
    //         if(csd_index_map.find(csd_id) == csd_index_map.end()){
    //             csd_index_map.insert({csd_id, ci});
    //             ci++;
    //         }
    //     }
    // }

    // // sst x '할당 가능한 csd' 를 0과1로 표시한 allocation_map 생성
    // int allocation_map[sst_index_map.size()][csd_index_map.size()];
    // std::memset(allocation_map, 0, sizeof(allocation_map));

    // for(int i=0; i < sst_info->size(); ++i){
    //     string sst_name = sst_info->Get(i).sst_name();
    //     int sst_index = sst_index_map[sst_name];

    //     for(int j=0; j < sst_info->Get(i).csd_size(); ++j){
    //         string csd_id = sst_info->Get(i).csd().Get(j).csd_id();
    //         int csd_index = csd_index_map[csd_id];

    //         allocation_map[sst_index][csd_index] = 1;
    //     }
    // }

    // // allocation_map을 기반으로 최소 중복을 갖는 경우 탐색
    // // 점수 산정 식 = 사용CSD수 / SST최대중복도
    // // 최고 점수 계산 = 전체CSD수 / Ceil(전체SST수/전체CSD수)
    // stack<int> selected_csd_index;
    // for(int i=0; i < sst_index_map.size(); ++i){
    //     for(int j=0; j < csd_index_map.size(); ++j){
    //         if(allocation_map[i][j] == 1){
    //             selected_csd_index.push(j);
    //             break;
    //         }
    //     }
    // }

    stack<string> selected_csd;
    stack<string> global_csd;
    float variance_score = dfs_for_dsi_algorithm(sst_info, global_csd, selected_csd, 0);

    for(int i=sst_info->size()-1; i > -1; --i){
        string csd = global_csd.top();
        global_csd.pop();
        bestcsd.insert({sst_info->Get(i).sst_name(), csd});
    }

    return;
}

float Scheduler::dfs_for_dsi_algorithm(const google::protobuf::RepeatedPtrField<StorageEngineInstance::SnippetRequest_SstInfo>* sst_info, stack<string>& global_csd, stack<string>& selected_csd, int si){
    if(selected_csd.size() == sst_info->size()){
        unordered_map<string,int> csd_count_map;
        stack<string> temp_selected_csd = selected_csd;

        while (!temp_selected_csd.empty()) {
            string top = temp_selected_csd.top();
            if(csd_count_map.find(top) == csd_count_map.end()){
                csd_count_map[top] = 1;
            }else{
                csd_count_map[top]++;
            }
            temp_selected_csd.pop();
        }

        int max_duplication = 0;
        for(const auto& csd_count : csd_count_map){
            max_duplication = max(csd_count.second, max_duplication);
        }
        float variance_score = (float)csd_count_map.size() / (float)max_duplication;

        return variance_score;
    }

    float global_variance_score = -1;
    
    for(int ci=0; ci<sst_info->Get(si).csd_size(); ++ci){
        selected_csd.push(sst_info->Get(si).csd().Get(ci).csd_id());

        float variance_score = dfs_for_dsi_algorithm(sst_info, global_csd, selected_csd, si+1);
        if(variance_score > global_variance_score){
            global_variance_score = variance_score;
            global_csd = selected_csd;
        }

        selected_csd.pop();
    }

    return global_variance_score;
}

void Scheduler::DFA_Algorithm(const google::protobuf::RepeatedPtrField<StorageEngineInstance::SnippetRequest_SstInfo>*sst_info, map<string,string>& bestcsd){
    // 할당 가능한 csd중 앞에 있는것 선택 + 선택된 csd에 대해 점수 패널티 적용
    KETILOG::DEBUGLOG(LOGTAG, "# DFA(Drop First Available) Algorithm");

    unordered_map<string,int> csd_penalty_map;

    for (int i = 0; i < sst_info->size(); ++i) {
        string sst_name = sst_info->Get(i).sst_name();
        int min_allocated = 1;
        string selected_csd;
        bool flag = true;

        while(flag){
            for(int j=0; i < sst_info->Get(i).csd().size(); ++j){
                string csd_id = sst_info->Get(i).csd().Get(j).csd_id();

                if(csd_penalty_map.find(csd_id) == csd_penalty_map.end()){
                    csd_penalty_map[csd_id] = 1;
                    selected_csd = csd_id;
                    flag = false;
                    break;
                }else if(csd_penalty_map[csd_id] < min_allocated){
                    csd_penalty_map[csd_id] += 1;
                    selected_csd = csd_id;
                    flag = false;
                    break;
                }else{
                    continue;
                }
            }
            min_allocated++;
        }

        bestcsd[sst_name] = selected_csd;
    }

    return;
}

void Scheduler::Auto_Selection(const google::protobuf::RepeatedPtrField<StorageEngineInstance::SnippetRequest_SstInfo>* sst_info, map<string,string>& bestcsd){
    // DSI, DCS, DFA 중 상황에 따라 알고리즘 선택
    KETILOG::DEBUGLOG(LOGTAG, "# Auto Selection");

    int max_csd_count = 0;
    int total_block_count = 0;
    for(int i=0; i<sst_info->size(); ++i){
        max_csd_count = max(max_csd_count, sst_info->Get(i).csd_size());
        total_block_count += max_csd_count, sst_info->Get(i).sst_block_count();
    }

    if(max_csd_count == 1){
        KETILOG::DEBUGLOG(LOGTAG, "# max csd count = 1");
        KETILOG::DEBUGLOG(LOGTAG, "# scheduling by DFA(Drop First Available) Algorithm");
        cout << "[Scheduler] max csd count = 1" << endl;
        cout << "[Scheduler] scheduling by DFA(Drop First Available) Algorithm" << endl;
        DFA_Algorithm(sst_info, bestcsd);
        return;
    }

    int sst_count = sst_info->size();
    int avg_block_count = total_block_count / sst_count;

    if(sst_count > SST_UPPER_BOUND && avg_block_count > BLOCK_UPPER_BOUND){
        KETILOG::DEBUGLOG(LOGTAG, "# sst_count > SST_UPPER_BOUND && avg_block_count > BLOCK_UPPER_BOUND");
        KETILOG::DEBUGLOG(LOGTAG, "# scheduling by DSI(Depend on SST Info) Algorithm");
        cout << "[Scheduler] sst_count(" << sst_count << ") > SST_UPPER_BOUND && avg_block_count(" << avg_block_count << ") > BLOCK_UPPER_BOUND" << endl;
        cout << "[Scheduler] scheduling by DSI(Depend on SST Info) Algorithm" << endl;
        DSI_Algorithm(sst_info, bestcsd);
        return;
    }else if(sst_count > SST_UPPER_BOUND && avg_block_count < BLOCK_UPPER_BOUND){
        KETILOG::DEBUGLOG(LOGTAG, "# sst_count > SST_UPPER_BOUND && avg_block_count < BLOCK_UPPER_BOUND");
        KETILOG::DEBUGLOG(LOGTAG, "# scheduling by DCS(Depend on CSD Status) Algorithm");
        cout << "[Scheduler] sst_count(" << sst_count << ") > SST_UPPER_BOUND && avg_block_count(" << avg_block_count << ") < BLOCK_UPPER_BOUND" << endl;
        cout << "[Scheduler] scheduling by DCS(Depend on CSD Status) Algorithm" << endl;
        DCS_Algorithm(sst_info, bestcsd);
        return;
    }else if(sst_count < SST_UPPER_BOUND && avg_block_count > BLOCK_UPPER_BOUND){
        KETILOG::DEBUGLOG(LOGTAG, "# sst_count < SST_UPPER_BOUND && avg_block_count > BLOCK_UPPER_BOUND");
        KETILOG::DEBUGLOG(LOGTAG, "# scheduling by DSI(Depend on SST Info) Algorithm");
        cout << "[Scheduler] sst_count(" << sst_count << ") < SST_UPPER_BOUND && avg_block_count(" << avg_block_count << ") > BLOCK_UPPER_BOUND" << endl;
        cout << "[Scheduler] scheduling by DSI(Depend on SST Info) Algorithm" << endl;
        DSI_Algorithm(sst_info, bestcsd);
        return;
    }else{
        KETILOG::DEBUGLOG(LOGTAG, "# sst_count < SST_UPPER_BOUND && avg_block_count < BLOCK_UPPER_BOUND");
        KETILOG::DEBUGLOG(LOGTAG, "# scheduling by DCS(Depend on CSD Status) Algorithm");
        cout << "[Scheduler] sst_count(" << sst_count << ") < SST_UPPER_BOUND && avg_block_count(" << avg_block_count << ") < BLOCK_UPPER_BOUND" << endl;
        cout << "[Scheduler] scheduling by DCS(Depend on CSD Status) Algorithm" << endl;
        DCS_Algorithm(sst_info, bestcsd);
        return;
    }

}

void Scheduler::t_snippet_scheduling(TmaxRequest request, TmaxResponse tResponse){
    KETILOG::DEBUGLOG("Offloading", "<T> scheduling tmax snippet...");

    map<string, vector<string>> file_csd_map; //key - file name, value - csd id
    file_csd_map["tdisk0"] = {"1"}; //temp
    file_csd_map["tdisk1"] = {"8"};
    file_csd_map["tdisk2"] = {"3"};
    file_csd_map["tdisk3"] = {"4"};
    file_csd_map["tdisk4"] = {"5"};
    file_csd_map["tdisk5"] = {"6"};
    file_csd_map["tdisk6"] = {"7"};
    file_csd_map["csd_disk0"] = {"1"}; //temp
    file_csd_map["csd_disk1"] = {"8"};
    file_csd_map["csd_disk2"] = {"3"};
    file_csd_map["csd_disk3"] = {"4"};
    file_csd_map["csd_disk4"] = {"5"};
    file_csd_map["csd_disk5"] = {"6"};
    file_csd_map["csd_disk6"] = {"7"};
    file_csd_map["tas_csd_disk1"] = {"1"};
    
    string target_csd_id;
   
    for (const auto& file : request.file_list()) {
        string file_name = file.filename(); 
        if(file_csd_map.find(file_name) != file_csd_map.end()){
            if(file_csd_map[file_name].size() == 1){
                target_csd_id = file_csd_map[file_name].at(0);
                t_offloading_snippet(request, tResponse, target_csd_id, file_name);
            }else{//scheduling
                target_csd_id = file_csd_map[file_name].at(0);
                t_offloading_snippet(request, tResponse, target_csd_id, file_name);
            }
        }else{
            tResponse.set_type(TmaxResponse::FO_RESPONSE);
            tResponse.set_errorcode(TmaxResponse::TMAX_ERROR_INVALID_REQTYPE);
            KETILOG::DEBUGLOG("Offloading", "<T> File does not exist");
            return;
        }
    }

    // for (const auto& file : request.file_list()) {
    //     std::string target_file = file.filename();
    //     string target_csd_id;
    //     bool file_found = false;

        // for (int i = 2; i <= 8; ++i) {
        //     std::string nvmeDir = "/mnt/gluster/nvme" + std::to_string(i) + "/tmax_test";

        //     try {
        //         if (fs::exists(nvmeDir) && fs::is_directory(nvmeDir)) {
        //             for (const auto& entry : fs::directory_iterator(nvmeDir)) {
        //                 if (entry.is_regular_file() && entry.path().filename() == target_file) {
        //                     file_found = true;
        //                     target_csd_id = to_string(i);
        //                     break;
        //                 }
        //             }
        //         }
        //     } catch (const fs::filesystem_error& e) {
        //         std::cerr << "Error accessing " << nvmeDir << ": " << e.what() << std::endl;
        //     }

        //     if (file_found){
        //         cout << "<T> Schuedling : file " << target_file << " = " << target_csd_id << endl;
        //         t_offloading_snippet(request, tResponse,target_csd_id, target_file); 
        //         break;
        //     }
        // }

        // if (!file_found) {
        //     tResponse.set_type(TmaxResponse::FO_RESPONSE);
        //     tResponse.set_errorcode(TmaxResponse::TMAX_ERROR_INVALID_REQTYPE);
        //     KETILOG::DEBUGLOG("Offloading", "<T> File not found in any nvme directories");
        // }

    //     t_offloading_snippet(request, tResponse, "1", target_file); 

    // }

    tResponse.set_type(TmaxResponse::FO_RESPONSE);
    tResponse.set_errorcode(TmaxResponse::TMAX_ERROR_NONE);
        
    return;
}

void Scheduler::t_offloading_snippet(TmaxRequest request, TmaxResponse tResponse, string csd_id, string file_name) {
    KETILOG::DEBUGLOG("Offloading", "<T> called t_offloading_snippet");
    KETILOG::DEBUGLOG("Offloading", "<T> parsing tmax snippet...");

    StringBuffer snippetbuf;
    Writer<StringBuffer> writer(snippetbuf);

    writer.StartObject();

    writer.Key("type");
    writer.Int(StorageEngineInstance::SnippetRequest::TMAX_SNIPPET);

    writer.Key("csd_ip");
    string csdIP = "10.1." + csd_id + ".2";
    writer.String(csdIP.c_str());

    writer.Key("id");
    writer.Int(request.id());

    writer.Key("block_size");
    writer.Int(request.block_size());

    writer.Key("buffer_size");
    writer.Int(request.buffer_size());

    std::string base64_encoded = Base64Encode(request.filter());
    writer.Key("filter_info");
    writer.String(base64_encoded.c_str());

    writer.Key("file_name");
    writer.String(file_name.c_str());

    writer.Key("chunk_list");
    writer.StartArray();
    for (int i = 0; i < request.file_list_size(); i++) {
        if (file_name == request.file_list(i).filename()) {
            for (int j = 0; j < request.file_list(i).chunk_list_size(); j++) {
                writer.StartObject();
                writer.Key("offset");
                writer.Int64(request.file_list(i).chunk_list(j).offset());
                writer.Key("length");
                writer.Int64(request.file_list(i).chunk_list(j).length());
                writer.EndObject();
            }
        }
    }
    writer.EndArray(); 

    writer.EndObject();

    string snippet_json = snippetbuf.GetString();
    KETILOG::DEBUGLOG("Offloading", "<T> send tmax snippet...");
    KETILOG::DEBUGLOG("Offloading", snippet_json);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    std::istringstream port_(CSD_IDENTIFIER_PORT);
    std::uint16_t port{};
    port_ >> port;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(STORAGE_NODE_IP);
    serv_addr.sin_port = htons(port);

    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    
    size_t len = strlen(snippet_json.c_str());
    send(sock, &len, sizeof(len), 0);
    send(sock, (char *)snippet_json.c_str(), strlen(snippet_json.c_str()), 0);

    close(sock);

    return;
}