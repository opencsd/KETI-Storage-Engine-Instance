#include "snippet_scheduler.h"

void Scheduler::runScheduler(){ 
    while (1){
        SnippetRequest snippet = Scheduler::PopQueue();

        ScanInfo* scan_info = snippet.mutable_scan_info();

        int size = scan_info->sst_csd_map_size();
        map<string,string> bestcsd;
        bestcsd = Scheduler::getBestCSD(scan_info);
                
        // make adding PBA,WAL and sending snippet to csd thread
        std::thread trd(&SnippetManager::SetupSnippet, snippet, bestcsd);
        trd.detach();
    }
}

map<string,string> Scheduler::getBestCSD(ScanInfo *scanInfo){
    map<string,string> bestCSDList; 
    switch(SCHEDULING_ALGORITHM){
        case(DCS):{
            bestCSDList = DCS_Algorithm(scanInfo);
            break;
        }case(DSI):{
            bestCSDList = DSI_Algorithm(scanInfo);
            break;
        }case(RANDOM):{
            bestCSDList = Random(scanInfo);
            break;
        }case(AUTO_SELECTION):{
            bestCSDList = Auto_Selection(scanInfo);
            break;
        }
    }
    
    return bestCSDList;
}


map<string,string> Scheduler::DCS_Algorithm(ScanInfo *scanInfo){
    map<string,string> bestcsd;
    string csd_id;
    string sst_name;
    //csd 전체 info, key : csd id, value: csd 정보
    unordered_map<string,struct CSDStatusManager::CSDInfo> csd_info = CSDStatusManager::GetCSDInfoAll();
    //sst 파일을 가지고 있는 csd info, key : csd id, value: csd 정보
    unordered_map<string,struct CSDStatusManager::CSDInfo> having_sst_csd_info;
    //1. Table하나에 존재하는 SST 파일 횟수만큼 
    int sst_csd_map_size = scanInfo->sst_csd_map_size();
    int csd_list_size;
    float bestScore;
    float score;
    string bestCSD;
    
    KETILOG::DEBUGLOG("Offloading", "Depends on CSD Status");
    for(int i=0; i < sst_csd_map_size; i++){
        //2. sst 파일을 가지고 있는 csd만 저장
        int csd_count = scanInfo->sst_csd_map(i).csd_list_size();
        bestScore = INT_MAX;
        having_sst_csd_info.clear();
        //csd가 1개면 스케줄링 할 필요 없으니까 pass
        if(csd_count == 1){
            KETILOG::DEBUGLOG("Offloading", "sst-csd count : 1");

            sst_name = scanInfo->sst_csd_map(i).sst_name();
            csd_id = scanInfo->sst_csd_map(i).csd_list(0);
            KETILOG::DEBUGLOG("Offloading", "sst name " + sst_name + ", id : " + csd_id);
            bestcsd[sst_name] = csd_id;
            continue;
        }

        //DCS스케줄링
        KETILOG::DEBUGLOG("Offloading", "sst-csd count : n");

        csd_list_size = scanInfo->sst_csd_map(i).csd_list_size();
        for(int j=0;j<csd_list_size; j++){
            //sst파일을 가지고 있는 csd 메트릭 정보
            sst_name = scanInfo->sst_csd_map(i).sst_name();
            csd_id = scanInfo->sst_csd_map(i).csd_list(j);
            having_sst_csd_info[csd_id] = csd_info[csd_id];
            KETILOG::DEBUGLOG("Offloading", "Metric Information for " + csd_id + " with " + sst_name);
        }

        for(unordered_map<string,struct CSDStatusManager::CSDInfo>::iterator itr=having_sst_csd_info.begin(); itr !=having_sst_csd_info.end(); ++itr){
            score = itr->second.analysis_score / itr->second.working_block_count;
            KETILOG::DEBUGLOG("Offloading", itr->first + "'s score : " + std::to_string(score));
            if(score < bestScore){
                bestCSD = itr->first;
                bestScore = score;
            }
        }
        bestcsd[sst_name] = bestCSD; 
        csd_info[bestCSD].analysis_score += 10; // 가중치를 준다
        
    }
    
    for(map<string,string>::iterator itr=bestcsd.begin(); itr !=bestcsd.end(); ++itr){
        KETILOG::DEBUGLOG("Offloading", itr-> first + " : " + itr->second);
    }

    //<1, CSDInfo(cpu_ip,working_block_count,analysis_score)>,15,50
    //<3, CSDInfo(cpu_ip,working_block_count,analysis_score)>,45,80
    //<4, CSDInfo(cpu_ip,working_block_count,analysis_score)>,60,20
    //analysis_score/working_block_count 수가 낮은게 best csd
    
    //Score_map 설정 
    //<1,3.33333>
    //<3,1.77777>
    //<4,0.33333>

    return bestcsd;
}
string Scheduler::DCS_algorithm(vector<string> dcs_candidate_csd){//DSI용
    CSDStatusManager::CSDInfo csd1 = {"ip1",0,0,0,0,15,50};
    CSDStatusManager::CSDInfo csd2 = {"ip2",0,0,0,0,30,60};
    CSDStatusManager::CSDInfo csd3 = {"ip3",0,0,0,0,45,80};
    CSDStatusManager::CSDInfo csd4 = {"ip4",0,0,0,0,60,20};

    CSDStatusManager::SetCSDInfo("1",csd1);
    CSDStatusManager::SetCSDInfo("2",csd2);
    CSDStatusManager::SetCSDInfo("3",csd3);
    CSDStatusManager::SetCSDInfo("4",csd4);
    string bestcsd = "";
    string csd_id;
    float bestScore = INT_MAX;
    float score;
    unordered_map<string,struct CSDStatusManager::CSDInfo> csd_info = CSDStatusManager::GetCSDInfoAll();
    unordered_map<string,struct CSDStatusManager::CSDInfo> having_sst_csd_info;
    KETILOG::DEBUGLOG("Offloading", "DCS Algorithm");
    for(int i=0;i<dcs_candidate_csd.size();i++){
        csd_id = dcs_candidate_csd.at(i);
        having_sst_csd_info[csd_id] = csd_info[csd_id];
        KETILOG::DEBUGLOG("Offloading", "csd id : " + csd_id);
    }
    for(unordered_map<string, struct CSDStatusManager::CSDInfo>::iterator itr=having_sst_csd_info.begin(); itr != having_sst_csd_info.end();++itr){
        score = itr->second.analysis_score / itr->second.working_block_count;
        KETILOG::DEBUGLOG("Offloading", itr->first + "'s score" + std::to_string(score));
        if(score < bestScore){
            bestcsd = itr->first;
            bestScore = score;
        }
    }
    
    return bestcsd;
} 

