#pragma once

#include "buffer_manager.h"

using namespace std;
using StorageEngineInstance::SnippetRequest;
using StorageEngineInstance::SnippetRequest_Projection;
using StorageEngineInstance::SnippetRequest_Filter;
using StorageEngineInstance::SnippetRequest_Order;
using StorageEngineInstance::SnippetRequest_Dependency;
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
    MergeQueryManager(const SnippetRequest& snippet){
        this->snippet_ = snippet;
        this->snippet_type_ = snippet.type();
        this->group_by_table_.clear();
        this->group_by_key_.clear();
        this->ordered_index_.clear();
    };

    void RunSnippetWork();//스니펫 작업 수행

    inline const static std::string LOGTAG = "Merging::Merge Query Manager";

private:
    SnippetRequest snippet_;//스니펫
    int snippet_type_;//스니펫 작업 타입
    int table_count_;//작업 대상 테이블 개수
    bool is_groupby_;//그룹바이 여부
    bool is_orderby_;//오더바이 여부
    bool is_having_;//해빙절 여부
    bool is_limit_;//리미트 여부
    TableData left_table_;//table1(좌항)
    TableData right_table_;//table2(우항)
    TableData target_table_;//프로젝션 대상 테이블(left_table X right_table)
    unordered_map<string,vector<int>> hash_table_;//key:index 해시 조인 시(left, right 로우가 적은 쪽)
    map<string,int> group_by_key_;//<그룹바이기준컬럼,그룹바이 테이블 인덱스>
    vector<TableData> group_by_table_;//그룹바이된 테이블 컬럼(group by)
    TableData result_table_;//결과 테이블(projected)->버퍼저장
    TableData having_table_;//결과 테이블에 해빙절 수행 테이블
    vector<int> ordered_index_;//result_table의 정렬 후 인덱스 순서
    TableData order_by_table_;//결과 테이블(ordered)->버퍼저장

