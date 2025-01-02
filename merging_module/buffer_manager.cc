#include "buffer_manager.h"

void getColOffset(const char* row_data, int* col_offset_list, vector<int> return_datatype, vector<int> table_offlen);

void BufferManager::initBufferManager(){
    std::thread BufferManagerInterface(&BufferManager::bufferManagerInterface,this);
    std::thread T_BufferManagerInterface(&BufferManager::t_buffer_manager_interface,this);
    BufferManagerInterface.detach();
    T_BufferManagerInterface.detach();
}

void BufferManager::bufferManagerInterface(){
    int server_fd, client_fd;
	int opt = 1;
	struct sockaddr_in serv_addr, client_addr;
	socklen_t addrlen = sizeof(client_addr);
    static char cMsg[] = "ok";

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(SE_MERGING_TCP_PORT);
 
	if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		perror("bind");
		exit(EXIT_FAILURE);
	} 

	if (listen(server_fd, NCONNECTION) < 0){
		perror("listen");
		exit(EXIT_FAILURE);
	}

    KETILOG::WARNLOG(LOGTAG,"CSD Return Server Listening on 0.0.0.0:"+to_string(SE_MERGING_TCP_PORT));

	while(1){
		if ((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen)) < 0){
			perror("accept");
        	exit(EXIT_FAILURE);
		}

		std::string json = "";
        int njson;
		size_t ljson;

		recv(client_fd , &ljson, sizeof(ljson), 0);

        char buffer[ljson];
        memset(buffer, 0, ljson);
		
		while(1) {
			if ((njson = recv(client_fd, buffer, BUFF_SIZE-1, 0)) == -1) {
				perror("read");
				exit(1);
			}
			ljson -= njson;
		    buffer[njson] = '\0';
			json += buffer;

		    if (ljson == 0)
				break;
		}
		
        send(client_fd, cMsg, strlen(cMsg), 0);

		char data[BUFF_SIZE];
        char* dataiter = data;
		memset(data, 0, BUFF_SIZE);
        int ndata = 0;
        size_t ldata = 0;
        recv(client_fd , &ldata, sizeof(ldata),0);
		while(1) {
			if ((ndata = recv( client_fd , dataiter, ldata,0)) == -1) {
				perror("read");
				exit(1);
			}
            dataiter += ndata;
			ldata -= ndata;

		    if (ldata == 0)
				break;
		}

        send(client_fd, cMsg, strlen(cMsg), 0);
        
        pushResult(BlockResult(json.c_str(), data));
        
        close(client_fd);		
	}   
	close(server_fd);
}

void BufferManager::t_buffer_manager_interface(){
    int server_fd, client_fd;
	int opt = 1;
	struct sockaddr_in serv_addr, client_addr;
	socklen_t addrlen = sizeof(client_addr);
    static char cMsg[] = "ok";

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(T_SE_MERGING_TCP_PORT);
 
	if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		perror("bind");
		exit(EXIT_FAILURE);
	} 

	if (listen(server_fd, NCONNECTION) < 0){
		perror("listen");
		exit(EXIT_FAILURE);
	}

    KETILOG::WARNLOG(LOGTAG,"<T> CSD Return Server Listening on 0.0.0.0:"+to_string(T_SE_MERGING_TCP_PORT));

	while(1){
		if ((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen)) < 0){
			perror("accept");
        	exit(EXIT_FAILURE);
		}

        KETILOG::DEBUGLOG(LOGTAG, "<T> received csd result");
        std::string json = "";
        int njson; //처리결과 상태 저장
		size_t ljson; //문자열 길이 저장

        recv(client_fd , &ljson, sizeof(ljson), 0);

        char buffer[ljson];
        memset(buffer, 0, ljson);

        while(1) {
			if ((njson = recv(client_fd, buffer, T_BUFFER_SIZE-1, 0)) == -1) {
				perror("read");
				exit(1);
			}
			ljson -= njson;
		    buffer[njson] = '\0';
			json += buffer;

		    if (ljson == 0)
				break;
		}

		KETILOG::DEBUGLOG(LOGTAG, json);
        send(client_fd, cMsg, strlen(cMsg), 0);

        u_char data[T_BUFFER_SIZE];
        u_char* dataiter = data;
		memset(data, 0, T_BUFFER_SIZE);
        int ndata = 0;
        size_t ldata = 0;
        recv(client_fd , &ldata, sizeof(ldata),0);
        size_t data_size = ldata;

		while(1) {
			if ((ndata = recv( client_fd , dataiter, ldata,0)) == -1) {
				perror("read");
				exit(1);
			}
            dataiter += ndata;
			ldata -= ndata;
            
		    if (ldata == 0)
				break;
		}
        
        send(client_fd, cMsg, strlen(cMsg), 0);

        // /*debugg*/cout<<"buffer ";for(int t=0; t<data_size; t++){printf("%02X ",(u_char)data[t]);}cout << endl;
        
        t_result_merging(json, data, data_size);
        
        close(client_fd);		
	}   
	close(server_fd);
}

