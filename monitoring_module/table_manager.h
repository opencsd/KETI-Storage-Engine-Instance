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
	struct Block {
		off64_t offset;
		off64_t length;
	};

	struct BlockList {
		// Use list to save sequentially
		vector<pair<string, Block>> block_list; // key: block index handle, value: block chunk
	};

	struct TableBlock {
		map<int, BlockList> table_block_list;
	};

	struct SST {
		map<string, TableBlock> csd_pba_list;// key: csd id, value: csd
		TableBlock table_lba_list;// key: table_index_number, value: table lba
	};
	
	struct IndexTable {
		string sst_name;
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
		vector<string> sst_list;
		vector<string> index_sst_list;
		map<string, IndexTable> index_tables; // key: index column name, value: IndexTable
	};

	struct DB {
		map<string, Table> table; // key: table name, value: struct Table
	};

	inline const static string LOGTAG = "Monitoring::Table Manager";

	static void DumpTableManager(){
		return GetInstance().dumpTableManager();
	}

	static bool CheckExistDB(string db_name){
		return GetInstance().checkExistDB(db_name);
	}

	static bool CheckExistTable(string db_name, string table_name){
		return GetInstance().checkExistTable(db_name, table_name);
	}

	static vector<string> GetTableSSTList(string db_name, string table_name){
		return GetInstance().getTableSSTList(db_name, table_name);
	}

	static int GetTableIndexNumber(string db_name, string table_name){
		return GetInstance().getTableIndexNumber(db_name, table_name);
	}

	static void SetTableInfo(string db_name, string table_name, Table table){
		return GetInstance().setTableInfo(db_name, table_name, table);
	}

	static void SetDBInfo(string db_name, DB db){
		return GetInstance().setDBInfo(db_name, db);
	}

	static void SetSSTPBAInfo(string sst_name, map<string, TableBlock> csd_pba_list){
		return GetInstance().setSSTPBAInfo(sst_name, csd_pba_list);
	}

	static string makeKey(int qid, int wid){
		string key = to_string(qid)+"|"+to_string(wid); // (ex:"s_suppkey","l_orderkey|l_partkey")
        return key;
    }

	static void UpdateSSTPBA(string sst_name){
		return GetInstance().updateSSTPBA(sst_name);
	}

	static void RequestSSTPBA(StorageEngineInstance::MetaDataRequest metadata_request, int &total_block_count, map<string,string> &sst_pba_map){
		return GetInstance().requestSSTPBA(metadata_request, total_block_count, sst_pba_map);
	}

private:
	TableManager();
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

	vector<string> getTableSSTList(string db_name, string table_name){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().TableManager_[db_name].table[table_name].sst_list;
	}

	int getTableIndexNumber(string db_name, string table_name){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().TableManager_[db_name].table[table_name].table_index_number;
	}

	void setTableInfo(string db_name, string table_name, Table table){
		std::lock_guard<std::mutex> lock(mutex_);
		TableManager_[db_name].table[table_name] = table;
	}

	void setDBInfo(string db_name, DB db){
		std::lock_guard<std::mutex> lock(mutex_);
		TableManager_[db_name] = db;
	}

	void setSSTPBAInfo(string sst_name, map<string, TableBlock> csd_pba_list){
		std::lock_guard<std::mutex> lock(mutex_);
		SSTManager_[sst_name].csd_pba_list = csd_pba_list;
	}

	int initTableManager();
	void dumpTableManager();
	void updateSSTPBA(string sst_name);
	void requestSSTPBA(StorageEngineInstance::MetaDataRequest metadata_request, int &total_block_count, map<string,string> &sst_pba_map);
	void updateSSTIndexTable(string sst_name);

    mutex mutex_;
	unordered_map<string, DB> TableManager_; // key: db name, value: struct DB
	unordered_map<string, SST> SSTManager_; //key: sst name, value: struct SST
};