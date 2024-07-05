#pragma once

#include <vector>
#include <unordered_map>
#include <mutex>
#include <fcntl.h>
#include <unistd.h>
#include <iomanip>
#include <algorithm>
#include <condition_variable>
#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h" 

#include <grpcpp/grpcpp.h>
#include "storage_engine_instance.grpc.pb.h"

#include "keti_log.h"
#include "ip_config.h"
#include "lba2pba_handler.h"

using namespace std;
using namespace rapidjson;

class TableManager { /* modify as singleton class */

public:	
	struct Chunk {
		off64_t offset;
		off64_t length;
	};

	struct LBAPBAMap {
		map<off64_t, off64_t> offset_pair; // first: lba offset, second: pba offset
	};

	struct TableBlock {
		map<string, Chunk> lba_block_list; // first: block handle, second: lba chunk
		map<string, LBAPBAMap> csd_pba_map;// key: csd id, value: lba pba map
	};

	struct SST {
		vector<string> csd_list;
		map<int, TableBlock> table_block_list;// key: table index number, value: block chunks
	};
	
	struct IndexTable {
		vector<string> sst_list;

		string table_index_number;
		map<string, string> index_table; //key: index, value: primary key 
		/*
			HEX 000002D18000000380000280: 
			...
			ex) 000002D1 80000003 80000280 = (4byte table_index_number + (4byte index + 4byte primary key))
		*/
	};

	struct Table {
		int table_index_number;
		int index_table_index_number;
		vector<string> sst_list; // table sst list
		vector<string> index_sst_list; // index table sst list
		map<string, IndexTable> index_tables; // key: index column name, value: IndexTable
	};

	struct DB {
		map<string, Table> table; // key: table name, value: struct Table
	};

	inline const static string LOGTAG = "Monitoring::Table Manager";

	static void InitTableManager(){
		return GetInstance().initTableManager();
	}

	static void DumpTableManager(){
		return GetInstance().dumpTableManager();
	}

	static bool CheckExistDB(string db_name){
		return GetInstance().checkExistDB(db_name);
	}

	static bool CheckExistTable(string db_name, string table_name){
		return GetInstance().checkExistTable(db_name, table_name);
	}

	SST GetSST(string sst_name){
		return GetInstance().getSST(sst_name);
	}

	static vector<string> GetTableSSTList(string db_name, string table_name){
		return GetInstance().getTableSSTList(db_name, table_name);
	}

	static map<string, Chunk> GetSSTTableLBAList(string sst_name, int table_index_number){
		return GetInstance().getSSTTableLBAList(sst_name, table_index_number);
	}

	static map<off64_t, off64_t> GetTableCSDPBAList(string sst_name, int table_index_number, string csd_name){
		return GetInstance().getTableCSDPBAList(sst_name, table_index_number, csd_name);
	}

	static int GetTableIndexNumber(string db_name, string table_name){
		return GetInstance().getTableIndexNumber(db_name, table_name);
	}

	static vector<Chunk> GetSSTPBABlocks(string sst_name, string csd_name, int table_index_number){
		return GetInstance().GetSSTPBABlocks(sst_name, csd_name, table_index_number);
	}

	static void SetTableInfo(string db_name, string table_name, Table table){
		return GetInstance().setTableInfo(db_name, table_name, table);
	}

	static void SetSSTInfo(string sst_name, SST sst){
		return GetInstance().setSSTInfo(sst_name, sst);
	}

	static void SetDBInfo(string db_name, DB db){
		return GetInstance().setDBInfo(db_name, db);
	}

	static void SetSSTPBAInfo(string sst_name, int table_index_number, string csd_id, LBAPBAMap lba_pba_map){
		return GetInstance().setSSTPBAInfo(sst_name, table_index_number, csd_id, lba_pba_map);
	}

	static void SetSSTTableBlockInfo(string sst_name, map<int, TableBlock> table_block_list){
		return GetInstance().setSSTTableBlockInfo(sst_name, table_block_list);
	}

	static string makeKey(int qid, int wid){
		string key = to_string(qid)+"|"+to_string(wid);
        return key;
    }

