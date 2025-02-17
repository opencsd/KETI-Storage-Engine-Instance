#pragma once
#include <vector>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <fstream>
#include <string>
#include <algorithm>
#include <stdlib.h>
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
#include "tb_block.h"
#include "log_to_file.h"

using namespace std;
using namespace rapidjson;

using StorageEngineInstance::SnippetRequest;

#define NUM_OF_BLOCKS 100
#define BUFF_SIZE (NUM_OF_BLOCKS * 5000)
#define T_BUFFER_SIZE 4194304
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
  int type;//0 empty, 1 string, 2 int, 3 float, 4 boolean, 5 date (KETI_VECTOR_TYPE)
  int row_count;
  int real_size;
  ColData(){
    strvec.clear();
    intvec.clear();
    floatvec.clear();
    isnull.clear();
    type = TYPE_EMPTY; 
    row_count = 0;
    real_size = 0;
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

      query_id = document["query_id"].GetInt();
      work_id = document["work_id"].GetInt();
      table_alias = document["table_alias"].GetString();

      row_count = document["row_count"].GetInt();

      Value &row_offset_ = document["row_offset"];
      for(int i = 0; i<row_offset_.Size(); i++){
          row_offset.push_back(row_offset_[i].GetInt());
      }

      Value &column_alias_ = document["column_alias"];
      for(int i = 0; i<column_alias_.Size(); i++){
          column_alias.push_back(column_alias_[i].GetString());
      }
      
      Value return_datatype_ = Value(rapidjson::kArrayType);;
      if (document.HasMember("return_column_type") && document["return_column_type"].IsArray()) {
        return_datatype_ = document["return_column_type"];
      }

      Value return_offlen_= Value(rapidjson::kArrayType);;
      if (document.HasMember("return_column_length") && document["return_column_length"].IsArray()) {
        return_offlen_ = document["return_column_length"];
      }
     
      for(int i = 0; i<return_datatype_.Size(); i++){
        if (!return_datatype_[i].IsInt() || !return_offlen_[i].IsInt()) {
            cout << "Error: Invalid data type in return_column_type or return_column_length." << endl;
            continue;
        }
          int datatype_value = return_datatype_[i].GetInt();
          int offlen_value = return_offlen_[i].GetInt();
          return_datatype.push_back(datatype_value);
          return_offlen.push_back(offlen_value);
          if(return_datatype_[i].GetInt() > 3000 || return_offlen_[i].GetInt() > 3000){
            cout <<"weird data~!" << endl;
          }
      }        

      length = document["length"].GetInt();

      memcpy(data, data_, length);

      result_block_count = document["current_block_count"].GetInt();
      table_total_block_count = document["total_block_count"].GetInt();

      scanned_row_count = document["scanned_row_count"].GetInt();
      filtered_row_count = document["filtered_row_count"].GetInt();
    }
};

struct WorkBuffer {
  kQueue<BlockResult> work_buffer_queue;
  string table_alias;//*결과 테이블의 별칭
  vector<string> table_column;//*결과 컬럼명
  unordered_map<string,ColData> table_data;//결과의 컬럼 별 데이터(컬럼명,타입,데이터)
  int table_total_block_count;
  int merged_block_count;//*처리 블록 수(csd 결과 병합 체크용) -> Monitoring Container 통신
  int status;//작업 상태
  int row_count;//행 개수
  condition_variable work_done_condition;
  condition_variable work_in_progress_condition;
  mutex mu;

  WorkBuffer(){
    table_alias.clear();
    table_column.clear();
    status = Initialized;
    table_data.clear();
    table_total_block_count = 0;
    merged_block_count = 0;
    row_count = 0;
  }

