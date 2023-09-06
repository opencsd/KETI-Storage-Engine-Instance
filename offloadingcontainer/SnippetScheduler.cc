#include "SnippetScheduler.h"
#include "CSDStatusManager.h"
#include "MonitoringContainerInterface.h"
#include "keti_log.h"

using StorageEngineInstance::Snippet;
using StorageEngineInstance::MetaDataResponse_PBAInfo;

void Scheduler::runScheduler(){ 
    while (1){
        Object scheduling_target = Scheduler::PopQueue();

        MetaDataResponse schedulerinfo;

        Monitoring_Container_Interface mc(grpc::CreateChannel((std::string)LOCALHOST+":"+std::to_string(SE_MONITORING_CONTAINER_PORT), grpc::InsecureChannelCredentials()));
        schedulerinfo = mc.GetMetaData(scheduling_target.snippet.query_id(), scheduling_target.snippet.work_id(), scheduling_target.snippet.table_name(0));
        
        scheduling_target.metadata = schedulerinfo;

        string id = "{" + to_string(scheduling_target.snippet.query_id()) + "|" + to_string(scheduling_target.snippet.work_id()) + "}";
        KETILOG::DEBUGLOG(LOGTAG, "# Snippet Scheduling Start " + id);

        //sstfile 단위로 for문으로 스케줄링 수행
        const auto& my_map = scheduling_target.metadata.sst_csd_map();
        for (const auto& entry : my_map) {
            string sst_name = entry.first; 
            MetaDataResponse_PBAInfo pba_info = entry.second;
            Scheduler::scheduling(scheduling_target, sst_name, pba_info);
        }
    }
}

void Scheduler::scheduling(Object &scheduling_target, string sst_name, MetaDataResponse_PBAInfo pba_info){
    //get best csd
    string bestcsd = bestCSD(scheduling_target, sst_name, pba_info);//get best csd
    string pba = pba_info.csd_pba_map().at(bestcsd);

    //make snippet json
    StringBuffer snippetbuf;
    snippetbuf.Clear();
    serialize(snippetbuf, scheduling_target.snippet, bestcsd, pba);

    //send snippet
    KETILOG::DEBUGLOG(LOGTAG, "# Send Snippet(" + sst_name + ") to CSD Worker Module [CSD" + bestcsd + "]");
    string snippetJson = snippetbuf.GetString();
    // CSD IP 형식 : "10.1.${id}.2" 
    //         ex) 10.1.1.2 ~ 10.1.8.2
    string csdIP = "";
    std::thread trd(&Scheduler::sendSnippetToCSD, &Scheduler::GetInstance(), snippetJson);
    trd.detach();
}

