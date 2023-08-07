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

#include "./rocksdb/include/rocksdb/sst_file_reader.h"
#include "./rocksdb/include/rocksdb/sst_file_manager.h"
#include "./rocksdb/include/rocksdb/slice.h"
#include "./rocksdb/include/rocksdb/iterator.h"
#include "./rocksdb/include/rocksdb/table_properties.h"

#include "TableManager.h" //추가 : table manager로부터 정보 받아오기

class IndexTblGenManager {
    public:
        static void InitIndexTblGenManager(){
            GetInstance().initIndexTblGenManager();
        }
    private:
        IndexTblGenManager() {};
        IndexTblGenManager(const IndexTblGenManager&);
        IndexTblGenManager& operator=(const IndexTblGenManager&){
            return *this;
        };
        
        static IndexTblGenManager& GetInstance(){
            static IndexTblGenManager _instance;
            return _instance;
        }   
    
        int initIndexTblGenManager();
        int getTableMetaData(string tablename, struct tmd);
};