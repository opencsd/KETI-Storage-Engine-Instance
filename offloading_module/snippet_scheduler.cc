#include "snippet_scheduler.h"

//한 스니펫에서 테이블 1개를 스캔

void Scheduler::runScheduler(){ 
    while (1){
        SnippetRequest snippet = Scheduler::PopQueue();
        //임시 ScanInfo
        // SnippetRequest snippet;

        ScanInfo* scan_info =snippet.mutable_scan_info();;
        
        // ScanInfo_SSTInfo* scaninfo_sstinfo = scan_info->add_sst_csd_map_info();
        // scaninfo_sstinfo->set_sst_name("10.sst");
        // scaninfo_sstinfo->add_csd_list("1");
        // scaninfo_sstinfo->add_csd_list("2");
        // scaninfo_sstinfo->add_csd_list("3");

        // scaninfo_sstinfo = scan_info->add_sst_csd_map_info();
        // scaninfo_sstinfo->set_sst_name("11.sst");
        // scaninfo_sstinfo->add_csd_list("1");
        // scaninfo_sstinfo->add_csd_list("2");
        // scaninfo_sstinfo->add_csd_list("4");

        // scaninfo_sstinfo = scan_info->add_sst_csd_map_info();
        // scaninfo_sstinfo->set_sst_name("12.sst");
        // scaninfo_sstinfo->add_csd_list("1");
        // scaninfo_sstinfo->add_csd_list("3");
        // scaninfo_sstinfo->add_csd_list("4");


        // scaninfo_sstinfo = scan_info->add_sst_csd_map_info();
        // scaninfo_sstinfo->set_sst_name("13.sst");
        // scaninfo_sstinfo->add_csd_list("2");
        // scaninfo_sstinfo->add_csd_list("3");
        // scaninfo_sstinfo->add_csd_list("4");

        // scaninfo_sstinfo = scan_info->add_sst_csd_map_info();
        // scaninfo_sstinfo->set_sst_name("14.sst");
        // scaninfo_sstinfo->add_csd_list("1");
        // scaninfo_sstinfo->add_csd_list("2");
        // scaninfo_sstinfo->add_csd_list("3");

        // scaninfo_sstinfo = scan_info->add_sst_csd_map_info();
        // scaninfo_sstinfo->set_sst_name("15.sst");
        // scaninfo_sstinfo->add_csd_list("1");
        // scaninfo_sstinfo->add_csd_list("2");
        // scaninfo_sstinfo->add_csd_list("4");

        int size = scan_info->sst_csd_map_info_size();
        map<string,string> bestcsd ;
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
    //임시 CSD 정보 값 
    
    // CSDStatusManager::CSDInfo csd1 = {"ip1",0,0,0,0,15,50};
    // CSDStatusManager::CSDInfo csd2 = {"ip2",0,0,0,0,15,50};
    // CSDStatusManager::CSDInfo csd3 = {"ip3",0,0,0,0,15,50};
    // CSDStatusManager::CSDInfo csd4 = {"ip4",0,0,0,0,15,50};

    // CSDStatusManager::SetCSDInfo("1",csd1);
    // CSDStatusManager::SetCSDInfo("2",csd2);
    // CSDStatusManager::SetCSDInfo("3",csd3);
    // CSDStatusManager::SetCSDInfo("4",csd4);
    map<string,string> bestcsd;
    string csd_id;
    string sst_name;
    //csd 전체 info, key : csd id, value: csd 정보
    unordered_map<string,struct CSDStatusManager::CSDInfo> csd_info = CSDStatusManager::GetCSDInfoAll();
    //sst 파일을 가지고 있는 csd info, key : csd id, value: csd 정보
    unordered_map<string,struct CSDStatusManager::CSDInfo> having_sst_csd_info;
    //1. Table하나에 존재하는 SST 파일 횟수만큼 
    int sst_csd_map_info_size = scanInfo->sst_csd_map_info_size();
    int csd_list_size;
    float bestScore;
    float score;
    string bestCSD;
    cout << endl << "Depends on CSD Status" << endl;

    for(int i=0; i < sst_csd_map_info_size; i++){
        //2. sst 파일을 가지고 있는 csd만 저장
        int csd_count = scanInfo->sst_csd_map_info(i).csd_list_size();
        bestScore = INT_MAX;
        having_sst_csd_info.clear();
        //csd가 1개면 스케줄링 할 필요 없으니까 pass
        if(csd_count == 1){
            cout << "sst-csd 1"<< endl;
            sst_name = scanInfo->sst_csd_map_info(i).sst_name();
            csd_id = scanInfo->sst_csd_map_info(i).csd_list(0);
            cout << "sst name : " << sst_name << ", id : " << csd_id << endl;
            bestcsd[sst_name] = csd_id;
            continue;
        }

        //DCS스케줄링
        csd_list_size = scanInfo->sst_csd_map_info(i).csd_list_size();
        for(int j=0;j<csd_list_size; j++){
            //sst파일을 가지고 있는 csd 메트릭 정보
            sst_name = scanInfo->sst_csd_map_info(i).sst_name();
            csd_id = scanInfo->sst_csd_map_info(i).csd_list(j);
            having_sst_csd_info[csd_id] = csd_info[csd_id];
            // cout << sst_name <<"를 가지고 있는 csd : " << csd_id <<", 메트릭 정보 : "  << having_sst_csd_info[csd_id].csd_ip << ", " <<endl;
        }

        for(unordered_map<string,struct CSDStatusManager::CSDInfo>::iterator itr=having_sst_csd_info.begin(); itr !=having_sst_csd_info.end(); ++itr){
            score = itr->second.analysis_score / itr->second.working_block_count;
            cout << itr->first << "' score : " << score <<endl;
            if(score < bestScore){
                bestCSD = itr->first;
                bestScore = score;
            }
        }
        bestcsd[sst_name] = bestCSD; 
        csd_info[bestCSD].analysis_score += 10; // 가중치를 준다
        cout << endl;
    }
    cout <<endl;
    for(map<string,string>::iterator itr=bestcsd.begin(); itr !=bestcsd.end(); ++itr){
        cout << itr-> first << " : " << itr->second <<endl;
    }
    //<1, CSDInfo(cpu_ip,working_block_count,analysis_score)>,15,50
    //<3, CSDInfo(cpu_ip,working_block_count,analysis_score)>,45,80
    //<4, CSDInfo(cpu_ip,working_block_count,analysis_score)>,60,20
    //analysis_score/working_block_count 수가 낮은게 best csd
    
    //Score_map 설정 
    //<1,3.33333>
    //<3,1.77777>
    //<4,0.33333>
    // cout<<"sst파일명: "<<scanInfo->sst_csd_map_info(0).sst_name()<<endl;
    // cout<<"sst파일명: "<<scanInfo->sst_csd_map_info(1).sst_name()<<endl;
    // cout<<"sst파일명: "<<scanInfo->sst_csd_map_info(2).sst_name()<<endl;    


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
    for(int i=0;i<dcs_candidate_csd.size();i++){
        csd_id = dcs_candidate_csd.at(i);
        having_sst_csd_info[csd_id] = csd_info[csd_id];
        // cout << "csd id : " << csd_id << " " << endl; 
    }
    cout << endl;
    for(unordered_map<string, struct CSDStatusManager::CSDInfo>::iterator itr=having_sst_csd_info.begin(); itr != having_sst_csd_info.end();++itr){
        score = itr->second.analysis_score / itr->second.working_block_count;
        cout << itr->first << "'s score : " << score <<endl;
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
    int sst_csd_map_info_size = scanInfo->sst_csd_map_info_size();
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
    cout << endl << "Depends on SST Information" << endl;

    for(int i = 0;i<sst_csd_map_info_size;i++){
        sst_name = scanInfo->sst_csd_map_info(i).sst_name();
        csd_list_size = scanInfo->sst_csd_map_info(i).csd_list_size();
        
        for(int j=0; j < csd_list_size; j++){
            
            csd_id = scanInfo->sst_csd_map_info(i).csd_list(j);

            for(int k=0; k < 2; k++){
                csd_sst_count[csd_id].push_back(0);
            }

            csd_sst_count[csd_id].at(0)++;
            // cout << "csd_id : " << csd_sst_count[csd_id].at(0)<<endl;
        }
    }
    cout << endl;
    for(map<string,vector<int>>::iterator itr = csd_sst_count.begin(); itr!= csd_sst_count.end(); ++itr){
        cout << itr->first << " - SST file count : " <<itr->second.at(0) <<endl;
    }
    //2. DSI 스케줄링
    for(int i=0; i<sst_csd_map_info_size;i++){

        sst_name = scanInfo->sst_csd_map_info(i).sst_name();
        csd_list_size = scanInfo->sst_csd_map_info(i).csd_list_size();
        candidate_csd.clear();
        cout << sst_name << " for best csd " << endl;
        if(csd_list_size == 1){
            cout << "sst-csd 1"<< endl;
            csd_id = scanInfo->sst_csd_map_info(i).csd_list(0);
            cout << "sst name : " << sst_name << ", id : " << csd_id << endl;
            bestcsd[sst_name] = csd_id;
            continue;
        }
        //1. 최소 스케줄링 수 찾기
        best_scheduling_count = INT_MAX;
        for(int j=0; j < csd_list_size; j++){
            csd_id = scanInfo->sst_csd_map_info(i).csd_list(j);
            scheduling_count = csd_sst_count[csd_id].at(1);
            sst_name = scanInfo->sst_csd_map_info(i).sst_name();
            cout << csd_id << "'s schduling count : " << scheduling_count << " ,";
            if(scheduling_count < best_scheduling_count){
                best_scheduling_count = scheduling_count;
            }
        }
        cout << "best schduling count : " << best_scheduling_count << endl; 
        //2. 최소 스케줄링 수를 가지고 있는 candidate_csd vector 생성
        for(int j=0; j < csd_list_size; j++){
            csd_id = scanInfo->sst_csd_map_info(i).csd_list(j);
            scheduling_count = csd_sst_count[csd_id].at(1);
            if(best_scheduling_count == scheduling_count){
                candidate_csd.push_back(csd_id);
            }
        }
        if(candidate_csd.size() == 1){
            cout << "candidate is 1" << endl;
            bestCSD = candidate_csd.at(0);
            bestcsd[sst_name] = bestCSD;
            csd_sst_count[bestCSD].at(1)++;
            cout << "bsetCSD is " << bestCSD<<endl;
        }
        //3. 스케줄링 수가 같으면 중복도 수 비교
        else{
            cout << "candidates are many, sst file count compare" << endl;
            bestscore = INT_MAX;
            cout<< "sst file candidates : " ;
            dcs_candidate_csd.clear();
            for(int j = 0; j < candidate_csd.size(); j++){
                cout << candidate_csd.at(j) << " ,";
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
            cout << "DCS candidates : " ;
            for(int j=0; j < dcs_candidate_csd.size(); j++){
                cout << dcs_candidate_csd.at(j) << ", " ;
            }
            if(dcs_candidate_csd.size() == 1){
                cout << "DCS candidates is 1 " ;
                bestCSD = dcs_candidate_csd.at(0);
            }
            else{
                bestCSD = DCS_algorithm(dcs_candidate_csd);

            }

            cout << endl <<"bsetCSD is " << bestCSD<<endl;
            bestcsd[sst_name] = bestCSD;
            csd_sst_count[bestCSD].at(1)++;
        }
        cout << endl;
    }
    for(map<string,string>::iterator itr=bestcsd.begin(); itr !=bestcsd.end(); ++itr){
        cout << itr-> first << " : " << itr->second <<endl;
    }
    return bestcsd;
}
map<string,string> Scheduler::Random(ScanInfo *scanInfo){
    map<string,string> bestcsd;
    string csd_id;
    string sst_name;
    srand(time(NULL));
    int random_num ;
    int sst_csd_map_info_size = scanInfo->sst_csd_map_info_size();
    vector<string> csd_list;
    cout << "random" << endl; 

    for(int i=0;i<sst_csd_map_info_size;i++){
        csd_list.clear();
        int csd_count = scanInfo->sst_csd_map_info(i).csd_list_size();
        if(csd_count == 1){
            cout << "sst-csd 1"<< endl;
            sst_name = scanInfo->sst_csd_map_info(i).sst_name();
            csd_id = scanInfo->sst_csd_map_info(i).csd_list(0);
            cout << "sst name : " << sst_name << ", id : " << csd_id << endl;
            bestcsd[sst_name] = csd_id;
            continue;
        }
        for(int j = 0; j<csd_count; j++){
            csd_list.push_back(scanInfo->sst_csd_map_info(i).csd_list(j));
            // cout << "csd_list(" << j << ")값은 " <<csd_list.at(j) << endl;
        }
        random_num = rand() % csd_count ;
        // cout << "random 수는 : " <<random_num << endl;
        sst_name = scanInfo->sst_csd_map_info(i).sst_name();
        csd_id = csd_list.at(random_num);
        bestcsd[sst_name] = csd_id;
    }
    for(map<string,string>::iterator itr=bestcsd.begin(); itr !=bestcsd.end(); ++itr){
        cout << itr-> first << " : " << itr->second <<endl;
    }
    return bestcsd;
}
map<string,string> Scheduler::Auto_Selection(ScanInfo *scanInfo){
    map<string, vector<int>> csd_sst_count;
    map<string,string> bestcsd;
    float total_csd_num;
    float total_sst_num = 0;
    int sst_csd_map_info_size = scanInfo->sst_csd_map_info_size();
    int csd_list_size;
    string sst_name;
    string csd_id;
    //1. csd가 가지고 있는 파일 수 저장
    for(int i = 0;i<sst_csd_map_info_size;i++){
        sst_name = scanInfo->sst_csd_map_info(i).sst_name();
        csd_list_size = scanInfo->sst_csd_map_info(i).csd_list_size();
        
        for(int j=0; j < csd_list_size; j++){
            
            csd_id = scanInfo->sst_csd_map_info(i).csd_list(j);

            for(int k=0; k < 2; k++){
                csd_sst_count[csd_id].push_back(0);
            }

            csd_sst_count[csd_id].at(0)++;
            // cout << "csd_id : " << csd_sst_count[csd_id].at(0)<<endl;
        }
    }
    total_csd_num = csd_sst_count.size();
    for(map<string,vector<int>>::iterator itr = csd_sst_count.begin(); itr!= csd_sst_count.end(); ++itr){
        total_sst_num = total_sst_num + itr->second.at(0);
    }
    cout << "auto" << endl; 

    cout << "total csd count :" <<total_csd_num << ", total sst count : " << total_sst_num << endl;
    if(total_sst_num/total_csd_num < 4){
        cout << "DCS Algorithm" << endl;
        bestcsd = DCS_Algorithm(scanInfo);
    }
    else {
        cout << "DSI Algorithm" << endl;
        bestcsd = DSI_Algorithm(scanInfo);
    }
    return bestcsd;
}
//map sst가 key, 선정 csd가 value