map<string,string> Scheduler::DSI_Algorithm(ScanInfo *scanInfo){
    map<string,string> bestcsd;
    map<string, vector<int>> csd_sst_count; // csd가 가지고 있는 sst 전체 파일 수, 0번째는 sst수, 1번째는 스케줄링 횟수  
    int sst_csd_map_size = scanInfo->sst_csd_map_size();
    int csd_list_size ;
    string sst_name;
    string csd_id;
    string bestCSD="";
    int bestscore = INT_MAX;
    int score ;
    int scheduling_count;
    int best_scheduling_count = INT_MAX;
    vector<string> candidate_csd;
    vector<string> dcs_candidate_csd;
    //1. csd가 가지고 있는 파일 수 저장
    KETILOG::DEBUGLOG("Offloading", "Depends on SST Information");
    for(int i = 0;i<sst_csd_map_size;i++){
        sst_name = scanInfo->sst_csd_map(i).sst_name();
        csd_list_size = scanInfo->sst_csd_map(i).csd_list_size();
        
        for(int j=0; j < csd_list_size; j++){
            
            csd_id = scanInfo->sst_csd_map(i).csd_list(j);

            for(int k=0; k < 2; k++){
                csd_sst_count[csd_id].push_back(0);
            }

            csd_sst_count[csd_id].at(0)++;
            KETILOG::DEBUGLOG("Offloading", "csd id : " + csd_sst_count[csd_id].at(0));
        }
    }
    for(map<string,vector<int>>::iterator itr = csd_sst_count.begin(); itr!= csd_sst_count.end(); ++itr){
        KETILOG::DEBUGLOG("Offloading", itr->first + " - sst file count : " + std::to_string(itr->second.at(0)));
    }
    //2. DSI 스케줄링
    for(int i=0; i<sst_csd_map_size;i++){

        sst_name = scanInfo->sst_csd_map(i).sst_name();
        csd_list_size = scanInfo->sst_csd_map(i).csd_list_size();
        candidate_csd.clear();

        if(csd_list_size == 1){
            KETILOG::DEBUGLOG("Offloading", "sst-csd count : 1 ");

            csd_id = scanInfo->sst_csd_map(i).csd_list(0);
            KETILOG::DEBUGLOG("Offloading", "sst name " + sst_name + ", id : " + csd_id);
            bestcsd[sst_name] = csd_id;
            continue;
        }
        //1. 최소 스케줄링 수 찾기
        best_scheduling_count = INT_MAX;
        for(int j=0; j < csd_list_size; j++){
            csd_id = scanInfo->sst_csd_map(i).csd_list(j);
            scheduling_count = csd_sst_count[csd_id].at(1);
            sst_name = scanInfo->sst_csd_map(i).sst_name();
            KETILOG::DEBUGLOG("Offloading", csd_id + "'s scheduling count : " + std::to_string(scheduling_count));
            if(scheduling_count < best_scheduling_count){
                best_scheduling_count = scheduling_count;
            }
        }
        KETILOG::DEBUGLOG("Offloading", "best schduling count : " + best_scheduling_count);

        // 2. 최소 스케줄링 수를 가지고 있는 candidate_csd vector 생성
        for(int j=0; j < csd_list_size; j++){
            csd_id = scanInfo->sst_csd_map(i).csd_list(j);
            scheduling_count = csd_sst_count[csd_id].at(1);
            if(best_scheduling_count == scheduling_count){
                candidate_csd.push_back(csd_id);
            }
        }
        if(candidate_csd.size() == 1){
            KETILOG::DEBUGLOG("Offloading", "candidate's count is 1");
            bestCSD = candidate_csd.at(0);
            bestcsd[sst_name] = bestCSD;
            csd_sst_count[bestCSD].at(1)++;
            KETILOG::DEBUGLOG("Offloading", "bsetCSD is" + bestCSD);
        }
        //3. 스케줄링 수가 같으면 중복도 수 비교
        else{
            KETILOG::DEBUGLOG("Offloading", "candidates are many, sst file count compare");
            bestscore = INT_MAX;
            KETILOG::DEBUGLOG("Offloading", "sst file candidates");
            dcs_candidate_csd.clear();
            for(int j = 0; j < candidate_csd.size(); j++){
                KETILOG::DEBUGLOG("Offloading", candidate_csd.at(j) + ", ");
            }
            //최소 파일 수 찾기
            for(int j = 0; j < candidate_csd.size(); j++){
                csd_id = candidate_csd.at(j);
                score = csd_sst_count[csd_id].at(0);
                 
                if(score < bestscore){
                    bestscore = score;
                    bestCSD = csd_id;
                }
            }
            //최소 파일 수 를 가지고 있는 csd 후보자들 찾기
            for(int j = 0; j < candidate_csd.size(); j++){
                csd_id = candidate_csd.at(j);
                score = csd_sst_count[csd_id].at(0);
                if(bestscore == score){
                    dcs_candidate_csd.push_back(csd_id);
                }
            }
            KETILOG::DEBUGLOG("Offloading", "DCS candidates : ");
            for(int j=0; j < dcs_candidate_csd.size(); j++){
                KETILOG::DEBUGLOG("Offloading", dcs_candidate_csd.at(j) + ", ");
            }
            if(dcs_candidate_csd.size() == 1){
                KETILOG::DEBUGLOG("Offloading", "DCS candidates count is 1 ");
                bestCSD = dcs_candidate_csd.at(0);
            }
            else{
                bestCSD = DCS_algorithm(dcs_candidate_csd);
            }
            KETILOG::DEBUGLOG("Offloading", "bsetCSD is " + bestCSD);
            bestcsd[sst_name] = bestCSD;
            csd_sst_count[bestCSD].at(1)++;
        }
    }
    for(map<string,string>::iterator itr=bestcsd.begin(); itr !=bestcsd.end(); ++itr){
        KETILOG::DEBUGLOG("Offloading", itr->first + " : " + itr->second);
    }
    return bestcsd;
}
map<string,string> Scheduler::Random(ScanInfo *scanInfo){
    map<string,string> bestcsd;
    string csd_id;
    string sst_name;
    srand(time(NULL));
    int random_num ;
    int sst_csd_map_size = scanInfo->sst_csd_map_size();
    vector<string> csd_list;
    KETILOG::DEBUGLOG("Offloading", " Random " );
    for(int i=0;i<sst_csd_map_size;i++){
        csd_list.clear();
        int csd_count = scanInfo->sst_csd_map(i).csd_list_size();
        if(csd_count == 1){
            KETILOG::DEBUGLOG("Offloading", "sst-csd count : 1 ");
            sst_name = scanInfo->sst_csd_map(i).sst_name();
            csd_id = scanInfo->sst_csd_map(i).csd_list(0);
            KETILOG::DEBUGLOG("Offloading", "sst name : " + sst_name + ", id : " + csd_id);
            bestcsd[sst_name] = csd_id;
            continue;
        }
        for(int j = 0; j<csd_count; j++){
            csd_list.push_back(scanInfo->sst_csd_map(i).csd_list(j));
            KETILOG::DEBUGLOG("Offloading", "csd_list(" + std::to_string(j) + ") is " + csd_list.at(j));
        }
        random_num = rand() % csd_count ;
        KETILOG::DEBUGLOG("Offloading", "random number is " + random_num);
        sst_name = scanInfo->sst_csd_map(i).sst_name();
        csd_id = csd_list.at(random_num);
        bestcsd[sst_name] = csd_id;
    }
    for(map<string,string>::iterator itr=bestcsd.begin(); itr !=bestcsd.end(); ++itr){
        KETILOG::DEBUGLOG("Offloading", itr-> first + " : " + itr->second);
    }
    return bestcsd;
}
map<string,string> Scheduler::Auto_Selection(ScanInfo *scanInfo){
    map<string, vector<int>> csd_sst_count;
    map<string,string> bestcsd;
    float total_csd_num;
    float total_sst_num = 0;
    int sst_csd_map_size = scanInfo->sst_csd_map_size();
    int csd_list_size;
    string sst_name;
    string csd_id;
    KETILOG::DEBUGLOG("Offloading", "Auto" );
    //1. csd가 가지고 있는 파일 수 저장
    for(int i = 0;i<sst_csd_map_size;i++){
        sst_name = scanInfo->sst_csd_map(i).sst_name();
        csd_list_size = scanInfo->sst_csd_map(i).csd_list_size();
        
        for(int j=0; j < csd_list_size; j++){
            
            csd_id = scanInfo->sst_csd_map(i).csd_list(j);

            for(int k=0; k < 2; k++){
                csd_sst_count[csd_id].push_back(0);
            }

            csd_sst_count[csd_id].at(0)++;
            KETILOG::DEBUGLOG("Offloading", csd_id + " : " + std::to_string(csd_sst_count[csd_id].at(0)));
        }
    }
    total_csd_num = csd_sst_count.size();
    for(map<string,vector<int>>::iterator itr = csd_sst_count.begin(); itr!= csd_sst_count.end(); ++itr){
        total_sst_num = total_sst_num + itr->second.at(0);
    }
    KETILOG::DEBUGLOG("Offloading", "total csd count : " + std::to_string(total_csd_num));
    KETILOG::DEBUGLOG("Offloading", ", total sst count : " + std::to_string(total_sst_num));

    if(total_sst_num/total_csd_num < 4){
        KETILOG::DEBUGLOG("Offloading", "DCS Algorithm");
        bestcsd = DCS_Algorithm(scanInfo);
    }
    else {
        KETILOG::DEBUGLOG("Offloading", "DSI Algorithm");
        bestcsd = DSI_Algorithm(scanInfo);
    }
    return bestcsd;
}
//map sst가 key, 선정 csd가 value