void Scheduler::serialize(StringBuffer &buff, Snippet &snippet, string bestcsd, string pba) {
    Writer<StringBuffer> writer(buff);

    writer.StartObject();
    writer.Key("Snippet"); 

    writer.StartObject();

    writer.Key("queryID");
    writer.Int(snippet.query_id());

    writer.Key("workID");
    writer.Int(snippet.work_id());

    writer.Key("tableName");
    writer.String(snippet.table_name(0).c_str());

    writer.Key("tableCol");
    writer.StartArray();
    for (int i = 0; i < snippet.table_col_size(); i++){
        writer.String(snippet.table_col(i).c_str());
    }
    writer.EndArray();

    writer.Key("tableFilter");
    writer.StartArray();
    if(snippet.table_filter_size() > 0){
        for (int i = 0; i < snippet.table_filter().size(); i++)
        {
            writer.StartObject();
            if (snippet.table_filter(i).lv().type().size() > 0)
            {
                writer.Key("LV");
                
                if(snippet.table_filter(i).lv().type(0) == 10){
                    writer.String(snippet.table_filter()[i].lv().value(0).c_str());
                }
            }
            writer.Key("OPERATOR");
            writer.Int(snippet.table_filter(i).operator_());

            if(snippet.table_filter(i).operator_() == 8 || snippet.table_filter(i).operator_() == 9 || snippet.table_filter(i).operator_() == 16){
                //EXTRA가 DB Connector에서 올땐 RV로 오는데 여기서 EXTRA로 바뀌어서 CSD로 넘어감 -> 통일하는게 좋을듯
                writer.Key("EXTRA");
                writer.StartArray();

                for(int j = 0; j < snippet.table_filter()[i].rv().type().size(); j++){
                    if(snippet.table_filter()[i].rv().type(j) != 10){
                        if(snippet.table_filter()[i].rv().type(j) == 7 || snippet.table_filter()[i].rv().type(j) == 3){
                            string tmpstr = snippet.table_filter()[i].rv().value(j);
                            int tmpint = atoi(tmpstr.c_str());
                            writer.Int(tmpint);
                        }else if(snippet.table_filter()[i].rv().type(j) == 9){
                            string tmpstr = snippet.table_filter()[i].rv().value(j);
                            tmpstr = "+" + tmpstr;
                            writer.String(tmpstr.c_str());
                        }else if(snippet.table_filter()[i].rv().type(j) == 4 || snippet.table_filter()[i].rv().type(j) == 5){
                            string tmpstr = snippet.table_filter()[i].rv().value(j);
                            double tmpfloat = stod(tmpstr);
                            writer.Double(tmpfloat);
                        }else{
                            string tmpstr = snippet.table_filter()[i].rv().value(j);
                            tmpstr = "+" + tmpstr;
                            writer.String(tmpstr.c_str());
                        }
                    }else{
                        writer.String(snippet.table_filter()[i].rv().value(j).c_str());
                    }
                }
                writer.EndArray();
            } else if (snippet.table_filter(i).rv().type().size() > 0){
                writer.Key("RV");

                if(snippet.table_filter(i).rv().type(0) != 10){
                    if(snippet.table_filter(i).rv().type(0) == 7 || snippet.table_filter(i).rv().type(0) == 3){
                        string tmpstr = snippet.table_filter(i).rv().value(0);
                        int tmpint = atoi(tmpstr.c_str());
                        writer.Int(tmpint);
                    }else if(snippet.table_filter(i).rv().type(0) == 9){
                        string tmpstr = snippet.table_filter(i).rv().value(0);
                        tmpstr = "+" + tmpstr;
                        writer.String(tmpstr.c_str());
                    }else if(snippet.table_filter(i).rv().type(0) == 4 || snippet.table_filter(i).rv().type(0) == 5){
                        string tmpstr = snippet.table_filter(i).rv().value(0);
                        double tmpfloat = stod(tmpstr);
                        writer.Double(tmpfloat);
                    }else{
                        string tmpstr = snippet.table_filter(i).rv().value(0);
                        tmpstr = "+" + tmpstr;
                        writer.String(tmpstr.c_str());
                    }
                }else{
                    writer.String(snippet.table_filter(i).rv().value(0).c_str());
                }
            }

            writer.EndObject();
        }
    }
    writer.EndArray();
    
    // writer.Key("columnFiltering");
    // writer.StartArray();
    // for (int i = 0; i < snippet.column_filtering_size(); i++)
    // {
    //     writer.String(snippet.column_filtering()[i].c_str());
    // }
    // writer.EndArray();

    writer.Key("columnProjection");
    writer.StartArray();
    for (int i = 0; i < snippet.column_projection().size(); i++){
        writer.StartObject();
        writer.Key("selectType");
        writer.Int(snippet.column_projection(i).select_type()); 

        for(int j = 0; j < snippet.column_projection(i).value_size(); j++){
            if(j==0){
                writer.Key("value");
                writer.StartArray();
                writer.String(snippet.column_projection(i).value(j).c_str());
            }
            else{
                writer.String(snippet.column_projection(i).value(j).c_str());
            }
        }
        writer.EndArray();
        for(int j = 0; j < snippet.column_projection(i).value_type_size(); j++){
            if(j == 0){
                writer.Key("valueType");
                writer.StartArray();
                writer.Int(snippet.column_projection(i).value_type(j));
            }else{
                writer.Int(snippet.column_projection(i).value_type(j));
            }
        }
        writer.EndArray();

        if(snippet.column_projection(i).value_size() == 0){
            writer.Key("value");
            writer.StartArray();
            writer.EndArray();
            writer.Key("valueType");
            writer.StartArray();
            writer.EndArray();
        }
        writer.EndObject();
    }
    writer.EndArray();

    writer.Key("tableOffset");
    writer.StartArray();
    for (int i = 0; i < snippet.table_offset_size(); i++){
        writer.Int(snippet.table_offset()[i]);
    }
    writer.EndArray();

    writer.Key("tableOfflen");
    writer.StartArray();
    for (int i = 0; i < snippet.table_offlen_size(); i++){
        writer.Int(snippet.table_offlen()[i]);
    }
    writer.EndArray();

    writer.Key("tableDatatype");
    writer.StartArray();
    for (int i = 0; i < snippet.table_datatype_size(); i++){
        writer.Int(snippet.table_datatype()[i]);
    }
    writer.EndArray();

    writer.Key("pba");
    writer.RawValue(pba.c_str(), strlen(pba.c_str()), kObjectType);

    writer.Key("primaryKey");
    writer.Int(snippet.pk_num());

    writer.Key("csdName");
    writer.String(bestcsd.c_str());

    writer.EndObject();
    
    string csdIP = "10.1."+bestcsd+".2";
    writer.Key("csdIP");
    writer.String(csdIP.c_str());

    writer.EndObject();
}

