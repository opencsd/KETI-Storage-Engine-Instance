#pragma once
#include <vector>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <fstream>
#include <string>
#include <algorithm>
#include <queue>
#include <tuple>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <list>
#include <unordered_set>
#include <bitset>
#include <stack>
#include <thread>
#include <chrono>
#include <iostream>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h" 

#include "storage_engine_instance.grpc.pb.h"

#include "ip_config.h"
#include "keti_type.h"
#include "keti_log.h"
#include "internal_queue.h"

using namespace std;
using namespace rapidjson;

using StorageEngineInstance::Snippet;

#define NUM_OF_BLOCKS 15
#define BUFF_SIZE (NUM_OF_BLOCKS * 5000)
#define NCONNECTION 8
#define ID_INIT_WAIT_MAX_TIME 1000

template <typename T>
struct Block_Buffer;
struct Work_Buffer;

struct ColData{
  vector<string> strvec;
  vector<int64_t> intvec;
  vector<double> floatvec;
  vector<bool> isnull;//해당 인덱스 null 여부 (조인시)
  int type;//0 empty, 1 string, 2 int, 3 float, 4 boolean (KETI_VECTOR_TYPE)
  int row_count;
  ColData(){
    strvec.clear();
    intvec.clear();
    floatvec.clear();
    isnull.clear();
    type = TYPE_EMPTY; 
    row_count = 0;
  }
};

struct BlockResult{//csd 결과 데이터 파싱 구조체
    int query_id;
    int work_id;  
    char data[BUFF_SIZE];
    int length;
    vector<int> row_offset; 
    int row_count;
    string csd_name;
    int table_total_block_count;
    int result_block_count;
    vector<int> return_datatype;//결과의 컬럼 데이터 타입(csd 결과 파싱용) -> CSD가 리턴
    vector<int> return_offlen;//결과의 컬럼 길이 (csd 결과 파싱용) -> CSD가 리턴
    int scanned_row_count;
    int filtered_row_count;
    string table_alias;
    vector<string> column_alias;

    BlockResult(){}
    BlockResult(const char* json_, char* data_){
      Document document;
      document.Parse(json_); 

      query_id = document["queryID"].GetInt();
      work_id = document["workID"].GetInt();
      table_alias = document["tableAlias"].GetString();

      row_count = document["rowCount"].GetInt();

      Value &row_offset_ = document["rowOffset"];
      for(int i = 0; i<row_offset_.Size(); i++){
          row_offset.push_back(row_offset_[i].GetInt());
      }

      Value &column_alias_ = document["columnAlias"];
      for(int i = 0; i<column_alias_.Size(); i++){
          column_alias.push_back(column_alias_[i].GetString());
      }

      Value &return_datatype_ = document["returnDatatype"];
      Value &return_offlen_ = document["returnOfflen"];
      for(int i = 0; i<return_datatype_.Size(); i++){
          return_datatype.push_back(return_datatype_[i].GetInt());
          return_offlen.push_back(return_offlen_[i].GetInt());
      }        

      length = document["length"].GetInt();

      memcpy(data, data_, length);

      csd_name = document["csdName"].GetString();
      result_block_count = document["resultBlockCount"].GetInt();
      table_total_block_count = document["tableTotalBlockCount"].GetInt();

      scanned_row_count = document["scannedRowCount"].GetInt();
      filtered_row_count = document["filteredRowCount"].GetInt();
    }
};

struct WorkBuffer {
  kQueue<BlockResult> work_buffer_queue;
  string table_alias;//*결과 테이블의 별칭
  vector<string> table_column;//*결과 컬럼명
  unordered_map<string,ColData> table_data;//결과의 컬럼 별 데이터(컬럼명,타입,데이터)
  int left_block_count;//*남은 블록 수(csd 결과 병합 체크용) -> Monitoring Container 통신
  int status;//작업 상태
  int row_count;//행 개수
  condition_variable cond;
  mutex mu;

  WorkBuffer(){
    table_alias.clear();
    table_column.clear();
    status = Initialized;
    table_data.clear();
    left_block_count = 0;
    row_count = 0;
  }
};

struct QueryBuffer{
  int query_id;//*쿼리ID
  int scanned_row_count;
  int filtered_row_count;
  unordered_map<int,WorkBuffer*> work_buffer_list;//워크버퍼 <key:*workID, value:Work_Buffer>
  unordered_map<string,int> tablename_workid_map;//<key:table_name, value: work_id>

  QueryBuffer(int qid)
  :query_id(qid){
    work_buffer_list.clear();
    tablename_workid_map.clear();
    scanned_row_count = 0;
    filtered_row_count = 0;
  }
};

struct TableData{//결과 리턴용
  bool valid;//결과의 유효성
  int scanned_row_count;
  int filtered_row_count;
  unordered_map<string,ColData> table_data;//결과 데이터
  int row_count;

  TableData(){
    valid = false;
    table_data.clear();
    row_count = 0;
  }
};

class BufferManager{
  public:
    static void InitializeBuffer(int qid, int wid, string tname){
      return GetInstance().initializeBuffer(qid, wid, tname);
    }

    static TableData GetTableData(int qid, int wid, string tname){
      return GetInstance().getTableData(qid, wid, tname);
    }

    static int SaveTableData(Snippet snippet, TableData &table_data_, int offset, int length){
      return GetInstance().saveTableData(snippet, table_data_, offset, length);
    }

    static int EndQuery(StorageEngineInstance::Request qid){
      return GetInstance().endQuery(qid);
    }

    static void InitBufferManager(){
      GetInstance().initBufferManager();
    }

  private:
    BufferManager(){};

    BufferManager(const BufferManager&);
    ~BufferManager() {};
    BufferManager& operator=(const BufferManager&){
        return *this;
    };

    static BufferManager& GetInstance() {
        static BufferManager bufferManager;
        return bufferManager;
    }

    void initBufferManager();
    void bufferManagerInterface();
    void pushResult(BlockResult blockResult);
    void mergeResult(int qid, int wid);
    void initializeBuffer(int qid, int wid, string tname);//return true only when the snippet work done
    TableData getTableData(int qid, int wid, string tname);//return table data on queryID/tableName
    int saveTableData(Snippet snippet, TableData &table_data_, int offset, int length);
    int endQuery(StorageEngineInstance::Request qid);

    void t_buffer_manager_interface();
    void t_result_merging(char* t_data);
    void t_result_sending();

    inline const static std::string LOGTAG = "Merging::Buffer Manager";

  private:
    unordered_map<int, struct QueryBuffer*> DataBuffer_;//buffer manager data buffer
    thread BufferManagerInterface;//get csd result thread
    mutex buffer_mutex_;
};