void BufferManager::pushResult(BlockResult blockResult){
    int qid = blockResult.query_id;
    int wid = blockResult.work_id;
    string table_name = blockResult.table_alias;
    
    initializeBuffer(qid, wid, table_name);

    WorkBuffer* workBuffer = DataBuffer_[qid]->work_buffer_list[wid];
    unique_lock<mutex> lock(workBuffer->mu);

    if(workBuffer->status == Initialized){
        workBuffer->table_alias = blockResult.table_alias;
        for(int i = 0; i < blockResult.column_alias.size(); i++){
            workBuffer->table_column.push_back(blockResult.column_alias[i]);
            workBuffer->table_data.insert({blockResult.column_alias[i],ColData{}});
            workBuffer->save_table_column_type(blockResult.return_datatype);
        }

        workBuffer->table_total_block_count = blockResult.table_total_block_count;
        workBuffer->status = NotFinished;

        std::thread mergeResultThread(&BufferManager::mergeResult,this, qid, wid);
        mergeResultThread.detach();
    }

    workBuffer->work_buffer_queue.push_work(blockResult);
}

void BufferManager::mergeResult(int qid, int wid){
    WorkBuffer* workBuffer = DataBuffer_[qid]->work_buffer_list[wid];

    while (1){
        BlockResult result = workBuffer->work_buffer_queue.wait_and_pop();
        unique_lock<mutex> lock(workBuffer->mu);
        
        if(result.length != 0){
            int col_type, col_offset, col_len, origin_row_len, col_count = 0;  
            string col_name;
            vector<int> new_row_offset;
            new_row_offset.assign(result.row_offset.begin(), result.row_offset.end());
            new_row_offset.push_back(result.length);

            // if(KETILOG::IsLogLevelUnder(TRACE)){
            //     // 리턴 데이터 형식 확인 - Debug Code   
            //     for(int i = 0; i<result.return_datatype.size(); i++){
            //         cout << result.return_datatype[i] << " ";
            //     }
            //     cout << endl;
            //     for(int i = 0; i<result.return_offlen.size(); i++){
            //         cout << result.return_offlen[i] << " ";
            //     }
            //     cout << endl;  
            // }

            for(int i=0; i<result.row_count; i++){
                origin_row_len = new_row_offset[i+1] - new_row_offset[i];
                char row_data[origin_row_len];
                memcpy(row_data,result.data+result.row_offset[i],origin_row_len);

                col_count = workBuffer->table_column.size();
                int col_offset_list[col_count + 1];
                getColOffset(row_data, col_offset_list, result.return_datatype, result.return_offlen);
                col_offset_list[col_count] = origin_row_len;

                for(size_t j=0; j<workBuffer->table_column.size(); j++){
                    col_name = workBuffer->table_column[j];
                    col_offset = col_offset_list[j];
                    col_len = col_offset_list[j+1] - col_offset_list[j];
                    col_type = result.return_datatype[j];                    

                    switch (col_type){
                        case MySQL_DataType::MySQL_BYTE:{
                            char tempbuf[col_len];
                            memcpy(tempbuf,row_data+col_offset,col_len);
                            int64_t my_value = *((int8_t *)tempbuf);
                            workBuffer->table_data[col_name].intvec.push_back(my_value);
                            break;
                        }case MySQL_DataType::MySQL_INT16:{
                            char tempbuf[col_len];
                            memcpy(tempbuf,row_data+col_offset,col_len);
                            int64_t my_value = *((int16_t *)tempbuf);
                            workBuffer->table_data[col_name].intvec.push_back(my_value);     
                            break;
                        }case MySQL_DataType::MySQL_INT32:{
                            char tempbuf[col_len];
                            memcpy(tempbuf,row_data+col_offset,col_len);
                            int64_t my_value = *((int32_t *)tempbuf);
                            workBuffer->table_data[col_name].intvec.push_back(my_value);
                            break;
                        }case MySQL_DataType::MySQL_INT64:{
                            char tempbuf[col_len];
                            memcpy(tempbuf,row_data+col_offset,col_len);
                            int64_t my_value = *((int64_t *)tempbuf);
                            workBuffer->table_data[col_name].intvec.push_back(my_value);
                            break;
                        }case MySQL_DataType::MySQL_FLOAT32:{
                            //아직 처리X
                            char tempbuf[col_len];//col_len = 4
                            memcpy(tempbuf,row_data+col_offset,col_len);
                            double my_value = *((float *)tempbuf);
                            workBuffer->table_data[col_name].floatvec.push_back(my_value);
                            break;
                        }case MySQL_DataType::MySQL_DOUBLE:{
                            //아직 처리X
                            char tempbuf[col_len];//col_len = 8
                            memcpy(tempbuf,row_data+col_offset,col_len);
                            double my_value = *((double *)tempbuf);
                            workBuffer->table_data[col_name].floatvec.push_back(my_value);
                            break;
                        }case MySQL_DataType::MySQL_NEWDECIMAL:{
                            //decimal(15,2)만 고려한 상황 -> col_len = 7 or 8 (integer:6/real:1 or 2 or 3)
                            char tempbuf[col_len];//col_len = 7 or 8 or 9
                            memcpy(tempbuf,row_data+col_offset,col_len);
                            bool is_negative = false;
                            if(std::bitset<8>(tempbuf[0])[7] == 0){//음수일때 not +1
                                is_negative = true;
                                for(int i = 0; i<7; i++){
                                    tempbuf[i] = ~tempbuf[i];//not연산
                                }
                                // tempbuf[6] = tempbuf[6] +1;//+1
                            }   
                            char integer[8];
                            memset(&integer, 0, 8);
                            for(int k=0; k<5; k++){
                                integer[k] = tempbuf[5-k];
                            }

                            int64_t ivalue = *((int64_t *)integer); 
                            double rvalue;
                            if(col_len == 7){
                                char real[1];
                                real[0] = tempbuf[6];
                                rvalue = *((int8_t *)real); 
                                rvalue *= 0.01;
                            }else if(col_len == 8){
                                char real[2];
                                real[0] = tempbuf[7];
                                real[1] = tempbuf[6];
                                rvalue = *((int16_t *)real); 
                                rvalue *= 0.0001;
                            }else if(col_len == 9){
                                char real[4];
                                real[0] = tempbuf[8];
                                real[1] = tempbuf[7];
                                real[2] = tempbuf[6];
                                real[3] = 0x00;
                                rvalue = *((int32_t *)real); 
                                rvalue *= 0.000001;
                            }else{
                                KETILOG::FATALLOG("","Mysql_newdecimal>else : " + to_string(col_len));
                            }
                            double my_value = ivalue + rvalue;
                            if(is_negative){
                                my_value *= -1;
                            }
                            workBuffer->table_data[col_name].floatvec.push_back(my_value);
                            break;
                        }case MySQL_DataType::MySQL_DATE:{
                            char tempbuf[col_len+1];
                            memcpy(tempbuf,row_data+col_offset,col_len);
                            tempbuf[3] = 0x00;
                            int64_t my_value = *((int *)tempbuf);
                            workBuffer->table_data[col_name].intvec.push_back(my_value);
                            break;
                        }case MySQL_DataType::MySQL_TIMESTAMP:{
                            //아직 처리X
                            char tempbuf[col_len];
                            memcpy(tempbuf,row_data+col_offset,col_len);
                            int my_value = *((int *)tempbuf);
                            workBuffer->table_data[col_name].intvec.push_back(my_value);
                            break;
                        }case MySQL_DataType::MySQL_STRING:{
                            char tempbuf[col_len+1];
                            memcpy(tempbuf,row_data+col_offset,col_len);
                            tempbuf[col_len] = '\0';
                            string my_value(tempbuf);
                            workBuffer->table_data[col_name].strvec.push_back(my_value);
                            break;
                        }case MySQL_DataType::MySQL_VARSTRING:{
                            char tempbuf[col_len];
                            if(col_len < 258){//0~257 (_1_,___256____)
                                memcpy(tempbuf,row_data+col_offset+1,col_len-1);
                                tempbuf[col_len-1] = '\0';
                            }else{//259~65535 (__2__,_____65535______)
                                memcpy(tempbuf,row_data+col_offset+2,col_len-2);
                                tempbuf[col_len-2] = '\0';
                            }
                            string my_value(tempbuf);
                            workBuffer->table_data[col_name].strvec.push_back(my_value);
                            break;
                        }default:{
                            // string msg = " error>> Type: " + to_string(col_type) + " is not defined!";
                            // KETILOG::FATALLOG(LOGTAG, msg);
                        }
                    }
                    workBuffer->table_data[col_name].isnull.push_back(false);
                    workBuffer->table_data[col_name].row_count++;
                }
            }
        }

        workBuffer->merged_block_count += result.result_block_count;
        workBuffer->row_count += result.row_count;
        DataBuffer_[qid]->scanned_row_count += result.scanned_row_count;
        DataBuffer_[qid]->filtered_row_count += result.filtered_row_count;
        workBuffer->work_in_progress_condition.notify_all();
        
        KETILOG::DEBUGLOG(LOGTAG,"# save data {" + to_string(qid) + "|" + to_string(wid) + "|" + workBuffer->table_alias + "} ... ( " + std::to_string(workBuffer->merged_block_count) + " / " + std::to_string(workBuffer->table_total_block_count) + ")");

        if(workBuffer->merged_block_count == workBuffer->table_total_block_count){ //Work Done
            string msg = "# merging data {" + to_string(qid) + "|" + to_string(wid) + "|" + workBuffer->table_alias + "} done";
            KETILOG::INFOLOG(LOGTAG,msg);

            // string message = ">Save Result in Buffer ID:" + to_string(qid) + "-" + to_string(wid) + " Complete (lines:" + to_string(workBuffer->row_count) + ")";
            // logToFile(message);

            workBuffer->status = WorkDone;
            workBuffer->work_in_progress_condition.notify_all();
            workBuffer->work_done_condition.notify_all();

            break;
        }
    }
}

