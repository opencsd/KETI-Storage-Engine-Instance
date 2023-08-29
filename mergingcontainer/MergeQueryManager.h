#pragma once

#include "BufferManager.h"

using namespace std;

inline std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v\0"){
	s.erase(s.find_last_not_of(t) + 1);
	return s;
}inline std::string& ltrim(std::string& s, const char* t = " \t\n\r\f\v\0"){
	s.erase(0, s.find_first_not_of(t));
	return s;
}inline std::string& trim(std::string& s, const char* t = " \t\n\r\f\v\0"){
	return ltrim(rtrim(s, t), t);
}

struct T{
    string varString;
    int64_t varInt;
    double varFloat;
    bool isnull;
    bool boolean;
    int type;//0 string, 1 int, 2 float, 3 boolean (KETI_VECTOR_TYPE)
    
    T(){
        varString = "";
        varInt = 0;
        varFloat = 0;
        isnull = false;
        boolean = false;
        type = -1;
    }
};

class MergeQueryManager{	
public:
    MergeQueryManager(StorageEngineInstance::SnippetRequest snippet_){
        this->snippet = snippet_.snippet();
        this->snippetType = snippet_.type();
        this->group_by_table.clear();
        this->group_by_key.clear();
        this->ordered_index.clear();
    };

    void RunSnippetWork();//스니펫 작업 수행

    string makeGroupbyKey(int row_index){
        string key = "";
        for(int g=0; g<snippet.group_by_size(); g++){
            string groupby_col_name = snippet.group_by(g);
            int groupby_col_type = target_table.table_data[groupby_col_name].type;
            string trim_str;
            switch(groupby_col_type){
            case TYPE_STRING:
                trim_str  = trim(target_table.table_data[groupby_col_name].strvec[row_index]);
                key += "|" + trim_str;
                break;
            case TYPE_INT:
                key += "|" + to_string(target_table.table_data[groupby_col_name].intvec[row_index]);
                break;
            case TYPE_FLOAT:
                key += "|" + to_string(target_table.table_data[groupby_col_name].floatvec[row_index]);
                break;
            }
        }
        return key;
    }

    inline const static std::string LOGTAG = "Merging Container::Merge Query Manager";

private:
    StorageEngineInstance::Snippet snippet;//스니펫
    int snippetType;//스니펫 작업 타입
    int tableCnt;//작업 대상 테이블 개수
    bool isGroupby;//그룹바이 여부
    bool isOrderby;//오더바이 여부
    bool isHaving;//해빙절 여부
    bool isLimit;//리미트 여부
    bool right_is_smaller;//true = right is smaller, false = left is smaller
    bool equal_join_exist;
    TableData left_table;//table1(좌항)
    TableData right_table;//table2(우항)
    unordered_map<string,vector<int>> hash_table;//key:index 해시 조인 시(left, right 로우가 적은 쪽)
    TableData target_table;//프로젝션 대상 테이블(left_table X right_table)
    map<string,int> group_by_key;//<그룹바이기준컬럼,그룹바이 테이블 인덱스>
    vector<TableData> group_by_table;//그룹바이된 테이블 컬럼(group by)
    TableData result_table;//결과 테이블(projected)->버퍼저장
    TableData having_table;//결과 테이블에 해빙절 수행 테이블
    vector<int> ordered_index;//result_table의 정렬 후 인덱스 순서
    TableData order_by_table;//결과 테이블(ordered)->버퍼저장

private:
    void Aggregation();//column projection, 모든 스니펫이 기본적으로 수행
    T Projection(int groupIndex, int projectionIndex, int rowIndex);//프로젝션 수행,Aggregation 호출
    T Postfix(int groupIndex, int projectionIndex, int rowIndex, int start, int end);//실제 postfix 계산
    void GroupBy();//group by, 그룹바이 절이 있으면 수행
    void OrderBy();//order by, 오더바이 절이 있으면 수행
    void InnerJoin_nestedloop();//inner join (nested loop join)
    void InnerJoin_hash();//inner join (hash join + nested loop join)
    void LeftOuterJoin_nestedloop();//left outer join (nested loop join)
    void LeftOuterJoin_hash();//left outer join (hash join)
    void RightOuterJoin_hash();//right outer join (hash join)
    void CrossJoin();//cartesian product
    void Union();//left and right table union
    void In();//non dependency in
    void DependencyInnerJoin();//dependency join
    void DependencyExist();//dependency exist/not exist
    void DependencyIn();//dependency in/not in
    void Filtering();//single table filtering
    void Having();//having clause
    void createHashTable(vector<int> equal_join_index);//create hash table for hash join
    template <typename T>
    bool compareByOperator(int oper, const T& arg1, const T& arg2);
    // bool compareByOperator(int oper, string left_column, string right_column, int left_index, int right_index);
};
