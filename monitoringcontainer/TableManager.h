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

#include "storage_engine_instance.grpc.pb.h"

#include "keti_log.h"
#include "ip_config.h"

using namespace std;
using StorageEngineInstance::Snippet;
using StorageEngineInstance::MetaDataResponse;

struct InternalRequest {
	int query_id;
	int work_id;
	string table_name;
};

struct MetaDataResponse_ {
	MetaDataResponse* metadataResponse;
	// Snippet snippet;
	int block_count;
	bool is_done;
	condition_variable cond;
	mutex mu;
	bool lba2pba_done = false;
	bool index_scan_done = false;
	bool wal_done = false;//아직 wal 수행 전
	MetaDataResponse_(){
		this->metadataResponse = new StorageEngineInstance::MetaDataResponse;
	}
};

class TableManager { /* modify as singleton class */

/* Methods */
public:
	struct ColumnSchema {
		string column_name;
		int type;
		int length;
		int offset;
	};
	
	struct DataBlockHandle {
		string IndexBlockHandle;
		off64_t Offset;
		off64_t Length;
	};
	
	struct SSTFile{
		string filename;
		vector<struct DataBlockHandle> BlockList;
		map<string,vector<string>> IndexTable;
	};
	
	struct Table {
		string tablename;
		int tablesize;
		vector<struct ColumnSchema> Schema;
		vector<struct SSTFile> SSTList;
		vector<string> IndexList;
		vector<string> PK; 
	};

	static int GetTableSchema(string tablename,vector<struct ColumnSchema> &dst){
		return GetInstance().getTableSchema(tablename,dst);
	}

	static vector<string> GetOrderedTableBySize(vector<string> tablenames){
		return GetInstance().getOrderedTableBySize(tablenames);
	}

	static int GetIndexList(string tablename, vector<string> &dst){
		return GetInstance().getIndexList(tablename,dst);
	}

	static vector<string> GetSSTList(string tablename){
		return GetInstance().getSSTList(tablename);
	}

	static void PrintTableManager(){
		GetInstance().printTableManager();
	}

	static Table GetTable(string tablename){
		return GetInstance().m_TableManager[tablename];
	}

	static Table GetMutableTable(string tablename){
		Table& table_ref = GetInstance().m_TableManager[tablename];
		return table_ref;
	}

	// static Table SetResponseSnippet(Snippet snippet, string key){
	// 	GetInstance().return_data[key]->snippet->CopyFrom(snippet);
	// }

	// monitoring container에서 response 객체는 query_id와 work_id를 조합한 key로 관리됨
	static string makeKey(int qid, int wid){
		string key = to_string(qid)+"|"+to_string(wid);
        return key;
    }

	static void SetReturnData(string key){
		MetaDataResponse_* response = new MetaDataResponse_;
		GetInstance().return_data[key] = response;
	}

	static MetaDataResponse_* GetReturnData(string key){
		return GetInstance().return_data[key];
	}

	static void InitTableManager(){
		GetInstance().initTableManager();
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

	int initTableManager();
	int getTableSchema(string tablename,vector<struct ColumnSchema> &dst);
	vector<string> getOrderedTableBySize(vector<string> tablenames);
	int getIndexList(string tablename, vector<string> &dst);
	vector<string> getSSTList(string tablename);
	void printTableManager();

/* Variables */
public:
    inline const static string LOGTAG = "Monitoring Container::Table Manager";

private:
    mutex Table_Mutex;
	unordered_map<string,struct Table> m_TableManager;
	unordered_map<string,MetaDataResponse_*> return_data;
};