  void save_table_column_type(vector<int> return_datatype, vector<int> return_column_length){
    for(size_t i=0; i<table_column.size(); i++){
        string col_name = table_column[i];
        int col_type = return_datatype[i];
        switch (col_type){
            case MySQL_DataType::MySQL_BYTE:{
                table_data[col_name].type = TYPE_INT;
                break;
            }case MySQL_DataType::MySQL_INT16:{
                table_data[col_name].type = TYPE_INT;     
                break;
            }case MySQL_DataType::MySQL_INT32:{
                table_data[col_name].type = TYPE_INT;
                break;
            }case MySQL_DataType::MySQL_INT64:{
                table_data[col_name].type = TYPE_INT;
                break;
            }case MySQL_DataType::MySQL_FLOAT32:{
                table_data[col_name].type = TYPE_FLOAT;
                break;
            }case MySQL_DataType::MySQL_DOUBLE:{
                table_data[col_name].type = TYPE_FLOAT;
                break;
            }case MySQL_DataType::MySQL_NEWDECIMAL:{
                table_data[col_name].type = TYPE_FLOAT;
                table_data[col_name].real_size = (return_column_length[i] - 6) * 2;
                break;
            }case MySQL_DataType::MySQL_DATE:{
                table_data[col_name].type = TYPE_DATE;
                break;
            }case MySQL_DataType::MySQL_TIMESTAMP:{
                table_data[col_name].type = TYPE_INT;
                break;
            }case MySQL_DataType::MySQL_STRING:{
                table_data[col_name].type = TYPE_STRING;
                break;
            }case MySQL_DataType::MySQL_VARSTRING:{
                table_data[col_name].type = TYPE_STRING;
                break;
            }
        }
    }
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

  ~QueryBuffer(){
    for (auto& pair : work_buffer_list) {
        delete pair.second;
    }
  }
};

struct ChunkBuffer {
  int chunk_count;
  string result;
};

struct TmaxQueryBuffer{
  mutex mu;
  condition_variable available;
  vector<ChunkBuffer> chunk_buffer;

  TmaxQueryBuffer(){
    chunk_buffer.clear();
  };
};

struct TableData{//결과 리턴용
  int scanned_row_count;
  int filtered_row_count;
  unordered_map<string,ColData> table_data;//결과 데이터
  int row_count;
  int status;

  TableData(){
    table_data.clear();
    row_count = 0;
  }
};

class BufferManager{
  public:
    static void InitializeBuffer(int qid, int wid, string tname){
      return GetInstance().initializeBuffer(qid, wid, tname);
    }

    static TableData GetFinishedTableData(int qid, int wid, string tname){
      return GetInstance().getFinishedTableData(qid, wid, tname);
    }

    static TableData GetTableData(int qid, int wid, string tname, int row_index){
      return GetInstance().getTableData(qid, wid, tname, row_index);
    }

    static int SaveTableData(SnippetRequest snippet, TableData &table_data_, int offset, int length){
      return GetInstance().saveTableData(snippet, table_data_, offset, length);
    }

    static int CheckTableStatus(int qid, int wid, string tname){
      return GetInstance().checkTableStatus(qid, wid, tname);
    }

    static int EndQuery(StorageEngineInstance::Request qid){
      return GetInstance().endQuery(qid);
    }

    static void InitBufferManager(){
      GetInstance().initBufferManager();
    }
    static ChunkBuffer GetTmaxQueryResult(int id){
      return GetInstance().t_get_result(id);
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
    TableData getFinishedTableData(int qid, int wid, string tname);//return table data on queryID/tableName
    TableData getTableData(int qid, int wid, string tname, int row_index);//return table data on queryID/tableName
    int saveTableData(SnippetRequest snippet, TableData &table_data_, int offset, int length);
    int checkTableStatus(int qid, int wid, string tname);
    int endQuery(StorageEngineInstance::Request qid);

    void t_buffer_manager_interface();
    void t_result_merging(string json, u_char *data,  size_t data_size);
    ChunkBuffer t_get_result(int id);
    void t_initialize_buffer(int id);

    inline const static std::string LOGTAG = "Merging::Buffer Manager";
  private:
    unordered_map<int, struct QueryBuffer*> DataBuffer_;//buffer manager data buffer
    unordered_map<int, TmaxQueryBuffer> TmaxDataBuffer_; //key : id, value : TmaxQueryBuffer
    thread BufferManagerInterface;//get csd result thread
    mutex buffer_mutex_;
    mutex t_buffer_mutex_;
};