void BufferManager::initializeBuffer(int qid, int wid, string table_name){
    unique_lock<mutex> lock(buffer_mutex_);

    if(DataBuffer_.find(qid) == DataBuffer_.end()){
        QueryBuffer* queryBuffer = new QueryBuffer(qid);
        DataBuffer_.insert(pair<int,QueryBuffer*>(qid,queryBuffer));
    } 

    if(wid == -1){
        return;
    }
    
    if(DataBuffer_[qid]->work_buffer_list.find(wid) == DataBuffer_[qid]->work_buffer_list.end()){
        WorkBuffer* workBuffer = new WorkBuffer();
        DataBuffer_[qid]->tablename_workid_map[table_name] = wid;
        DataBuffer_[qid]->work_buffer_list[wid] = workBuffer;
    }
}

int BufferManager::endQuery(StorageEngineInstance::Request qid){
    auto it = DataBuffer_.find(qid.query_id());
    if (it != DataBuffer_.end()) {
        delete it->second;     
        DataBuffer_.erase(it);
    }
    return 1;
}

int BufferManager::checkTableStatus(int qid, int wid, string table_name){
    WorkBuffer* workBuffer = DataBuffer_[qid]->work_buffer_list[wid];
    unique_lock<mutex> lock(workBuffer->mu);
    return workBuffer->status;
}

