#pragma once

#include "BufferManager.h"

using namespace std;
using StorageEngineInstance::Snippet_Projection;
using StorageEngineInstance::Snippet_Filter;
using StorageEngineInstance::Snippet_Order;
using StorageEngineInstance::Snippet_Dependency;
using google::protobuf::RepeatedPtrField;

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
    int type;//0 empty, 1 string, 2 int, 3 float, 4 boolean (KETI_VECTOR_TYPE)
    
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
        this->group_by_table_.clear();
        this->group_by_key_.clear();
        this->ordered_index_.clear();
    };

    void RunSnippetWork();//스니펫 작업 수행

    inline const static std::string LOGTAG = "Merging Container::Merge Query Manager";

private:
    StorageEngineInstance::Snippet snippet;//스니펫
    int snippetType;//스니펫 작업 타입
    int tableCnt;//작업 대상 테이블 개수
    bool isGroupby;//그룹바이 여부
    bool isOrderby;//오더바이 여부
    bool isHaving;//해빙절 여부
    bool isLimit;//리미트 여부
    TableData left_table_;//table1(좌항)
    TableData right_table_;//table2(우항)
    unordered_map<string,vector<int>> hash_table_;//key:index 해시 조인 시(left, right 로우가 적은 쪽)
    map<string,int> group_by_key_;//<그룹바이기준컬럼,그룹바이 테이블 인덱스>
    vector<TableData> group_by_table_;//그룹바이된 테이블 컬럼(group by)
    TableData result_table_;//결과 테이블(projected)->버퍼저장
    TableData having_table_;//결과 테이블에 해빙절 수행 테이블
    vector<int> ordered_index_;//result_table의 정렬 후 인덱스 순서
    TableData order_by_table_;//결과 테이블(ordered)->버퍼저장

private:
    // void Aggregation();//column projection, 모든 스니펫이 기본적으로 수행
    void Aggregation(TableData &aggregation_table, const RepeatedPtrField<Snippet_Projection>& projections, const RepeatedPtrField<string>& alias, TableData &dest);//column projection, 모든 스니펫이 기본적으로 수행
    T Projection(TableData &aggregation_table, Snippet_Projection projection, int rowIndex);//프로젝션 수행,Aggregation 호출
    T Postfix(TableData &aggregation_table, Snippet_Projection projection, int rowIndex, int start, int end);//실제 postfix 계산
    void GroupBy(TableData &groupby_table, const RepeatedPtrField<string>& groups, vector<TableData> &dest);//group by, 그룹바이 절이 있으면 수행
    void OrderBy(TableData &orderby_table, const Snippet_Order& orders, TableData &dest);//order by, 오더바이 절이 있으면 수행
    void InnerJoin_hash(TableData &left_table, TableData &right_table, const RepeatedPtrField<Snippet_Filter>& filters, TableData &dest);//inner join (hash join + nested loop join)
    void LeftOuterJoin_hash(TableData &left_table, TableData &right_table, const RepeatedPtrField<Snippet_Filter>& filters, TableData &dest);//left outer join (hash join)
    void RightOuterJoin_hash(TableData &left_table, TableData &right_table, const RepeatedPtrField<Snippet_Filter>& filters, TableData &dest);//right outer join (hash join)
    void CrossJoin(TableData &left_table, TableData &right_table, const RepeatedPtrField<Snippet_Filter>& filters, TableData &dest);//cartesian product
    void Union(TableData &left_table, TableData &right_table, const RepeatedPtrField<Snippet_Filter>& filters, TableData &dest);//left and right table union
    void In(TableData &left_table, TableData &right_table, const RepeatedPtrField<Snippet_Filter>& filters, TableData &dest);//non dependency in
    void DependencyInnerJoin(TableData &left_table, TableData &right_table, const RepeatedPtrField<Snippet_Filter>& filters, const Snippet_Dependency &dependency, TableData &dest);//dependency join
    void DependencyExist(TableData &left_table, TableData &right_table, const RepeatedPtrField<Snippet_Filter>& filters, const Snippet_Dependency &dependency, TableData &dest);//dependency exist/not exist
    void DependencyIn(TableData &left_table, TableData &right_table, const RepeatedPtrField<Snippet_Filter>& filters, const Snippet_Dependency &dependency, TableData &dest);//dependency in/not in
    void Filtering(TableData &filter_table, const RepeatedPtrField<Snippet_Filter>& filters, TableData &dest);//single table filtering
    void createHashTable(TableData &table, vector<string> equal_join_column);//create hash table for hash join
    // template <typename T>
    // bool compareByOperator(int oper, const T& arg1, const T& arg2);
    template <typename T,typename U>
    bool compareByOperator(int oper, const T& arg1, const U& arg2);
    string makeGroupbyKey(TableData &groupby_table, const RepeatedPtrField<string>& groups, int row_index);

    void InnerJoin_nestedloop(TableData &left_table, TableData &right_table, const RepeatedPtrField<Snippet_Filter>& filters, TableData &dest);//inner join (nested loop join)
    void LeftOuterJoin_nestedloop();//left outer join (nested loop join)
};
