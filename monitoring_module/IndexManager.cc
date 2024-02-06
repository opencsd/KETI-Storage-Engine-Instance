#include "IndexManager.h"

using namespace rapidjson;

//테이블 스키마 정보 임의로 저장 >> 이후 DB File Monitoring을 통해 데이터 저장 -> 업데이트시 DB Connector Instance로 반영
void IndexManager::initIndexManager(){
	// 00 00 01 3E 80 00 2A 20 80 00 00 01
    char data0[12];
    data0[0] = 0x00;
    data0[1] = 0x00;
    data0[2] = 0x01;
    data0[3] = 0x3E;
    data0[4] = 0x80;
    data0[5] = 0x00;
    data0[6] = 0x2A;
    data0[7] = 0x20;
    data0[8] = 0x80;
    data0[9] = 0x00;
    data0[10] = 0x00;
    data0[11] = 0x01;
    // 00 00 01 3E 80 00 2A 20 80 00 00 02  
    char data1[12];               
    data1[0] = 0x00;
    data1[1] = 0x00;
    data1[2] = 0x01;
    data1[3] = 0x3E;
    data1[4] = 0x80;
    data1[5] = 0x00;
    data1[6] = 0x2A;
    data1[7] = 0x20;
    data1[8] = 0x80;
    data1[9] = 0x00;
    data1[10] = 0x00;
    data1[11] = 0x02;               
    // 00 00 01 3E 80 00 2A 20 80 00 00 03  
    char data2[12];               
    data2[0] = 0x00;
    data2[1] = 0x00;
    data2[2] = 0x01;
    data2[3] = 0x3E;
    data2[4] = 0x80;
    data2[5] = 0x00;
    data2[6] = 0x2A;
    data2[7] = 0x20;
    data2[8] = 0x80;
    data2[9] = 0x00;
    data2[10] = 0x00;
    data2[11] = 0x03;               
    // 00 00 01 3E 80 00 2A 20 80 00 00 04     
    char data3[12];            
    data3[0] = 0x00;
    data3[1] = 0x00;
    data3[2] = 0x01;
    data3[3] = 0x3E;
    data3[4] = 0x80;
    data3[5] = 0x00;
    data3[6] = 0x2A;
    data3[7] = 0x20;
    data3[8] = 0x80;
    data3[9] = 0x00;
    data3[10] = 0x00;
    data3[11] = 0x04;               
    // 00 00 01 3E 80 00 2A 20 80 00 00 05        
    char data4[12];         
    data4[0] = 0x00;
    data4[1] = 0x00;
    data4[2] = 0x01;
    data4[3] = 0x3E;
    data4[4] = 0x80;
    data4[5] = 0x00;
    data4[6] = 0x2A;
    data4[7] = 0x20;
    data4[8] = 0x80;
    data4[9] = 0x00;
    data4[10] = 0x00;
    data4[11] = 0x05;                
    // 00 00 01 3E 80 00 2A 20 80 00 00 06        
    char data5[12];                  
    data5[0] = 0x00;
    data5[1] = 0x00;
    data5[2] = 0x01;
    data5[3] = 0x3E;
    data5[4] = 0x80;
    data5[5] = 0x00;
    data5[6] = 0x2A;
    data5[7] = 0x20;
    data5[8] = 0x80;
    data5[9] = 0x00;
    data5[10] = 0x00;
    data5[11] = 0x06;                   

    // [2] index table 정보 저장
    // key : index, value : pk list

	TableManager::Table table = TableManager::GetMutableTable("lineitem");
	for(int i=0; i<table.SSTList.size(); i++){
		TableManager::SSTFile sst = table.SSTList[i];
		
		char* ikey0 = &data0[0];
		string index0(ikey0 + 4, ikey0 + 8);
		string pk0(ikey0 + 4, ikey0 + 12);
		sst.IndexTable[index0].push_back(pk0);

		char* ikey1 = &data1[0];
		string index1(ikey1 + 4, ikey1 + 8);
		string pk1(ikey1 + 4, ikey1 + 12);
		sst.IndexTable[index1].push_back(pk1);

		char* ikey2 = &data2[0];
		string index2(ikey2 + 4, ikey2 + 8);
		string pk2(ikey2 + 4, ikey2 + 12);
		sst.IndexTable[index2].push_back(pk2);

		char* ikey3 = &data3[0];
		string index3(ikey3 + 4, ikey3 + 8);
		string pk3(ikey3 + 4, ikey3 + 12);
		sst.IndexTable[index3].push_back(pk3);

		char* ikey4 = &data4[0];
		string index4(ikey4 + 4, ikey4 + 8);
		string pk4(ikey4 + 4, ikey4 + 12);
		sst.IndexTable[index4].push_back(pk4);

		char* ikey5 = &data5[0];
		string index5(ikey5 + 4, ikey5 + 8);
		string pk5(ikey5 + 4, ikey5 + 12);
		sst.IndexTable[index5].push_back(pk5);
		
	}
    
	std::thread IndexManager_Thread(&IndexManager::requestIndexScan,this);
    IndexManager_Thread.detach();
}

void IndexManager::requestIndexScan(){
    // while(1){
    //     Request request = popQueue();

    //     int qid = request.query_id();
    //     int wid = request.work_id();
    //     string tname = request.table_name();

	// 	string key = TableManager::makeKey(qid,wid);
    //     Response* data = TableManager::GetReturnData(key);
    //     unique_lock<mutex> lock(data->mu);
	// 	TableManager::Table table = TableManager::GetMutableTable(tname);

	// 	for(int i=0; i<table.SSTList.size(); i++){
	// 		MetaDataResponse_PBAInfo pba_info;
    //         // string file_name = pbaResponse.file_csd_list(i).file_name();

	// 		int find_index = 10784; // index절의 조건절
	// 		int table_index_num = 380; // 임시저장

	// 		char temp[4];
	// 		memcpy(temp, &find_index, sizeof(int));

	// 		char find_index_char[4];
	// 		for(int i=0; i<4; i++){
	// 			find_index_char[i] = temp[3-i];
	// 		}
	// 		find_index_char[0] ^= 0x80;

	// 		string find_index_string(find_index_char,find_index_char + 4);
			
	// 		// [4] index_table에서 index에 해당하는 pk 획득
	// 		// 획득한 pk를 table_index_number와 조합해 vector<string>에 저장

	// 		vector<string> seek_pk_list;
			
	// 		memcpy(temp, &table_index_num, sizeof(int));
	// 		char table_index_num_char[4];
	// 		for(int i=0; i<4; i++){
	// 			table_index_num_char[i] = temp[3-i];
	// 		}

	// 		int PK_SIZE = 8;//pk 사이즈 사전 계산 필요
	// 		for(int i=0; i<table.SSTList[i].IndexTable[find_index_string].size(); i++){
	// 			char seek_pk_char[4 + PK_SIZE];// table_index_num_size + pk_size
	// 			memcpy(seek_pk_char, table_index_num_char, 4);
	// 			memcpy(seek_pk_char + 4, table.SSTList[i].IndexTable[find_index_string][i].c_str(), PK_SIZE);

	// 			cout << "seek_pk_char: ";
	// 			for(int i=0; i<12; i++){
	// 				printf("%02X ",(u_char)seek_pk_char[i]);
	// 			}
	// 			cout << endl;

	// 			string seek_pk_string(seek_pk_char, seek_pk_char + 4 + PK_SIZE);
	// 			seek_pk_list.push_back(seek_pk_string);
	// 		}

	// 		// data->metadataResponse->mutable_sst_csd_map()->insert({file_name,pba_info});
	// 	}

	// }
}