#include "table_manager.h"

// 테이블 매니저 정보 임시 하드코딩 데이터 저장
void TableManager::initTableManager(){
	KETILOG::DEBUGLOG(LOGTAG, "# Init TableManager");

	// Init TableManager_ Data
	string json = "";
	std::ifstream openFile("../table_manager_data/table_manager.init.json");
	if(openFile.is_open() ){
		std::string line;
		while(getline(openFile, line)){
			json += line;
		}
		openFile.close();
	}
	
	Document document;
	document.Parse(json.c_str());

	Value &dbList = document["dbList"];
	for(int i=0;i<dbList.Size();i++){
		Value &dbObject = dbList[i];

		TableManager::DB db;
		string db_name = dbObject["dbName"].GetString();

		Value &tableList = dbObject["tableList"];
		for(int j=0;j<tableList.Size();j++){
			Value &tableObject = tableList[j];

			string table_name = tableObject["tableName"].GetString();

			TableManager::Table table;
			table.table_index_number = tableObject["tableIndexNumber"].GetInt64();

			Value &sstList = tableObject["sstList"];
			for(int l=0; l<sstList.Size(); l++){
				table.sst_list.push_back(sstList[l].GetString());
			}

			db.table[table_name] = table;
		}

		TableManager::SetDBInfo(db_name, db);
	}

	// Init SSTManager_ Data
	json = "";
	std::ifstream openFile2("../table_manager_data/sst_manager.init.json");
	if(openFile2.is_open() ){
		std::string line;
		while(getline(openFile2, line)){
			json += line;
		}
		openFile2.close();
	}
	
	document.Parse(json.c_str());

	Value &sstList = document["sstList"];
	for(int i=0;i<sstList.Size();i++){
		Value &sstObject = sstList[i];
		
		TableManager::SST sst;
		string sst_name = sstObject["sstName"].GetString();

		Value &tableBlockList = sstObject["tableBlockList"];
		for(int j=0;j<tableBlockList.Size();j++){
			Value &tableBlockObject = tableBlockList[j];

			TableManager::TableBlock tableBlock;

			int table_index_number = tableBlockObject["tableIndexNumber"].GetInt64();

			int temp_block_handle = 0;

			Value &blockList = tableBlockObject["blockList"];
			for(int l=0; l<blockList.Size(); l++){
				string block_handle;
				Value &blockObject = blockList[l];

				TableManager::Chunk chunk;
				chunk.offset = blockObject["offset"].GetInt64();
				chunk.length = blockObject["length"].GetInt64();

				if(blockObject.HasMember("blockHandle")){
					block_handle = blockObject["blockHandle"].GetString();
				}else{
					std::stringstream ss;
					ss << std::setw(5) << std::setfill('0') << temp_block_handle;
					block_handle = ss.str();
					temp_block_handle++;
				}

				tableBlock.lba_block_list[block_handle] = chunk;
			}

			sst.table_block_list[table_index_number] = tableBlock;
		}

		Value &csdList = sstObject["csdList"];
		for(int j=0;j<csdList.Size();j++){
			sst.csd_list.push_back(csdList[j].GetString());
		}

		TableManager::SetSSTInfo(sst_name, sst);
	}

	// Get PBA Data
	for(const auto sst : SSTManager_){
		string sst_name = sst.first;
		updateSSTPBA(sst_name);
	}

	return;
}