void Scheduler::sendSnippetToCSD(string snippet_json){
    int sock;
    struct sockaddr_in serv_addr;
    sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    /*Send Snippet To CSD Proxy*/
    serv_addr.sin_addr.s_addr = inet_addr(CSD_PROXY_IP);
    serv_addr.sin_port = htons(CSD_PROXY_PORT);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    {
    // // CSD 스니펫 내용 확인 - Debug Code  
    // cout << endl << snippet_json.c_str() << endl << endl;
    }

    size_t len = strlen(snippet_json.c_str());
    send(sock, &len, sizeof(len), 0);
    send(sock, (char *)snippet_json.c_str(), strlen(snippet_json.c_str()), 0);

    close(sock);
}

string Scheduler::bestCSD(Object &scheduling_target, string sst_name, MetaDataResponse_PBAInfo &pba_info){
    string bestcsd;

    switch(SCHEDULING_ALGORITHM){
        case(ROUND_ROBBIN):{
            bestcsd = RoundRobbin(scheduling_target, sst_name, pba_info);
            break;
        }case(ALGORITHM_AUTO_SELECT):{
            bestcsd = AlgorithmAutoSelection(scheduling_target, sst_name, pba_info);
            break;
        }case(FILE_DISTRIBUTION):{
            bestcsd = FileDistribution(scheduling_target, sst_name, pba_info);
            break;
        }case(CSD_METRIC_SCORE):{
            bestcsd = CSDMetricScore(scheduling_target, sst_name, pba_info);
            break;
        }
    }
    

    return bestcsd;
}

string Scheduler::CSDMetricScore(Object &scheduling_target, string sst_name, MetaDataResponse_PBAInfo pba_info){
    string bestcsd;

    return bestcsd;
}

string Scheduler::FileDistribution(Object &scheduling_target, string sst_name, MetaDataResponse_PBAInfo pba_info){
    string bestcsd;

    return bestcsd;
}

string Scheduler::RoundRobbin(Object &scheduling_target, string sst_name, MetaDataResponse_PBAInfo pba_info){
    string bestcsd;
    const auto& my_map = pba_info.csd_pba_map();
    
    for (const auto& entry : my_map) {
        bestcsd = entry.first; //제일 첫번째 csd 리턴
    }

    return bestcsd;
}

string Scheduler::AlgorithmAutoSelection(Object &scheduling_target, string sst_name, MetaDataResponse_PBAInfo pba_info){
    string bestcsd;

    return bestcsd;
}
