#include "table_manager.h"

// 테이블 매니저 정보 임의 저장
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

    StorageEngineInstance::LBARequest lba_request;

	for(const auto sst : SSTManager_){
		string sst_name = sst.first;
		StorageEngineInstance::LBARequest_SST lba_request_sst;
		StorageEngineInstance::TableBlock table_block;

		for(const auto csd : sst.second.csd_pba_list){
			string csd_id = csd.first;
			lba_request_sst.add_csd_list(csd_id);
		}

		for(const auto block : sst.second.table_lba_list.table_block_list){
			int table_index_number = block.first;
			StorageEngineInstance::ChunkList chunk_list;
			for(int i=0; i<block.second.block_list.size(); i++){
				StorageEngineInstance::Chunk chunk;
				chunk.set_block_handle(block.second.block_list[i].first);
				chunk.set_offset(block.second.block_list[i].second.offset);
				chunk.set_length(block.second.block_list[i].second.length);
				chunk_list.add_chunks()->CopyFrom(chunk);
			}
			table_block.mutable_table_block_list()->insert({table_index_number, chunk_list});
		}

		lba_request_sst.mutable_table_lba_list()->CopyFrom(table_block);
		lba_request.mutable_sst_list()->insert({sst_name, lba_request_sst});
	}

    StorageManagerConnector smc(grpc::CreateChannel((string)STORAGE_CLUSTER_MASTER_IP+":"+(string)LBA2PBA_MANAGER_PORT, grpc::InsecureChannelCredentials()));
    smc.RequestPBA(lba_request);
	
	return 0;
}

void TableManager::updateSSTPBA(string sst_name){
	std::lock_guard<std::mutex> lock(mutex_);

	StorageEngineInstance::LBARequest lba_request;

	TableManager::SST sst = SSTManager_[sst_name];

	StorageEngineInstance::LBARequest_SST lba_request_sst;
	StorageEngineInstance::TableBlock table_block;

	for(const auto csd : sst.csd_pba_list){
		string csd_id = csd.first;
		lba_request_sst.add_csd_list(csd_id);
	}

	for(const auto block : sst.table_lba_list.table_block_list){
		int table_index_number = block.first;
		StorageEngineInstance::ChunkList chunk_list;
		for(int i=0; i<block.second.block_list.size(); i++){
			StorageEngineInstance::Chunk chunk;
			chunk.set_block_handle(block.second.block_list[i].first);
			chunk.set_offset(block.second.block_list[i].second.offset);
			chunk.set_length(block.second.block_list[i].second.length);
			chunk_list.add_chunks()->CopyFrom(chunk);
		}
		table_block.mutable_table_block_list()->insert({table_index_number, chunk_list});
	}

	lba_request_sst.mutable_table_lba_list()->CopyFrom(table_block);
	lba_request.mutable_sst_list()->insert({sst_name, lba_request_sst});
	
    StorageManagerConnector smc(grpc::CreateChannel((string)STORAGE_CLUSTER_MASTER_IP+":"+(string)LBA2PBA_MANAGER_PORT, grpc::InsecureChannelCredentials()));
    smc.RequestPBA(lba_request);
}


void TableManager::dumpTableManager(){
	cout << "-------------------------------------" << endl;
	cout << "# DB Info" << endl;
	int db_numbering = 1;
	for(const auto db : GetInstance().TableManager_){
		cout << db_numbering << ". db_name: " << db.first << endl;
		int table_numbering = 1;
		for(const auto table : db.second.table){
			cout << db_numbering << "-" << table_numbering << ". table_name: " << table.first << endl;
			cout << db_numbering << "-" << table_numbering << ". sst_list: ";
			for(const auto sst : table.second.sst_list){
				cout << sst << ", ";
			}
			cout << endl;
			table_numbering++;
		}
		db_numbering++;
	}
	cout << "-------------------------------------" << endl;
}

void TableManager::requestSSTPBA(StorageEngineInstance::MetaDataRequest metadata_request, int &total_block_count, map<string,string> &sst_pba_map){
	vector<string> sst_list = getTableSSTList(metadata_request.db_name(), metadata_request.table_name());
	int table_index_number = getTableIndexNumber(metadata_request.db_name(), metadata_request.table_name());

	for(int i=0; i<sst_list.size(); i++){
		
	}
}

void TableManager::updateSSTIndexTable(string sst_name){
	
}

// string sst_name = entry.first;
//             PBAResponse_SST csd_pba = entry.second;

//             //json 구성
//             StringBuffer buffer;
//             buffer.Clear();
//             Writer<StringBuffer> writer(buffer);
//             writer.StartObject();
//             writer.Key("blockList");
//             writer.StartArray();
//             writer.StartObject();
//             for(int l=0; l<csd_pba.chunks_size(); l++){
//                 if(l == 0){
//                     writer.Key("offset");
//                     writer.Int64(csd_pba.chunks(l).offset());
//                     writer.Key("length");
//                     writer.StartArray();
//                 }else{
//                     if(csd_pba.chunks(l-1).offset()+csd_pba.chunks(l-1).length() != csd_pba.chunks(l).offset()){
//                         writer.EndArray();
//                         writer.EndObject();
//                         writer.StartObject();
//                         writer.Key("offset");
//                         writer.Int64(csd_pba.chunks(l).offset());
//                         writer.Key("length");
//                         writer.StartArray();
//                     }
//                 }
//                 writer.Int(csd_pba.chunks(l).length());

//                 if(l == csd_pba.chunks_size() - 1){
//                     writer.EndArray();
//                     writer.EndObject();
//                 }
//             }
//             writer.EndArray();
//             writer.EndObject();

//             total_block_count += csd_pba.chunks_size();
//             string pba_string = buffer.GetString();
//             sst_pba_map[sst_name] = pba_string;
//         }