void TableManager::updateSSTPBA(string sst_name){
	StorageEngineInstance::LBARequest lba_request;

	TableManager::SST sst = TableManager::GetSST(sst_name);

	StorageEngineInstance::LBARequest_SST lba_request_sst;
	StorageEngineInstance::TableBlock table_lba_block;
	
	for(int i=0; i<sst.csd_list.size(); i++){
		lba_request_sst.add_csd_list(sst.csd_list[i]);
	}

	for(const auto table_block : sst.table_block_list){
		int table_index_number = table_block.first;
		StorageEngineInstance::Chunks chunks;
		
		for(const auto lba_block : table_block.second.lba_block_list){
			StorageEngineInstance::Chunk chunk;
			chunk.set_offset(lba_block.second.offset);
			chunk.set_length(lba_block.second.length);
			chunks.add_chunks()->CopyFrom(chunk);
		}

		table_lba_block.mutable_table_block_chunks()->insert({table_index_number, chunks});
	}

	lba_request_sst.mutable_table_lba_block()->CopyFrom(table_lba_block);
	lba_request.mutable_sst_list()->insert({sst_name, lba_request_sst});
	
    StorageManagerConnector smc(grpc::CreateChannel((string)STORAGE_CLUSTER_MASTER_IP+":"+(string)LBA2PBA_MANAGER_PORT, grpc::InsecureChannelCredentials()));
    StorageEngineInstance::PBAResponse pbaResponse = smc.RequestPBA(lba_request);

	for (const auto res_sst : pbaResponse.sst_list()) {
		string sst_name = res_sst.first;

		for(const auto table_block : res_sst.second.table_pba_block()){
			string csd_id = table_block.first;
			
			for(const auto chunks : table_block.second.table_block_chunks()){
				int table_index_number = chunks.first;
				TableManager::LBAPBAMap lba_pba_map;
				map<off64_t, off64_t> offset_pair;

				map<string, TableManager::Chunk> lba_block_list = TableManager::GetSSTTableLBAList(sst_name, table_index_number);
				
				int index = 0;
				for(const auto lba_block : lba_block_list){
					off64_t lba_offset = lba_block.second.offset;
					off64_t pba_offset = chunks.second.chunks(index).offset();
					index++;
					offset_pair.insert({lba_offset, pba_offset});
				}
				
				lba_pba_map.offset_pair = offset_pair;
				TableManager::SetSSTPBAInfo(sst_name, table_index_number, csd_id, lba_pba_map);
			}
		}
	}
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
				cout << sst << " ";
				cout << "(" << GetInstance().SSTManager_[sst].csd_list[0] << "),";
			}
			cout << endl;
			table_numbering++;  
		}
		db_numbering++;
	}

	cout << "# SST Info" << endl;
	int sst_numbering = 1;
	for(const auto sst : GetInstance().SSTManager_){
		cout << sst_numbering << ". sst_name: " << sst.first << endl;
		for(const auto table_block_list : sst.second.table_block_list){
			cout << "- table index number: " << table_block_list.first << endl;
			for(const auto csd_pba : table_block_list.second.csd_pba_map){
				cout << "- csd id: " << csd_pba.first << endl;
				cout << "- block size: " << csd_pba.second.offset_pair.size() << endl;
			}
		}
	}
	cout << "-------------------------------------" << endl;
}

void TableManager::requestTablePBA(StorageEngineInstance::MetaDataRequest metadata_request, int &total_block_count, map<string,string> &sst_pba_map){
	int table_index_number = getTableIndexNumber(/*metadata_request.db_name()*/"tpch_origin", metadata_request.table_name());

	for(const auto sst_csd_map : metadata_request.scan_info().sst_csd_map()){
		string sst_name = sst_csd_map.sst_name();
		string csd_name = sst_csd_map.csd_list(0);

		vector<TableManager::Chunk> block_list = getSSTFilteredPBABlocks(metadata_request.scan_info().filter_info(), sst_name, csd_name, table_index_number);

		StringBuffer buffer;
		buffer.Clear();
		Writer<StringBuffer> writer(buffer);
		writer.StartObject();
		writer.Key("blockList");
		writer.StartArray();
		writer.StartObject();
		for(int l=0; l<block_list.size(); l++){
			if(l == 0){
				writer.Key("offset");
				writer.Int64(block_list[l].offset);
				writer.Key("length");
				writer.StartArray();
			}else{
				if(block_list[l-1].offset+block_list[l-1].length != block_list[l].offset){
					writer.EndArray();
					writer.EndObject();
					writer.StartObject();
					writer.Key("offset");
					writer.Int64(block_list[l].offset);
					writer.Key("length");
					writer.StartArray();
				}
			}
			writer.Int(block_list[l].length);

			if(l == block_list.size() - 1){
				writer.EndArray();
				writer.EndObject();
			}
		}
		writer.EndArray();
		writer.EndObject();

		total_block_count += block_list.size();
		string pba_string = buffer.GetString();
		sst_pba_map[sst_name] = pba_string;
	}
}

void TableManager::updateSSTIndexTable(string sst_name){
	// save index table from sst
}

vector<TableManager::Chunk> TableManager::getSSTFilteredPBABlocks(StorageEngineInstance::ScanInfo_BlockFilterInfo filter_info, string sst_name, string csd_name, int table_index_number){
	vector<TableManager::Chunk> return_chunks;

	map<off64_t, off64_t> origin_lba_pba_map =  TableManager::GetTableCSDPBAList(sst_name, table_index_number, csd_name);
	map<string, TableManager::Chunk> origin_lba_list = TableManager::GetSSTTableLBAList(sst_name, table_index_number);

	if(filter_info.lv() != ""){
		// block filtering here
	}else{
		for(auto lba : origin_lba_list){
			TableManager::Chunk chunk;
			chunk.offset = origin_lba_pba_map[lba.second.offset];
			chunk.length = lba.second.length;
			return_chunks.push_back(chunk);
		}
	}

	return return_chunks; 
}

vector<string> TableManager::seekIndexTable(StorageEngineInstance::ScanInfo_BlockFilterInfo filter_info, string db_name, string table_name){
	
}

