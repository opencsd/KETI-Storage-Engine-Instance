#include "snippet_scheduler.h"

void Scheduler::runScheduler(){ 
    while (1){
        SnippetRequest snippet = Scheduler::PopQueue();

        //get best csd
        map<string,string> bestcsd = getBestCSD(snippet.scan_info());

        //make adding PBA,WAL and sending snippet to csd thread
        std::thread trd(&SnippetManager::SetupSnippet, SnippetManager::GetInstance(), snippet, bestcsd);
        trd.detach();
    }
}

map<string,string> Scheduler::getBestCSD(ScanInfo scanInfo){
    map<string,string> bestCSDList;

    switch(SCHEDULING_ALGORITHM){
        case(ROUND_ROBBIN):{
            bestCSDList = RoundRobbin(scanInfo);
            break;
        }case(ALGORITHM_AUTO_SELECT):{
            bestCSDList = AlgorithmAutoSelection(scanInfo);
            break;
        }case(FILE_DISTRIBUTION):{
            bestCSDList = FileDistribution(scanInfo);
            break;
        }case(CSD_METRIC_SCORE):{
            bestCSDList = CSDMetricScore(scanInfo);
            break;
        }
    }
    
    return bestCSDList;
}

map<string,string> Scheduler::CSDMetricScore(ScanInfo scanInfo){
    map<string,string> bestcsd;

    return bestcsd;
}

map<string,string> Scheduler::FileDistribution(ScanInfo scanInfo){
    map<string,string> bestcsd;

    return bestcsd;
}

map<string,string> Scheduler::RoundRobbin(ScanInfo scanInfo){
    map<string,string> bestcsd;

    for(int i=0; i<scanInfo.block_info_size(); i++){
        string sst_name = scanInfo.block_info(i).sst_name();
        string selected_csd = scanInfo.block_info(i).csd_list(0);
        bestcsd[sst_name] = selected_csd;
    }

    return bestcsd;
}

map<string,string> Scheduler::AlgorithmAutoSelection(ScanInfo scanInfo){
    map<string,string> bestcsd;

    return bestcsd;
}