void Scheduler::t_snippet_scheduling(TmaxRequest request){
    KETILOG::DEBUGLOG("Offloading", "<T> scheduling tmax snippet...");

    t_offloading_snippet(request, "1");
    return;
}

void Scheduler::t_offloading_snippet(TmaxRequest request, string csd_id){
    KETILOG::DEBUGLOG("Offloading", "<T> called t_offloading_snippet");
    KETILOG::DEBUGLOG("Offloading", "<T> parsing tmax snippet...");

    StringBuffer snippetbuf;
    Writer<StringBuffer> writer(snippetbuf);

    writer.StartObject();

    writer.Key("tmax");
    string csdIP = "10.1."+csd_id+".2";
    writer.String(csdIP.c_str());

    writer.Key("query_id");
    writer.Int(request.query_id());

    writer.Key("work_id");
    writer.Int(request.work_id());

    writer.Key("file_name");
    writer.String(request.file_name().c_str());

    writer.Key("table_filter");
    writer.String(request.table_filter().c_str());

    writer.Key("column_projection");
    writer.String(request.column_projection().c_str());

    writer.Key("chunks");
    writer.StartArray();
    for (int i = 0; i < request.chunks_size(); i++){
        writer.StartObject();
        writer.Key("offset");
        writer.Int(request.chunks(i).offset());
        writer.Key("length");
        writer.Int(request.chunks(i).length());
        writer.EndObject();
    }
    writer.EndArray();

    writer.EndObject();

    string snippet_json = snippetbuf.GetString();

    // Check Send Snippet
    {
    cout << endl << snippet_json << endl << endl; 
    }

    KETILOG::DEBUGLOG("Offloading", "<T> send tmax snippet...");

    int sock = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    std::istringstream port_((string)CSD_IDENTIFIER_PORT);
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