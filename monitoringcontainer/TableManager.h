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
#include "rocksdb/sst_file_reader.h"
#include "./rocksdb/include/rocksdb/sst_file_manager.h"
#include "./rocksdb/include/rocksdb/slice.h"
#include "./rocksdb/include/rocksdb/iterator.h"
#include "./rocksdb/include/rocksdb/table_properties.h"

#include "storage_engine_instance.grpc.pb.h"

#include "keti_log.h"
#include "ip_config.h"

using namespace std;
using StorageEngineInstance::Snippet;
using StorageEngineInstance::MetaDataResponse;

struct Response {
	MetaDataResponse* metadataResponse;
	// Snippet snippet;
	int block_count;
	bool is_done;
	condition_variable cond;
	mutex mu;
	bool lba2pba_done = false;
	bool index_scan_done = false;
	bool wal_done = false;//아직 wal 수행 전
	Response(){
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
		bool isPk; //추가 : 해당 컬럼이 pk인지의 여부 -> 어느 구조체에서 관리가 되어야 할까? 
		bool isIndex; //추가 : 해당 컬럼이 index인지의 여부 -> 어느 구조체에서 관리가 되어야 할까? 
	};
	
	struct SSTFile{ 
		string filename;
		vector<struct DataBlockHandle> BlockList; //sst마다 block들이 list로 관리됨
		map<string,vector<string>> IndexTable; //sst마다 인덱스가 list로 관리됨
		string vector;
	};

	struct DataBlockHandle {
		string IndexBlockHandle; //table index 번호, 스캔할 블록의 시작점 -> 처음 데이터 테이블 세팅 과정에서 이 데이터를 가지고 해당 블록인지 비교해야 함
		off64_t Offset;
		off64_t Length;
	};
	
	struct Table {
		string tablename;
		int tablesize;
		vector<struct ColumnSchema> Schema;
		vector<struct SSTFile> SSTList;
		vector<string> IndexList;
		vector<string> PK;

		//추가: pk 존재 여부, index 및 pk로 지정된 컬럼들의 이름, 갯수 및 바이트 사이즈
		bool PkExist; //추가 : pk 존재 여부
		vector<string> IndexColumnNames; //추가 : index로 지정된 컬럼들의 이름, 갯수 및 바이트 사이즈
		vector<int> IndexColumnBytes;
		int IndexCnt; 
		vector<string> PkColumnNames; //추가 : pk로 지정된 컬럼들의 이름, 갯수 및 바이트 사이즈
		vector<int> PkColumnBytes;
		int PkCnt;
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

	static GetIndexTable(){

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
		Response* response = new Response;
		GetInstance().return_data[key] = response;
	}

	static Response* GetReturnData(string key){
		return GetInstance().return_data[key];
	}

	static void InitTableManager(){
		GetInstance().initTableManager();
	}

private:
	TableManager() {}; //기본 생성자 정의. {} 부분이 비어있으므로 아무 동작 x  
    TableManager(const TableManager&); //복사 생성자를 선언하는 코드
    TableManager& operator=(const TableManager&){ //할당 연산자 선언. 이미 생성된 객체에 다른 객체의 값을 할당할 때 호출되는 함수
        return *this;
    };

	//싱글톤 패턴을 사용하여 클래스의 인스턴스를 하나로 유지

    static TableManager& GetInstance() { //정적으로 선언되어 클래스의 유일한 인스턴스를 반환. 이 함수를 통해 인스턴스를 가져오므로 항상 동일한 인스턴스를 사용하게 됨
        static TableManager _instance;
        return _instance;
    }

	int initTableManager(); //TableManager의 초기화 함수
	int getTableSchema(string tablename,vector<struct ColumnSchema> &dst); //특정 테이블의 스키마 정보를 가져옴
	vector<string> getOrderedTableBySize(vector<string> tablenames); //크기 순으로 정렬된 테이블의 이름을 가져옴
	int getIndexList(string tablename, vector<string> &dst); //특정 테이블의 인덱스 목록을 가져옴
	vector<string> getSSTList(string tablename); //특정 테이블의 sst 파일 목록을 가져옴
	void printTableManager(); //테이블 매니저의 정보를 출력하는 함수

/* Variables */
public:
    inline const static string LOGTAG = "Monitoring Container::Table Manager";

private:
    mutex Table_Mutex;
	unordered_map<string,struct Table> m_TableManager; //key : 테이블 이름, value : 테이블 정보 구조체
	unordered_map<string,Response*> return_data;
};