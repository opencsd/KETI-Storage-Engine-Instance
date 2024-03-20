#include "table_manager.h"

//테이블 스키마 정보 임의로 저장 >> 이후 DB File Monitoring을 통해 데이터 저장 -> 업데이트시 DB Connector Instance로 반영
int TableManager::initTableManager(){
	KETILOG::DEBUGLOG(LOGTAG, "# Init TableManager");

	//read TableManager.json
	string json = "";
	std::ifstream openFile("../table_manager_data/TableManager_tpch_origin.json");
	if(openFile.is_open() ){
		std::string line;
		while(getline(openFile, line)){
			json += line;
		}
		openFile.close();
	}
	
	//parse json	
	Document document;
	document.Parse(json.c_str());

	Value &TableList = document["Table List"];
	
	for(int i=0;i<TableList.Size();i++){
		Value &TableObject = TableList[i];
		auto tbl = new Table();

		Value &tablenameObject = TableObject["tablename"];
		tbl->tablename = tablenameObject.GetString();

		// tbl.tablesize = TableObject["Size"].GetInt();

		// if(TableObject.HasMember("PK")){
		// 	for(int i = 0; i < TableObject["PK"].Size(); i++){
		// 		tbl.PK.push_back(TableObject["PK"][i].GetString());
		// 	}
		// }
		// if(TableObject.HasMember("Index")){
		// 	for(int i = 0; i < TableObject["Index"].Size(); i++){
		// 		vector<string> index;
		// 		for(int j = 0; j < TableObject["Index"][i].Size(); j++){
		// 			index.push_back(TableObject["Index"][i][j].GetString());
		// 		}
		// 		tbl.IndexList.push_back(index);
		// 	}
		// }
		
		Value &SchemaObject = TableObject["Schema"];
		for(int j=0;j<SchemaObject.Size();j++){
			Value &ColumnObject = SchemaObject[j];
			auto Column = new ColumnSchema;

			Column->column_name = ColumnObject["column_name"].GetString();
			Column->type = ColumnObject["type"].GetInt();
			Column->length = ColumnObject["length"].GetInt();
			Column->offset = ColumnObject["offset"].GetInt();

			tbl->Schema.push_back(*Column);
		}

		Value &SSTList = TableObject["SST List"];
		for(int j=0;j<SSTList.Size();j++){
			Value &SSTObject = SSTList[j];
			auto SstFile = new SSTFile;

			SstFile->filename = SSTObject["filename"].GetString();

			Value &BlockList = SSTObject["Block List"];
			
			for(int k=0;k<BlockList.Size();k++){
				Value &BlockHandleObject = BlockList[k];
				struct DataBlockHandle DataBlockHandle;

				// if(BlockHandleObject.HasMember("IndexBlockHandle")){
				// 	DataBlockHandle.IndexBlockHandle = BlockHandleObject["IndexBlockHandle"].GetString();
				// }

				DataBlockHandle.Offset = BlockHandleObject["Offset"].GetInt();
				DataBlockHandle.Length = BlockHandleObject["Length"].GetInt();

				SstFile->BlockList.push_back(DataBlockHandle);
			}

			tbl->SSTList.push_back(*SstFile);
		}
		m_TableManager.insert({tbl->tablename,*tbl});
	}

    
    StorageManagerConnector smc(grpc::CreateChannel((string)STORAGE_CLUSTER_MASTER_IP+":"+(string)LBA2PBA_MANAGER_PORT, grpc::InsecureChannelCredentials()));
    smc.RequestPBA(); //async로 구현할것!


	//인덱스테이블저장
	
	return 0;
}

void TableManager::getPBA(ScanInfo scan_info, int &total_block_count, map<string,string> &sst_pba_map){

}