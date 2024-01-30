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

using namespace std;

class TableManager { /* modify as singleton class */

/* Methods */
public:	
	struct CSD {
		vector<string> csd_list;
	};

	struct Table {
		int table_index_number;
		vector<string> sst_list;
	};

	struct DB {
		map<string, Table> table;
	};

	static vector<string> GetCSD(string sst_name){
		return GetInstance().getCSD(sst_name);
	}

	static vector<string> GetSST(string db_name, string table_name){
		return GetInstance().getSST(db_name, table_name);
	}

	static int GetTableIndexNumber(string db_name, string table_name){
		return GetInstance().getTableIndexNumber(db_name, table_name);
	}

	static void SetCSD(string sst_name, vector<string> csd_list){
		return GetInstance().setCSD(sst_name, csd_list);
	}

	static void SetTableInfo(string db_name, string table_name, Table table){
		return GetInstance().setTableInfo(db_name, table_name, table);
	}

	static void SetDBInfo(string db_name, DB db){
		return GetInstance().setDBInfo(db_name, db);
	}

private:
	TableManager() {};
    TableManager(const TableManager&);
    TableManager& operator=(const TableManager&){
        return *this;
    };

    static TableManager& GetInstance() {
        static TableManager _instance;
        return _instance;
    }

	vector<string> getCSD(string sst_name){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().SSTCSDMap_[sst_name].csd_list;
	}

	vector<string> getSST(string db_name, string table_name){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().TableManager_[db_name].table[table_name].sst_list;
	}

	int getTableIndexNumber(string db_name, string table_name){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().TableManager_[db_name].table[table_name].table_index_number;
	}

	void setCSD(string sst_name, vector<string> csd_list){
		std::lock_guard<std::mutex> lock(mutex_);
		SSTCSDMap_[sst_name].csd_list = csd_list;
	}

	void setTableInfo(string db_name, string table_name, Table table){
		std::lock_guard<std::mutex> lock(mutex_);
		TableManager_[db_name].table[table_name] = table;
	}

	void setDBInfo(string db_name, DB db){
		std::lock_guard<std::mutex> lock(mutex_);
		TableManager_[db_name] = db;
	}

/* Variables */
public:
    inline const static string LOGTAG = "Monitoring::Table Manager";

private:
    mutex mutex_;
	unordered_map<string,struct DB> TableManager_;
	unordered_map<string, struct CSD> SSTCSDMap_;
};