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
		vector<bool> is_primary;
	};

	struct Table {
		int table_index_number;
		vector<string> sst_list;
	};

	struct DB {
		map<string, Table> table;
	};

	static bool IsMetaDataInitialized(){
		return GetInstance().isMetaDataInitialized();
	}

	static bool IsDBFileInitialized(){
		return GetInstance().isDBFileInitialized();
	}

	static bool IsTableManagerInitialized(){
		return GetInstance().isTableManagerInitialized();
	}

	static bool CheckExistDB(string db_name){
		return GetInstance().checkExistDB(db_name);
	}

	static bool CheckExistTable(string db_name, string table_name){
		return GetInstance().checkExistTable(db_name, table_name);
	}

	static vector<string> GetCSD(string sst_name){
		return GetInstance().getCSD(sst_name);
	}

	static vector<string> GetSST(string db_name, string table_name){
		return GetInstance().getSST(db_name, table_name);
	}

	static int GetTableIndexNumber(string db_name, string table_name){
		return GetInstance().getTableIndexNumber(db_name, table_name);
	}

	static void SetMetaDataInitialized(){
		return GetInstance().setMetaDataInitialized();
	}

	static void SetDBFileInitialized(){
		return GetInstance().setDBFileInitialized();
	}

	static void SetCSD(string sst_name, CSD csd){
		return GetInstance().setCSD(sst_name, csd);
	}

	static void SetTableInfo(string db_name, string table_name, Table table){
		return GetInstance().setTableInfo(db_name, table_name, table);
	}

	static void SetDBInfo(string db_name, DB db){
		return GetInstance().setDBInfo(db_name, db);
	}

private:
	TableManager() {
		MetaDataInitialized_ = false;
		DBFileInitialized_= false;
	};
    TableManager(const TableManager&);
    TableManager& operator=(const TableManager&){
        return *this;
    };

    static TableManager& GetInstance() {
        static TableManager _instance;
        return _instance;
    }

	bool isMetaDataInitialized(){
		return GetInstance().MetaDataInitialized_;
	}
	
	bool isDBFileInitialized(){
		return GetInstance().DBFileInitialized_;
	}

	bool isTableManagerInitialized(){
		return GetInstance().MetaDataInitialized_ && GetInstance().DBFileInitialized_;
	}

	bool checkExistDB(string db_name){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().TableManager_.find(db_name) != GetInstance().TableManager_.end();
	}

	bool checkExistTable(string db_name, string table_name){
		std::lock_guard<std::mutex> lock(mutex_);
		return GetInstance().TableManager_[db_name].table.find(table_name) != GetInstance().TableManager_[db_name].table.end();
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

	void setMetaDataInitialized(){
		GetInstance().MetaDataInitialized_ = true;
	}

	void setDBFileInitialized(){
		GetInstance().DBFileInitialized_ = true;
	}

	void setCSD(string sst_name, CSD csd){
		std::lock_guard<std::mutex> lock(mutex_);
		SSTCSDMap_[sst_name] = csd;
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
	bool MetaDataInitialized_;
	bool DBFileInitialized_;
	unordered_map<string,struct DB> TableManager_;
	unordered_map<string, struct CSD> SSTCSDMap_;
};