	static string convertInt2Byte(int integer_data){
		char byte_data[4];
        memcpy(byte_data, &integer_data, sizeof(int));
		string byte_string(byte_data);
		
		return byte_data;
    }

	static void UpdateSSTPBA(string sst_name){
		return GetInstance().updateSSTPBA(sst_name);
	}

	static void RequestTablePBA(StorageEngineInstance::MetaDataRequest metadata_request, int &total_block_count, map<string,StorageEngineInstance::SnippetMetaData_PBAInfo> &sst_pba_map){
		return GetInstance().requestTablePBA(metadata_request, total_block_count, sst_pba_map);
	}

	static void TempRequestFilteredPBA(StorageEngineInstance::MetaDataRequest metadata_request, int &total_block_count, map<string,StorageEngineInstance::SnippetMetaData_PBAInfo> &sst_pba_map){
		return GetInstance().tempRequestFilteredPBA(metadata_request, total_block_count, sst_pba_map);
	}

	static vector<string> SeekIndexTable(string db_name, string table_name){
		return GetInstance().seekIndexTable(db_name, table_name);
	}

private:
	TableManager(){};
    TableManager(const TableManager&);
    TableManager& operator=(const TableManager&){
        return *this;
    };

    static TableManager& GetInstance() {
        static TableManager _instance;
        return _instance;
    }

	bool checkExistDB(string db_name){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().TableManager_.find(db_name) != GetInstance().TableManager_.end();
	}

	bool checkExistTable(string db_name, string table_name){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().TableManager_[db_name].table.find(table_name) != GetInstance().TableManager_[db_name].table.end();
	}

	SST getSST(string sst_name){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().SSTManager_[sst_name];
	}

	vector<string> getTableSSTList(string db_name, string table_name){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().TableManager_[db_name].table[table_name].sst_list;
	}

	map<string, Chunk> getSSTTableLBAList(string sst_name, int table_index_number){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().SSTManager_[sst_name].table_block_list[table_index_number].lba_block_list;
	}

	map<off64_t, off64_t> getTableCSDPBAList(string sst_name, int table_index_number, string csd_name){
		return GetInstance().SSTManager_[sst_name].table_block_list[table_index_number].csd_pba_map[csd_name].offset_pair;
	}

	int getTableIndexNumber(string db_name, string table_name){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().TableManager_[db_name].table[table_name].table_index_number;
	}

	void setTableInfo(string db_name, string table_name, Table table){
		std::lock_guard<std::mutex> lock(mutex_);
		TableManager_[db_name].table[table_name] = table;
	}

	void setSSTInfo(string sst_name, SST sst){
		std::lock_guard<std::mutex> lock(mutex_);
		SSTManager_[sst_name] = sst;
	}

	void setDBInfo(string db_name, DB db){
		std::lock_guard<std::mutex> lock(mutex_);
		TableManager_[db_name] = db;
	}

	void setSSTPBAInfo(string sst_name, int table_index_number, string csd_id, LBAPBAMap lba_pba_map){
		SSTManager_[sst_name].table_block_list[table_index_number].csd_pba_map[csd_id] = lba_pba_map;
	}

	void setSSTTableBlockInfo(string sst_name, map<int, TableBlock> table_block_list){
		std::lock_guard<std::mutex> lock(mutex_);
		SSTManager_[sst_name].table_block_list = table_block_list;
	}

	void initTableManager();
	void dumpTableManager();
	void updateSSTPBA(string sst_name);
	void updateSSTIndexTable(string sst_name);
	void requestTablePBA(StorageEngineInstance::MetaDataRequest metadata_request, int &total_block_count, map<string,StorageEngineInstance::SnippetMetaData_PBAInfo> &sst_pba_map);
	void tempRequestFilteredPBA(StorageEngineInstance::MetaDataRequest metadata_request, int &total_block_count, map<string,StorageEngineInstance::SnippetMetaData_PBAInfo> &sst_pba_map);
	vector<Chunk> getSSTPBABlocks(string sst_name, string csd_name, int table_index_number);
	vector<string> seekIndexTable(string db_name, string table_name);
    mutex mutex_;
	unordered_map<string, DB> TableManager_; // key: db name, value: struct DB
	unordered_map<string, SST> SSTManager_; //key: sst name, value: struct SST
};