TableData BufferManager::getTableData(int qid, int wid, string table_name, int row_index){
    initializeBuffer(qid, wid, table_name);

    if(wid == -1){
        while(true){
            if(DataBuffer_[qid]->tablename_workid_map.find(table_name) == DataBuffer_[qid]->tablename_workid_map.end()){
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }else{
                wid = DataBuffer_[qid]->tablename_workid_map[table_name];
                break;
            }
        }
    }

    TableData tableData;

    WorkBuffer* workBuffer = DataBuffer_[qid]->work_buffer_list[wid];
    unique_lock<mutex> lock(workBuffer->mu);

    while(workBuffer->row_count - row_index < 1000000 && workBuffer->status != WorkDone){
        workBuffer->work_in_progress_condition.wait(lock);
    }

    tableData.table_data = workBuffer->table_data;
    tableData.row_count = workBuffer->row_count;
    tableData.status = workBuffer->status;

    if(KETILOG::IsLogLevelUnder(TRACE)){// Debug Code 
        cout << "<get table data>" << endl;
        for(auto i : workBuffer->table_data){
            cout << i.first << "|" << i.second.row_count << "|" << i.second.type << endl;
        }
    }
    if(workBuffer->table_data.size() == 0){
        cout << "what ???" << endl;
    }
    
    return tableData;
}