private:
    void Aggregation(TableData &aggregation_table, const RepeatedPtrField<SnippetRequest_Projection>& projections, const RepeatedPtrField<string>& alias, TableData &dest);
    T Projection(TableData &aggregation_table, SnippetRequest_Projection projection, int rowIndex);//프로젝션 수행,Aggregation 호출
    T Postfix(TableData &aggregation_table, SnippetRequest_Projection projection, int rowIndex, int start, int end);//실제 postfix 계산
    void GroupBy(TableData &groupby_table, const RepeatedPtrField<string>& groups, vector<TableData> &dest);//group by, 그룹바이 절이 있으면 수행
    void OrderBy(TableData &orderby_table, const SnippetRequest_Order& orders, TableData &dest);//order by, 오더바이 절이 있으면 수행
    void InnerJoin_hash();//inner join (hash join + nested loop join)
    void LeftOuterJoin_hash();//left outer join (hash join)
    void RightOuterJoin_hash();//right outer join (hash join)
    void CrossJoin();//cartesian product
    void Union();//left and right table union
    void In();//non dependency in
    void DependencyInnerJoin();//dependency join
    void DependencyExist();
    void DependencyIn();
    void Filtering(TableData &filter_table, const RepeatedPtrField<SnippetRequest_Filter>& filters, TableData &dest);//single table filtering
    void createHashTable(TableData &table, vector<string> equal_join_column);//create hash table for hash join
    template <typename T,typename U>
    bool compareByOperator(int oper, const T& arg1, const U& arg2);
    string makeGroupbyKey(TableData &groupby_table, const RepeatedPtrField<string>& groups, int row_index);

    void InnerJoin_nestedloop();//inner join (nested loop join)
    // void LeftOuterJoin_nestedloop();//left outer join (nested loop join)

    inline void debug_table(int flag){
        if(KETILOG::IsLogLevelUnder(TRACE)){
            if(flag == 1){
                cout << "<left table>" << endl;
                for(auto i : left_table_.table_data){
                    if(i.second.type == TYPE_STRING){
                        cout << i.first << "|" << i.second.strvec.size() << "|" << i.second.type << endl;
                    }else if(i.second.type == TYPE_INT){
                        cout << i.first << "|" << i.second.intvec.size() << "|" << i.second.type << endl;
                    }else if(i.second.type == TYPE_FLOAT){
                        cout << i.first << "|" << i.second.floatvec.size() << "|" << i.second.type << endl;
                    }else if(i.second.type == TYPE_EMPTY){
                        cout << i.first << "|" << "empty row" << endl;
                    }else{
                        cout << "target table row else ?" << endl;
                    }
                }  
            }else if(flag == 2){
                cout << "<right table>" << endl;
                for(auto i : right_table_.table_data){
                    if(i.second.type == TYPE_STRING){
                        cout << i.first << "|" << i.second.strvec.size() << "|" << i.second.type << endl;
                    }else if(i.second.type == TYPE_INT){
                        cout << i.first << "|" << i.second.intvec.size() << "|" << i.second.type << endl;
                    }else if(i.second.type == TYPE_FLOAT){
                        cout << i.first << "|" << i.second.floatvec.size() << "|" << i.second.type << endl;
                    }else if(i.second.type == TYPE_EMPTY){
                        cout << i.first << "|" << "empty row" << endl;
                    }else{
                        cout << "target table row else ?" << endl;
                    }
                }  
            }else if(flag == 3){
                cout << "<target table>" << endl;
                for(auto i : target_table_.table_data){
                    if(i.second.type == TYPE_STRING){
                        cout << i.first << "|" << i.second.strvec.size() << "|" << i.second.type << endl;
                        // cout << i.first << "|" << i.second.strvec[0] << endl;
                    }else if(i.second.type == TYPE_INT){
                        cout << i.first << "|" << i.second.intvec.size() << "|" << i.second.type << endl;
                        // cout << i.first << "|" << i.second.intvec[0] << endl;
                    }else if(i.second.type == TYPE_FLOAT){
                        cout << i.first << "|" << i.second.floatvec.size() << "|" << i.second.type << endl;
                        // cout << i.first << "|" << i.second.floatvec[0] << endl;
                    }else if(i.second.type == TYPE_EMPTY){
                        cout << i.first << "|" << "empty row" << endl;
                    }else{
                        cout << "target table row else ?" << endl;
                    }
                }
            }else if(flag == 4){
                cout << "<group by table>" << endl;
                for(int j = 0; j < group_by_table_.size(); j++){
                    for(auto i : group_by_table_[j].table_data){
                        if(i.second.type == TYPE_STRING){
                            cout << i.first << "|" << i.second.strvec.size() << "|" << i.second.type << endl;
                            // cout << i.first << "|" << i.second.strvec[0] << endl;
                        }else if(i.second.type == TYPE_INT){
                            cout << i.first << "|" << i.second.intvec.size() << "|" << i.second.type << endl;
                            // cout << i.first << "|" << i.second.intvec[0] << endl;
                        }else if(i.second.type == TYPE_FLOAT){
                            cout << i.first << "|" << i.second.floatvec.size() << "|" << i.second.type << endl;
                            // cout << i.first << "|" << i.second.floatvec[0] << endl;
                        }else if(i.second.type == TYPE_EMPTY){
                            cout << i.first << "|" << "empty row" << "|" << i.second.type << endl;
                        }else{
                        cout << "target table row else ?" << endl;
                        }
                    }
                    cout << "--" << endl;
                }                
            }else if(flag == 5){
                cout << "<result table>" << endl;
                for(auto i : result_table_.table_data){
                    if(i.second.type == TYPE_STRING){
                        cout << i.first << "|" << i.second.strvec.size() << "|" << i.second.type << endl;
                        // cout << i.first << "|" << i.second.strvec[0] << endl;
                    }else if(i.second.type == TYPE_INT){
                        cout << i.first << "|" << i.second.intvec.size() << "|" << i.second.type << endl;
                        // cout << i.first << "|" << i.second.intvec[0] << endl;
                    }else if(i.second.type == TYPE_FLOAT){
                        cout << i.first << "|" << i.second.floatvec.size() << "|" << i.second.type << endl;
                        // cout << i.first << "|" << i.second.floatvec[0] << endl;
                    }else if(i.second.type == TYPE_EMPTY){
                        cout << i.first << "|" << "empty row" << "|" << i.second.type << endl;
                    }else{
                    cout << "target table row else ?" << endl;
                    }
                }
            }else if(flag == 6){
                cout << "<final table>" << endl;
                for(auto i : result_table_.table_data){
                    if(i.second.type == TYPE_STRING){
                        cout << i.first << "|" << i.second.strvec.size() << "|" << i.second.type << endl;
                        // cout << i.first << "|" << i.second.strvec[0] << endl;
                    }else if(i.second.type == TYPE_INT){
                        cout << i.first << "|" << i.second.intvec.size() << "|" << i.second.type << endl;
                        // cout << i.first << "|" << i.second.intvec[0] << endl;
                    }else if(i.second.type == TYPE_FLOAT){
                        cout << i.first << "|" << i.second.floatvec.size() << "|" << i.second.type << endl;
                        // cout << i.first << "|" << i.second.floatvec[0] << endl;
                    }else if(i.second.type == TYPE_EMPTY){
                        cout << i.first << "|" << "empty row" << "|" << i.second.type << endl;
                    }else{
                        cout << "target table row else ?" << endl;
                    }
                }
            }
        }
    }
};
