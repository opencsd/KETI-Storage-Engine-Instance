// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
// This source code is licensed under both the GPLv2 (found in the
// COPYING file in the root directory) and Apache 2.0 License
// (found in the LICENSE.Apache file in the root directory)

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <map> //추가
#include <vector>
#include <array> 
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include "rocksdb/sst_file_reader.h"
#include "rocksdb/sst_file_manager.h"
#include "rocksdb/slice.h"
#include "rocksdb/iterator.h"
#include "rocksdb/table_properties.h"

using ROCKSDB_NAMESPACE::SstFileReader;
using ROCKSDB_NAMESPACE::Slice;
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::Iterator;
using ROCKSDB_NAMESPACE::ReadOptions;
using ROCKSDB_NAMESPACE::Options;
using ROCKSDB_NAMESPACE::TableProperties;
using namespace rapidjson;
using namespace std;

#if defined(OS_WIN)
std::string kDBPath = "C:\\Windows\\TEMP\\rocksdb_simple_example";
#else
//const char *file_path = "/root/workspace/HUN/csd_file_scan/keti/sstFile/002269index.sst"; //index
//const char *file_path = "/root/workspace/HUN/csd_file_scan/keti/sstFile/002284pkindex.sst"; //pk+index
//const char *file_path = "/root/workspace/HUN/csd_file_scan/keti/sstFile/002343indexscan.sst"; //index
//const char *file_path = "/root/workspace/HUN/csd_file_scan/keti/sstFile/002411noindexscan.sst"; //no index
const char *file_path = "/root/workspace/HUN/csd_file_scan/keti/sst/004096.sst";
#endif

int main(int argc, char **argv) {
    map<string, vector<string>> indexScanMap; //Index Data map
    //vector<string> indexVecStr, pkVecStr;
    //vector<int> indexVecInt, pkVecInt;
    //std::string filename = argv[1];  
    //const char *file_path = filename.c_str();  //file 이름 직접 입력받기
    Options options; 
    ReadOptions readOptions;
    SstFileReader sstFileReader(options);
    TableProperties tableProperties;
    Status s = sstFileReader.Open(file_path); 
    // Status s = sstFileReader.KetiOpen(file_path);
    Iterator* it = sstFileReader.NewIterator(readOptions);
    if(!s.ok()){
      std::cout << "open error" << std::endl;
    }

    struct tableMetaData { // 1.tablemanager.cc에서 파싱 작업 -> 2.구조체에 메타데이터 저장 -> 3.IndexTblGenManager에서 구조체 관리
        string tableIndexNum = "0000018B";
        bool pkExist = true;
        int indexCnt = 2; 
        int pkCnt = 2;
        //각 index와 pk 컬럼들에 대한 길이 정보
        vector<string> indexColumnNames = {"id", "age"};
        vector<string> pkColumnNames = {"age", "id"};
        vector<int> indexColumnBytes = {4,4}; 
        vector<int> pkColumnBytes = {4,4};
    };

    //sst 파일 읽어서 seek 진행
    for (it->SeekToFirst(); it->Valid(); it->Next()) { //sst file에서 key, value 가져오기
        //std::cout << "Key: " << it->key().ToString(true) << ", Value: " << it->value().ToString(true) << std::endl;
        const Slice& key = it->key();
        const Slice& value = it->value();     
        tableMetaData MetaData;
        string indexList = " ";
        vector<std::string> pkList;
        vector<std::string> noPkIndexList;     
        int BlockOffset = 8; //블록 첫 부분의 table index num은 4바이트 고정이므로 시작 오프셋은 4x2=8
        string keyData = key.ToString(true); // 키 추출
        string tableIndexNum = keyData.substr(0,8); //table index num 추출
        //테이블 인덱스 번호가 같으므로 내가 찾으려는 인덱스 테이블 블록임
        if (value.empty() && (tableIndexNum.compare(MetaData.tableIndexNum)==0)){ 
            //index block 분류
            for (int i=0; i<MetaData.indexCnt; i++){
                int indexSize = MetaData.indexColumnBytes.at(i)*2; //해당 인덱스 컬럼의 length
                indexList += "|" + keyData.substr(BlockOffset, indexSize); //구분자로 구분하여 인덱스 리스트에 추가
                noPkIndexList.push_back(keyData.substr(BlockOffset, indexSize)); //pk가 없는 경우 해당 벡터를 value로 push back
                BlockOffset += indexSize; //블록 오프셋 이동
            }

            //pk 존재 유무
            if (MetaData.pkExist){ 
                //pk와 index의 동일여부 판단(일단 아예 같은 경우만 고려), 벡터끼리의 비교를 하려면 각 벡터의 원소를 하나씩 비교해야함
                if (areVectorsEqual(MetaData.pkColumnNames, MetaData.indexColumnNames)){ 
                    int newBlockOffSet=8; //같은 데이터를 key, value에 각각 string, vector로 들어가므로 offset도 달라야 함.
                    for (int i=0; i<MetaData.pkCnt; i++){
                        int pkSize = MetaData.pkColumnBytes.at(i)*2; //실제 블록에는 bytes size * 2의 데이터 저장됨
                        indexScanMap[indexList].push_back(keyData.substr(newBlockOffSet, pkSize));
                        newBlockOffSet += pkSize;
                    }
                } 
                else { 
                    //pk block 분류 작업 
                    for (int i=0; i<MetaData.pkCnt; i++){
                        int pkSize = MetaData.pkColumnBytes.at(i)*2; //실제 블록에는 bytes size * 2의 데이터 저장됨
                        indexScanMap[indexList].push_back(keyData.substr(BlockOffset, pkSize));
                        BlockOffset += pkSize;
                    }
                    //indexScanMap[indexList].push_back(pkList);
                }
            }
            else { 
                indexScanMap[indexList].push_back(keyData.substr(BlockOffset)); //indexScanMap에 key(인덱스), value(InternalBytes) 저장
            }
        }
    }    
    
    // map 순회 및 값 출력
    // indexScanMap의 값을 추출하여 출력
    for (const auto& entry : indexScanMap) {
        const std::string& key = entry.first;
        const std::vector<std::string>& values = entry.second;

        std::cout << "Key: " << key << ", Values: ";
        for (const auto& value : values) {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
    
    return 0;
}