TableData BufferManager::getFinishedTableData(int qid, int wid, string table_name){ 
    initializeBuffer(qid, wid, table_name);

    if(wid == -1){
        while(true){
            if(DataBuffer_[qid]->tablename_workid_map.find(table_name) == DataBuffer_[qid]->tablename_workid_map.end()){
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }else{
                wid = DataBuffer_[qid]->tablename_workid_map[table_name];
                break;
            }
        }
    }

    TableData tableData;

    WorkBuffer* workBuffer = DataBuffer_[qid]->work_buffer_list[wid];
    unique_lock<mutex> lock(workBuffer->mu);

    if(workBuffer->status == NotFinished || workBuffer->status == Initialized){
        KETILOG::DEBUGLOG(LOGTAG,"# not finished " + to_string(qid) + ":" + table_name);
        workBuffer->work_done_condition.wait(lock);
    }else if(workBuffer->status == WorkDone){
        KETILOG::DEBUGLOG(LOGTAG,"# done " + to_string(qid) + ":" + table_name);
    }

    tableData.table_data = workBuffer->table_data;
    tableData.row_count = workBuffer->row_count;
    tableData.status = workBuffer->status;
    tableData.scanned_row_count = DataBuffer_[qid]->scanned_row_count;
    tableData.filtered_row_count = DataBuffer_[qid]->filtered_row_count;

    if(KETILOG::IsLogLevelUnder(TRACE)){// Debug Code 
        cout << "<get finished table data>" << endl;
        for(auto i : workBuffer->table_data){
            cout << i.first << "|" << i.second.row_count << "|" << i.second.type << endl;
        }
    }
    if(workBuffer->table_data.size() == 0){
        cout << "what ???" << endl;
    }

    return tableData;
}

int BufferManager::saveTableData(SnippetRequest snippet, TableData &table_data_, int offset, int length){
    int qid = snippet.query_id();
    int wid = snippet.work_id();
    string table_name = snippet.result_info().table_alias();

    string msg = "# save table {" + to_string(qid) + "|" + table_name + "}";
    KETILOG::DEBUGLOG(LOGTAG,msg);

    WorkBuffer* workBuffer = DataBuffer_[qid]->work_buffer_list[wid];
    unique_lock<mutex> lock(workBuffer->mu);

    workBuffer->table_alias = table_name;
    for(int i = 0; i < snippet.result_info().column_alias_size(); i++){
        workBuffer->table_column.push_back(snippet.result_info().column_alias(i));
        workBuffer->table_data.insert({snippet.result_info().column_alias(i),ColData{}});
    }

    DataBuffer_[qid]->tablename_workid_map[table_name] = wid;
    DataBuffer_[qid]->work_buffer_list[wid] = workBuffer;
    
    if(length <= table_data_.row_count){
        for(auto &coldata: table_data_.table_data){
            ColData column;
            if(coldata.second.type == TYPE_STRING){
                column.strvec.assign(coldata.second.strvec.begin()+offset, coldata.second.strvec.begin()+length); 
                column.isnull.assign(coldata.second.isnull.begin()+offset, coldata.second.isnull.begin()+length);
                column.row_count = length - offset;
            }else if(coldata.second.type == TYPE_INT){
                column.intvec.assign(coldata.second.intvec.begin()+offset, coldata.second.intvec.begin()+length); 
                column.isnull.assign(coldata.second.isnull.begin()+offset, coldata.second.isnull.begin()+length);
                column.row_count = length - offset;
            }else if(coldata.second.type == TYPE_FLOAT){
                column.floatvec.assign(coldata.second.floatvec.begin()+offset, coldata.second.floatvec.begin()+length); 
                column.isnull.assign(coldata.second.isnull.begin()+offset, coldata.second.isnull.begin()+length);
                column.row_count = length - offset;
            }else if(coldata.second.type == TYPE_EMPTY){
            }else{
                KETILOG::FATALLOG(LOGTAG,"save table row type check plz... ");
            }
            column.type = coldata.second.type;
            workBuffer->table_data[coldata.first] = column;
        }
        workBuffer->row_count = length - offset;
    }else{
        DataBuffer_[qid]->work_buffer_list[wid]->table_data = table_data_.table_data;
        DataBuffer_[qid]->work_buffer_list[wid]->row_count = table_data_.row_count;
    }

    DataBuffer_[qid]->work_buffer_list[wid]->status = WorkDone;
    DataBuffer_[qid]->work_buffer_list[wid]->work_done_condition.notify_all();
    DataBuffer_[qid]->work_buffer_list[wid]->work_in_progress_condition.notify_all();
    
    return 1;
}

