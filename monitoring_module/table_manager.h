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

#include "keti_log.h"
#include "ip_config.h"

using namespace std;
using namespace rapidjson;

using StorageEngineInstance::ScanInfo;

class TableManager { /* modify as singleton class */

public:	
	struct Block {
		off64_t Offset;
		off64_t Length;
	};

	struct CSD {
		map<string, Block> pba_block_list;
	};

	struct SST {
		vector<CSD> csd_list;
		map<string, Block> lba_block_list; // key: block index handle, value:lba,pba
	};
	/*
		-block index handle: 블록 내 로우의 최대 pk 값
		[block_index_handle : 000002D08000001B | block_handle_offset : 0 | size : 4063]
		...
		ex) 000002D0 8000001B = (4byte table number + 4byte primary key)
	*/

	struct IndexTable {
		string index_table_number;
		map<string, string> index_table; //key: index column value, value: primary key 
	};
	/*
		HEX 000002D18000000380000280: 
		...
		ex) 000002D1 80000003 80000280 = (4byte index table number + (4byte index + 4byte primary key))
	*/

	struct Table {
		int table_index_number;
		map<string, SST> sst; // key: sst name, value: struct SST
		map<string, IndexTable> index_tables; // key: index column name, value: IndexTable
	};

	struct DB {
		map<string, Table> table; // key: table name, value: struct Table
	};

	static void DumpTableManager(){
		return GetInstance().dumpTableManager();
	}

	static bool CheckExistDB(string db_name){
		return GetInstance().checkExistDB(db_name);
	}

	static bool CheckExistTable(string db_name, string table_name){
		return GetInstance().checkExistTable(db_name, table_name);
	}

	static vector<string> GetSST(string db_name, string table_name){
		return GetInstance().getSST(db_name, table_name);
	}

	static int GetTableIndexNumber(string db_name, string table_name){
		return GetInstance().getTableIndexNumber(db_name, table_name);
	}

	static void GetPBA(ScanInfo scan_info, int &total_block_count, map<string,string> &sst_pba_map){
		return GetInstance().getPBA(scan_info, total_block_count, sst_pba_map);
	}

	static void SetTableInfo(string db_name, string table_name, Table table){
		return GetInstance().setTableInfo(db_name, table_name, table);
	}

	static void SetDBInfo(string db_name, DB db){
		return GetInstance().setDBInfo(db_name, db);
	}

	static string makeKey(int qid, int wid){
		string key = to_string(qid)+"|"+to_string(wid); // (ex:"s_suppkey","l_orderkey|l_partkey")
        return key;
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

	int initTableManager();

	void dumpTableManager(){
		cout << "-------------------------------------" << endl;
		cout << "# DB Info" << endl;
		int db_numbering = 1;
		for(const auto db : GetInstance().TableManager_){
			cout << db_numbering << ". db_name: " << db.first << endl;
			int table_numbering = 1;
			for(const auto table : db.second.table){
				cout << db_numbering << "-" << table_numbering << ". table_name: " << table.first << endl;
				cout << db_numbering << "-" << table_numbering << ". sst_list: ";
				for(const auto sst : table.second.sst){
					cout << sst.first << ", ";
				}
				cout << endl;
				table_numbering++;
			}
			db_numbering++;
		}
		cout << "-------------------------------------" << endl;
	}

	bool checkExistDB(string db_name){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().TableManager_.find(db_name) != GetInstance().TableManager_.end();
	}

	bool checkExistTable(string db_name, string table_name){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().TableManager_[db_name].table.find(table_name) != GetInstance().TableManager_[db_name].table.end();
	}

	vector<string> getSST(string db_name, string table_name){
		std::lock_guard<std::mutex> lock(mutex_);
		vector<string> sst_list;
		map<string, SST> sst_map = GetInstance().TableManager_[db_name].table[table_name].sst;
		for(const auto sst : sst_map){
			sst_list.push_back(sst.first);
		}
		return sst_list;
	}

	int getTableIndexNumber(string db_name, string table_name){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().TableManager_[db_name].table[table_name].table_index_number;
	}

	void getPBA(ScanInfo scan_info, int &total_block_count, map<string,string> &sst_pba_map);

	void setTableInfo(string db_name, string table_name, Table table){
		std::lock_guard<std::mutex> lock(mutex_);
		TableManager_[db_name].table[table_name] = table;
	}

	void setDBInfo(string db_name, DB db){
		std::lock_guard<std::mutex> lock(mutex_);
		TableManager_[db_name] = db;
	}

public:
    inline const static string LOGTAG = "Monitoring::Table Manager";

private:
    mutex mutex_;
	unordered_map<string, DB> TableManager_; // key: db name, value: struct DB
};