#include "snippet_scheduler.h"

void Scheduler::runScheduler(){ 
    while (1){
        Snippet snippet = Scheduler::PopQueue();

        MonitoringModuleConnector mc(grpc::CreateChannel((string)LOCALHOST+":"+(string)SE_MONITORING_NODE_PORT, grpc::InsecureChannelCredentials()));
        DataFileInfo dataFileInfo = mc.GetDataFileInfo("tpc_h"/*need db name in snippet*/,snippet.table_name(0));

        //get best csd
        map<string,string> bestcsd = getBestCSD(dataFileInfo);

        //make adding PBA,WAL and sending snippet to csd thread
        std::thread trd(&Scheduler::scheduling, &Scheduler::GetInstance(), snippet, bestcsd);
        trd.detach();
    }
}

void Scheduler::scheduling(Snippet snippet, map<string,string> bestcsd){
    //get PBA & WAL
    MonitoringModuleConnector mc(grpc::CreateChannel((string)LOCALHOST+":"+(string)SE_MONITORING_NODE_PORT, grpc::InsecureChannelCredentials()));
    SnippetMetaData snippetMetaData = mc.GetSnippetMetaData("tpc_h"/*need db name in snippet*/,snippet.table_name(0), bestcsd);

    StringBuffer snippetbuf;
    for (const auto entry : snippetMetaData.sst_pba_map()) {
        snippetbuf.Clear();
        string sst = entry.first;
        serialize(snippetbuf, snippet, bestcsd[entry.first], entry.second, snippetMetaData.table_total_block_count());
        sendSnippetToCSD(snippetbuf.GetString());
    }
}

void Scheduler::serialize(StringBuffer &snippetbuf, Snippet &snippet, string csd, string pba, int table_total_block_count) {
    Writer<StringBuffer> writer(snippetbuf);

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
    writer.String(csd.c_str());

    writer.Key("tableTotalBlockCount");
    writer.Int(table_total_block_count);

    string port = "";

    port = (string)SE_MONITORING_NODE_PORT;
    
    writer.Key("storageEnginePort");
    writer.String(port.c_str());

    writer.EndObject();
    
    string csdIP = "10.1."+csd+".2";
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
    serv_addr.sin_addr.s_addr = inet_addr(STORAGE_CLUSTER_MASTER_IP);
    std::istringstream port_((string)CSD_IDENTIFIER_PORT);
    std::uint16_t port{};
    port_ >> port;
    serv_addr.sin_port = htons(port);
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

map<string,string> Scheduler::getBestCSD(DataFileInfo dataFileInfo){
    map<string,string> bestCSDList;

    switch(SCHEDULING_ALGORITHM){
        case(ROUND_ROBBIN):{
            bestCSDList = RoundRobbin(dataFileInfo);
            break;
        }case(ALGORITHM_AUTO_SELECT):{
            bestCSDList = AlgorithmAutoSelection(dataFileInfo);
            break;
        }case(FILE_DISTRIBUTION):{
            bestCSDList = FileDistribution(dataFileInfo);
            break;
        }case(CSD_METRIC_SCORE):{
            bestCSDList = CSDMetricScore(dataFileInfo);
            break;
        }
    }
    
    return bestCSDList;
}

map<string,string> Scheduler::CSDMetricScore(DataFileInfo dataFileInfo){
    map<string,string> bestcsd;

    return bestcsd;
}

map<string,string> Scheduler::FileDistribution(DataFileInfo dataFileInfo){
    map<string,string> bestcsd;

    return bestcsd;
}

map<string,string> Scheduler::RoundRobbin(DataFileInfo dataFileInfo){
    map<string,string> bestcsd;
    
    for (const auto& entry : dataFileInfo.sst_csd_map()) {
        bestcsd[entry.first] = entry.second.csd_id(0);
    }

    return bestcsd;
}

map<string,string> Scheduler::AlgorithmAutoSelection(DataFileInfo dataFileInfo){
    map<string,string> bestcsd;

    return bestcsd;
}