void BufferManager::t_initialize_buffer(int id){
    unique_lock<mutex> lock(t_buffer_mutex_);

    if(TmaxDataBuffer_.find(id) == TmaxDataBuffer_.end()){
        TmaxQueryBuffer tmax_query_buffer;
        TmaxDataBuffer_[id];
    }
}

void BufferManager::t_result_merging(std::string json, u_char* data, size_t data_size) {
    KETILOG::DEBUGLOG(LOGTAG, "<T> called t_result_merging, data_size : " + to_string(data_size));

    int id, chunk_count;
    t_initialize_buffer(id);

    rapidjson::Document document;
    if (document.Parse(json.c_str()).HasParseError()) {
        KETILOG::ERRORLOG(LOGTAG, "Error: Failed to parse JSON.");
        return;
    }

    id = document["id"].GetInt();
    chunk_count = document["chunk_count"].GetInt();

    unique_lock<mutex> lock(TmaxDataBuffer_[id].mu);

    ChunkBuffer chunk_buffer;
    chunk_buffer.chunk_count = chunk_count;
    std::string data_str(reinterpret_cast<char*>(data), data_size);
    chunk_buffer.result = data_str;

    TmaxDataBuffer_[id].chunk_buffer.push_back(chunk_buffer);
    TmaxDataBuffer_[id].available.notify_all();
}

ChunkBuffer BufferManager::t_get_result(int id){
    KETILOG::DEBUGLOG(LOGTAG, "<T> called t_get_result");

    t_initialize_buffer(id);

    unique_lock<mutex> lock(TmaxDataBuffer_[id].mu);

    if(TmaxDataBuffer_[id].chunk_buffer.size() == 0){
        TmaxDataBuffer_[id].available.wait(lock);
    }

    ChunkBuffer last_chunk = TmaxDataBuffer_[id].chunk_buffer.back();
    TmaxDataBuffer_[id].chunk_buffer.pop_back();
    
    return last_chunk;
}

void getColOffset(const char* row_data, int* col_offset_list, vector<int> return_datatype, vector<int> table_offlen){
    int col_type = 0, col_len = 0, col_offset = 0, new_col_offset = 0, tune = 0;
    int col_count = return_datatype.size();

    for(int i=0; i<col_count; i++){
        col_type = return_datatype[i];
        col_len = table_offlen[i];
        new_col_offset = col_offset + tune;
        col_offset += col_len;
        if(col_type == MySQL_VARSTRING){
            if(col_len < 256){//0~255 -> 길이표현 1자리
                char var_len[1];
                var_len[0] = row_data[new_col_offset];
                uint8_t var_len_ = *((uint8_t *)var_len);
                tune += var_len_ + 1 - col_len;
            }else{//0~65535 -> 길이표현 2자리
                char var_len[2];
                var_len[0] = row_data[new_col_offset];
                int new_col_offset_ = new_col_offset + 1;
                var_len[1] = row_data[new_col_offset_];
                uint16_t var_len_ = *((uint16_t *)var_len);
                tune += var_len_ + 2 - col_len;
            }
        }

        col_offset_list[i] = new_col_offset;
    }
}