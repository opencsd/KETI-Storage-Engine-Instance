#include "merge_query_manager.h"
#include <chrono>

using StorageEngineInstance::SnippetRequest_Order_OrderDirection_ASC;
using StorageEngineInstance::SnippetRequest_Order_OrderDirection_DESC;
using StorageEngineInstance::SnippetRequest_ValueType_STRING;
using StorageEngineInstance::SnippetRequest_Filter_OperType;

void MergeQueryManager::RunSnippetWork(){
    BufferManager::InitializeBuffer(snippet_.query_id(),snippet_.work_id(),snippet_.result_info().table_alias());

    table_count_ = snippet_.query_info().table_name_size();
    is_groupby_ = (snippet_.query_info().group_by_size() == 0) ? false : true;
    is_orderby_ = (snippet_.query_info().order_by().column_name_size() == 0) ? false : true;
    is_having_ = (snippet_.query_info().having_size() == 0) ? false : true;
    is_limit_ = (snippet_.query_info().limit().length() != 0) ? true : false;
    
    cout << "start merging " << snippet_.query_id() << " " << snippet_.work_id() << endl;
    
    //Do snippet work -> (Make "hash_table") -> Make "target_table"
    switch(snippet_type_){
        case StorageEngineInstance::SnippetRequest_SnippetType_AGGREGATION:{
            target_table_ = BufferManager::GetFinishedTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(0));
            break;
        }case StorageEngineInstance::SnippetRequest_SnippetType_FILTER:{
            Filtering(left_table_, snippet_.query_info().filtering(), target_table_);
            break;
        }case StorageEngineInstance::SnippetRequest_SnippetType_INNER_JOIN:{
            InnerJoin_hash();
            break;
        }case StorageEngineInstance::SnippetRequest_SnippetType_LEFT_OUTER_JOIN:{
            // LeftOuterJoin_nestedloop();
            LeftOuterJoin_hash();
            break;
        }case StorageEngineInstance::SnippetRequest_SnippetType_RIGHT_OUTER_JOIN:{
            RightOuterJoin_hash();
            break;
        }case StorageEngineInstance::SnippetRequest_SnippetType_CROSS_JOIN:{
            CrossJoin();
            break;
        }case StorageEngineInstance::SnippetRequest_SnippetType_UNION:{
            Union();       
            break;
        }case StorageEngineInstance::SnippetRequest_SnippetType_IN:{
            In();
            break;
        }case StorageEngineInstance::SnippetRequest_SnippetType_DEPENDENCY_INNER_JOIN:{
            DependencyInnerJoin();
            break;
        }case StorageEngineInstance::SnippetRequest_SnippetType_DEPENDENCY_EXIST:{
            DependencyExist();
            break;
        }case StorageEngineInstance::SnippetRequest_SnippetType_DEPENDENCY_IN:{
            DependencyIn();
            break;
        }
    }

    left_table_.table_data.clear();
    right_table_.table_data.clear();
    hash_table_.clear();

    debug_table(3);

    if(is_groupby_){
        GroupBy(target_table_, snippet_.query_info().group_by(), group_by_table_);//GroupBy -> Make "group_by_key" & "group_by_table"
        // debug_table(4);
        
        //Column Projection -> Make "result_table"
        for(int i=0; i<group_by_table_.size(); i++){
            Aggregation(group_by_table_[i], snippet_.query_info().projection(), snippet_.result_info().column_alias(), result_table_);
        }
        group_by_table_.clear();
    }else{
        //Column Projection -> Make "result_table"
        Aggregation(target_table_, snippet_.query_info().projection(), snippet_.result_info().column_alias(), result_table_);
    } 

    target_table_.table_data.clear();

    debug_table(5);

    if(is_having_){
        Filtering(result_table_, snippet_.query_info().having(), having_table_);

        if(is_orderby_){
            //Order By -> Make "ordered_index" & "order_by_table"
            OrderBy(having_table_, snippet_.query_info().order_by(), order_by_table_);
            //Save "order_by_table"
            if(is_limit_){
                BufferManager::SaveTableData(snippet_,order_by_table_,snippet_.query_info().limit().offset(),snippet_.query_info().limit().length());
            }else{
                BufferManager::SaveTableData(snippet_,order_by_table_,0,order_by_table_.row_count);
            }
        }else{
            //Save "result_table"
            if(is_limit_){
                BufferManager::SaveTableData(snippet_,having_table_,snippet_.query_info().limit().offset(),snippet_.query_info().limit().length());
            }else{
                BufferManager::SaveTableData(snippet_,having_table_,0,having_table_.row_count);
            } 
        }
    }else{
        if(is_orderby_){
            //Order By -> Make "ordered_index" & "order_by_table"
            OrderBy(result_table_, snippet_.query_info().order_by(), order_by_table_);
            //Save "order_by_table"
            if(is_limit_){
                BufferManager::SaveTableData(snippet_,order_by_table_,snippet_.query_info().limit().offset(),snippet_.query_info().limit().length());
            }else{
                BufferManager::SaveTableData(snippet_,order_by_table_,0,order_by_table_.row_count);
            }
        }else{
            //Save "result_table"
            if(is_limit_){
                BufferManager::SaveTableData(snippet_,result_table_,snippet_.query_info().limit().offset(),snippet_.query_info().limit().length());
            }else{
                BufferManager::SaveTableData(snippet_,result_table_,0,result_table_.row_count);
            } 
        }
    }

    debug_table(6);

    cout << "merging done " << snippet_.query_id() << " " << snippet_.work_id() << endl;
}

void MergeQueryManager::Aggregation(TableData &aggregation_table, const RepeatedPtrField<SnippetRequest_Projection>& projections, const RepeatedPtrField<string>& alias, TableData &dest){
    //한 프로젝션씩 결과 생성
    int result_row_num = TYPE_EMPTY;
    int target_row_count = aggregation_table.row_count;

    if(aggregation_table.row_count == 0){
        for(int a = 0; a < alias.size(); a++){
            dest.table_data.insert({alias[a], ColData{}});
        }
        dest.row_count = TYPE_EMPTY;
        return;
    }

    for(int p = 0; p < projections.size(); p++){
        ColData colData;//프로젝션 결과 컬럼 벡터

        if(aggregation_table.row_count == 0) continue;
    
        switch(projections[p].select_type()){
            case StorageEngineInstance::SnippetRequest_Projection_SelectType_COLUMNNAME:{
                T t;
                if(is_groupby_){//group by가 있는데 단순 컬럼 선택이면 0번째 로우만 유효
                    t = Projection(aggregation_table,projections[p],0);//p번째 프로젝션을 0번째 로우에 적용
                    switch(t.type){
                        case TYPE_STRING:{
                            colData.strvec.push_back(t.varString);
                            colData.isnull.push_back(false);
                            colData.row_count++;
                            break;
                        }case TYPE_INT:{    
                            colData.intvec.push_back(t.varInt);
                            colData.isnull.push_back(false);
                            colData.row_count++;
                            break;
                        }case TYPE_FLOAT:{
                            colData.floatvec.push_back(t.varFloat);
                            colData.isnull.push_back(false);
                            colData.row_count++;
                            break;
                        }default:{
                            KETILOG::ERRORLOG(LOGTAG,"SnippetRequest_Projection_SelectType_COLUMNNAME1 => check plz..");
                        }
                    }
                    colData.type = t.type;
                }else{//로우 개수만큼
                    for (int r = 0; r < target_row_count; r++){
                        t = Projection(aggregation_table,projections[p],r);//p번째 프로젝션을 r번째 로우에 적용
                        switch(t.type){
                            case TYPE_STRING:{
                                colData.strvec.push_back(t.varString);
                                colData.isnull.push_back(false);
                                colData.row_count++;
                                break;
                            }case TYPE_INT:{    
                                colData.intvec.push_back(t.varInt);
                                colData.isnull.push_back(false);
                                colData.row_count++;
                                break;
                            }case TYPE_FLOAT:{
                                colData.floatvec.push_back(t.varFloat);
                                colData.isnull.push_back(false);
                                colData.row_count++;
                                break;
                            }default:{
                                KETILOG::ERRORLOG(LOGTAG,"SnippetRequest_Projection_SelectType_COLUMNNAME2 => check plz..");
                            }
                        }
                        colData.type = t.type;
                    }
                }
                break;
            }case StorageEngineInstance::SnippetRequest_Projection_SelectType_SUM:{
                T t, t_;
                for (int r = 0; r < target_row_count; r++){
                    t_ = Projection(aggregation_table,projections[p],r);
                    switch(t_.type){
                        case TYPE_INT:{    
                            t.varInt += t_.varInt;
                            break;
                        }case TYPE_FLOAT:{
                            t.varFloat += t_.varFloat;
                            break;
                        }default:{
                            KETILOG::ERRORLOG(LOGTAG,"MergeQueryManager::SnippetRequest_Projection_SelectType_SUM1 => check plz..");
                        }
                    }
                }
                t.type = t_.type;
                switch(t.type){
                    case TYPE_INT:{    
                        colData.intvec.push_back(t.varInt);
                        colData.isnull.push_back(false);
                        colData.row_count++;
                        colData.type = t.type;
                        break;
                    }case TYPE_FLOAT:{
                        colData.floatvec.push_back(t.varFloat);
                        colData.isnull.push_back(false);
                        colData.row_count++;
                        colData.type = t.type;
                        break;
                    }default:{
                        KETILOG::ERRORLOG(LOGTAG,"MergeQueryManager::SnippetRequest_Projection_SelectType_SUM2 => check plz..");
                    }
                }
                break;
            }case StorageEngineInstance::SnippetRequest_Projection_SelectType_AVG:{
                T t, t_;
                for (int r = 0; r < target_row_count; r++){
                    t_ = Projection(aggregation_table,projections[p],r);
                    switch(t_.type){
                        case TYPE_INT:{    
                            t.varInt += t_.varInt;
                            break;
                        }case TYPE_FLOAT:{
                            t.varFloat += t_.varFloat;
                            break;
                        }default:{
                            KETILOG::ERRORLOG(LOGTAG,"MergeQueryManager::SnippetRequest_Projection_SelectType_AVG1 => check plz..");
                        }
                    }
                }
                t.type = t_.type;
                float avg;
                switch(t.type){
                    case TYPE_INT:{    
                        avg = t.varInt / target_row_count;
                        break;
                    }case TYPE_FLOAT:{
                        avg = t.varFloat / target_row_count;
                        break;
                    }default:{
                        KETILOG::ERRORLOG(LOGTAG,"MergeQueryManager::SnippetRequest_Projection_SelectType_AVG2 => check plz..");
                    }
                }
                colData.floatvec.push_back(avg);
                colData.isnull.push_back(false);
                colData.row_count++;
                colData.type = TYPE_FLOAT;
                break;
            }case StorageEngineInstance::SnippetRequest_Projection_SelectType_COUNT:{//해당 컬럼의 null 데이터 제외 로우 개수 카운트
                vector<bool> is_column_null = aggregation_table.table_data[projections[p].value(0)].isnull;
                int count = std::count(is_column_null.begin(), is_column_null.end(), false);
                colData.intvec.push_back(count);
                colData.isnull.push_back(false);
                colData.row_count++;
                colData.type = TYPE_INT;
                break;
            }case StorageEngineInstance::SnippetRequest_Projection_SelectType_COUNTSTAR:{//로우 개수 카운트
                colData.intvec.push_back(target_row_count);
                colData.isnull.push_back(false);
                colData.row_count++;
                colData.type = TYPE_INT;
                break;
            }case StorageEngineInstance::SnippetRequest_Projection_SelectType_COUNTDISTINCT:{
                int distinct_row_count = 0;
                switch(aggregation_table.table_data[projections[p].value(0)].type){
                    case TYPE_INT:{    
                        std::set<int> uniqueElements(aggregation_table.table_data[projections[p].value(0)].intvec.begin(), aggregation_table.table_data[projections[p].value(0)].intvec.end());
                        distinct_row_count = uniqueElements.size();
                        break;
                    }case TYPE_FLOAT:{
                        std::set<float> uniqueElements(aggregation_table.table_data[projections[p].value(0)].floatvec.begin(), aggregation_table.table_data[projections[p].value(0)].floatvec.end());
                        distinct_row_count = uniqueElements.size();
                        break;
                    }case TYPE_STRING:{
                        std::set<string> uniqueElements(aggregation_table.table_data[projections[p].value(0)].strvec.begin(), aggregation_table.table_data[projections[p].value(0)].strvec.end());
                        distinct_row_count = uniqueElements.size();
                        break;
                    }default:{
                        KETILOG::ERRORLOG(LOGTAG,"MergeQueryManager::SnippetRequest_Projection_SelectType_COUNTDISTINCT => check plz..");
                    }
                }
                colData.intvec.push_back(distinct_row_count);
                colData.isnull.push_back(false);
                colData.row_count++;
                colData.type = TYPE_INT;
                break;
            }case StorageEngineInstance::SnippetRequest_Projection_SelectType_TOP:{
                KETILOG::ERRORLOG(LOGTAG,"MergeQueryManager::SnippetRequest_Projection_SelectType_TOP => check plz.. ");
                break;
            }case StorageEngineInstance::SnippetRequest_Projection_SelectType_MIN:{
                T t, t_;
                t = Projection(aggregation_table,projections[p],0);//맨 처음 로우를 최소값으로 저장

                for (int r = 1; r < target_row_count; r++){
                    t_ = Projection(aggregation_table,projections[p],r);
                    switch(t.type){
                        case TYPE_INT:{    
                            t.varInt = (t.varInt < t_.varInt) ? t.varInt : t_.varInt;
                            break;
                        }case TYPE_FLOAT:{
                            t.varFloat = (t.varFloat < t_.varFloat) ? t.varFloat : t_.varFloat;
                            break;
                        }case TYPE_STRING:{
                            t.varString = (t.varString < t_.varString) ? t.varString : t_.varString;
                            break;
                        }default:{
                            KETILOG::ERRORLOG(LOGTAG,"MergeQueryManager::SnippetRequest_Projection_SelectType_MIN1 => check plz..");
                        }
                    }
                }
                
                switch(t.type){
                    case TYPE_INT:{    
                        colData.intvec.push_back(t.varInt);
                        colData.isnull.push_back(false);
                        colData.row_count++;
                        break;
                    }case TYPE_FLOAT:{
                        colData.floatvec.push_back(t.varFloat);
                        colData.isnull.push_back(false);
                        colData.row_count++;
                        break;
                    }case TYPE_STRING:{
                        colData.strvec.push_back(t.varString);
                        colData.isnull.push_back(false);
                        colData.row_count++;
                        break;
                    }default:{
                        KETILOG::ERRORLOG(LOGTAG,"MergeQueryManager::SnippetRequest_Projection_SelectType_MIN2 => check plz..");
                    }
                }
                colData.type = t.type;
                break;
            }case StorageEngineInstance::SnippetRequest_Projection_SelectType_MAX:{
                T t, t_;
                t_ = Projection(aggregation_table,projections[p],0);//맨 처음 로우를 최대값으로 저장
                if(t_.type == TYPE_INT){
                    t.varInt = t_.varInt;
                    t.type = TYPE_INT;
                }else if(t_.type == TYPE_FLOAT){
                    t.varFloat = t_.varFloat;
                    t.type = TYPE_FLOAT;
                }else{
                    KETILOG::ERRORLOG(LOGTAG,"MergeQueryManager::SnippetRequest_Projection_SelectType_MAX0 => check plz..");
                }
                for (int r = 1; r < target_row_count; r++){
                    t_ = Projection(aggregation_table,projections[p],r);
                    switch(t.type){
                        case TYPE_INT:{    
                            t.varInt = (t.varInt > t_.varInt) ? t.varInt : t_.varInt;
                            break;
                        }case TYPE_FLOAT:{
                            t.varFloat = (t.varFloat > t_.varFloat) ? t.varFloat : t_.varFloat;
                            break;
                        }case TYPE_STRING:{
                            t.varString = (t.varString > t_.varString) ? t.varString : t_.varString;
                            break;
                        }default:{
                            KETILOG::ERRORLOG(LOGTAG,"MergeQueryManager::SnippetRequest_Projection_SelectType_MAX1 => check plz..");
                        }
                    }
                }
                switch(t.type){
                    case TYPE_INT:{    
                        colData.intvec.push_back(t.varInt);
                        colData.isnull.push_back(false);
                        colData.row_count++;
                        break;
                    }case TYPE_FLOAT:{
                        colData.floatvec.push_back(t.varFloat);
                        colData.isnull.push_back(false);
                        colData.row_count++;
                        break;
                    }case TYPE_STRING:{
                        colData.strvec.push_back(t.varString);
                        colData.isnull.push_back(false);
                        colData.row_count++;
                        break;
                    }default:{
                        KETILOG::ERRORLOG(LOGTAG,"MergeQueryManager::SnippetRequest_Projection_SelectType_MAX2 => check plz..");
                    }
                }
                colData.type = t_.type;
                break;
            }
        }

        switch(colData.type){
        case TYPE_INT:{    
            dest.table_data[alias[p]].intvec.insert(dest.table_data[alias[p]].intvec.end(), colData.intvec.begin(), colData.intvec.end());
            break;
        }case TYPE_FLOAT:{
            dest.table_data[alias[p]].floatvec.insert(dest.table_data[alias[p]].floatvec.end(), colData.floatvec.begin(), colData.floatvec.end());
            break;
        }case TYPE_STRING:{
            dest.table_data[alias[p]].strvec.insert(dest.table_data[alias[p]].strvec.end(), colData.strvec.begin(), colData.strvec.end());
            break;
        }default:{
            KETILOG::ERRORLOG(LOGTAG,"MergeQueryManager::Aggregation => check plz..");
        }
        }

        dest.table_data[alias[p]].isnull.insert(dest.table_data[alias[p]].isnull.end(), colData.isnull.begin(), colData.isnull.end());
        dest.table_data[alias[p]].row_count += colData.row_count;
        dest.table_data[alias[p]].type = colData.type;

        if(result_row_num == TYPE_EMPTY){//첫번째 컬럼을 처리하고 저장
            result_row_num = colData.row_count;
        }
    }

    dest.row_count += result_row_num;
}

T MergeQueryManager::Projection(TableData &aggregation_table, SnippetRequest_Projection projection, int rowIndex){
    T postfixResult;
    string first_operator = projection.value(0);

    if(first_operator == "CASE"){ //CASE WHEN THEN
        int case_len = projection.value_size();
        vector<pair<int,int>> when_then_offset;
        int else_offset, when_offset, then_offset = 0;
        bool passed = false;

        for(int i = 1; i<case_len; i++){
            if(projection.value(i) == "WHEN"){
                when_offset = i;
            }else if(projection.value(i) == "THEN"){
                then_offset = i;
                when_then_offset.push_back({when_offset, then_offset});
            }else if(projection.value(i) == "ELSE"){
                else_offset = i;
            }
        }

        T t;
        vector<pair<int,int>>::iterator iter;
        for (iter = when_then_offset.begin(); iter != when_then_offset.end(); ++iter){
            when_offset = (*iter).first;
            then_offset = (*iter).second;
            int offset = when_offset + 1;
            int start = offset;
            
            while(offset < then_offset + 1){
                if(projection.value(offset) == "AND"){
                    t = Postfix(aggregation_table, projection, rowIndex, start, offset);
                    if(t.boolean){
                        offset++;
                        start = offset;
                    }else{
                        passed = false;
                        break;
                    }
                }else if(projection.value(offset) == "OR"){
                    t = Postfix(aggregation_table, projection, rowIndex, start, offset);
                    if(t.boolean){
                        passed = true;
                        break;
                    }else{
                        offset++;
                        start = offset;
                    } 
                }else if(projection.value(offset) == "THEN"){
                    t = Postfix(aggregation_table, projection, rowIndex, start, offset);
                    if(t.boolean){
                        passed = true;
                        break;
                    }else{
                        offset++;
                    }
                }else{
                    offset++;
                }
            }

            offset = then_offset + 1;

            if(passed){//THEN
                start = offset;
                while(true){
                    if(projection.value(offset) == "WHEN" || 
                        projection.value(offset) == "ELSE" || 
                        projection.value(offset) == "END"){
                        break;
                    }
                    offset++;
                }
                t = Postfix(aggregation_table, projection, rowIndex, start, offset);

                switch (t.type){
                case TYPE_INT:
                    postfixResult.varInt = t.varInt;
                    break;
                case TYPE_STRING:
                    postfixResult.varString =t.varString;
                    break;
                case TYPE_FLOAT:
                    postfixResult.varFloat = t.varFloat;
                    break;
                default:
                    KETILOG::ERRORLOG(LOGTAG,"else type check plz.. "+to_string(t.type));
                    break;
                }
                postfixResult.isnull = false;
                postfixResult.type = t.type;

                break;
            }
        }

        if(!passed){//ELSE
            t = Postfix(aggregation_table, projection, rowIndex, else_offset+1, projection.value_size()-1);

            switch (t.type){
            case TYPE_INT:
                postfixResult.varInt = t.varInt;
                break;
            case TYPE_STRING:
                postfixResult.varString =t.varString;
                break;
            case TYPE_FLOAT:
                postfixResult.varFloat = t.varFloat;
                break;
            default:
                KETILOG::ERRORLOG(LOGTAG,"else type check plz.. "+to_string(t.type));
                break;
            }
            postfixResult.isnull = false;
            postfixResult.type = t.type;
        }
    }else if(first_operator == "SUBSTRING"){//["SUBSTRING",col_name,start,length],
        string src = projection.value(1);
        int type = projection.value_type(1);
        int start = stoi(projection.value(2));
        int length = stoi(projection.value(3));
        string value;

        if(type == StorageEngineInstance::SnippetRequest_ValueType_COLUMN){
            value = aggregation_table.table_data[src].strvec[rowIndex];
        }else if(type == StorageEngineInstance::SnippetRequest_ValueType_STRING){
            value = src;
        }else{
            KETILOG::ERRORLOG(LOGTAG,"SUBSTRING => check plz..");
        }

        postfixResult.varString = value.substr(start,length);;
        postfixResult.isnull = false;
        postfixResult.type = TYPE_STRING;
    }else if(first_operator == "EXTRACT"){//["EXTRACT","YEAR",col_name]
        string unit = projection.value(1);
        string col = projection.value(2);
        int type = projection.value_type(2);
        int value, result;

        if(type == StorageEngineInstance::SnippetRequest_ValueType_COLUMN){
            value = aggregation_table.table_data[col].intvec[rowIndex];
        }else{
            KETILOG::ERRORLOG(LOGTAG,"Extract1 => check plz..");
        }

        if(unit == "YEAR"){
            result = value / 512;
        }else if(unit == "MONTH"){
            value %= 512;
            result = value / 32;
        }else if(unit == "DAY"){
            value = value % 512;
            value %= 32;
            result = value;
        }else{
            KETILOG::ERRORLOG(LOGTAG,"Extract2 => check plz..");
        }

        postfixResult.varInt = result;
        postfixResult.isnull = false;
        postfixResult.type = TYPE_INT;
    }else{ //POSTFIX
        postfixResult = Postfix(aggregation_table, projection, rowIndex, 0, projection.value_size());
    }
    return postfixResult;
}

T MergeQueryManager::Postfix(TableData &aggregation_table, SnippetRequest_Projection projection, int rowIndex, int start, int end){
    T postfixResult;
    string first_operator = projection.value(0);

    stack<T> oper_stack;
    for (int i = start; i < end; i++){
        T t;
        switch(projection.value_type(i)){
            case StorageEngineInstance::SnippetRequest_ValueType_COLUMN:{
                if(aggregation_table.table_data[projection.value(i)].type == TYPE_STRING){
                    t.varString = aggregation_table.table_data[projection.value(i)].strvec[rowIndex];
                    t.type = TYPE_STRING;
                    oper_stack.push(t);
                }else if(aggregation_table.table_data[projection.value(i)].type == TYPE_INT){
                    t.varInt = aggregation_table.table_data[projection.value(i)].intvec[rowIndex];
                    t.type = TYPE_INT;
                    oper_stack.push(t);
                }else if(aggregation_table.table_data[projection.value(i)].type == TYPE_FLOAT){
                    t.varFloat = aggregation_table.table_data[projection.value(i)].floatvec[rowIndex];
                    t.type = TYPE_FLOAT;
                    oper_stack.push(t);
                }else{
                    KETILOG::ERRORLOG(LOGTAG,"Postfix type check plz... (a) " + to_string(aggregation_table.table_data[projection.value(i)].type));
                }
                break;
            }case StorageEngineInstance::SnippetRequest_ValueType_OPERATOR:{
                if(projection.value(i) == "+"){
                    T oper2 = oper_stack.top();
                    oper_stack.pop();
                    T oper1 = oper_stack.top();
                    oper_stack.pop();
                    if(oper1.type == TYPE_INT){
                        t.varInt = oper1.varInt + oper2.varInt;
                        t.type = TYPE_INT;
                        oper_stack.push(t);
                    }else if(oper1.type == TYPE_FLOAT){
                        t.varFloat = oper1.varFloat + oper2.varFloat;
                        t.type = TYPE_FLOAT;
                        oper_stack.push(t);
                    }else{
                        KETILOG::ERRORLOG(LOGTAG,"Postfix type check plz... (b)");
                    }
                }else if(projection.value(i) == "-"){
                    T oper2 = oper_stack.top();
                    oper_stack.pop();
                    T oper1 = oper_stack.top();
                    oper_stack.pop();
                    if(oper1.type == TYPE_INT){
                        if (oper2.type == TYPE_INT){
                            t.varInt = oper1.varInt - oper2.varInt;
                            t.type = TYPE_INT;
                            oper_stack.push(t);
                        }else if(oper2.type == TYPE_FLOAT){
                            t.varFloat = oper1.varInt - oper2.varFloat;
                            t.type = TYPE_FLOAT;
                            oper_stack.push(t);
                        }else{
                            KETILOG::ERRORLOG(LOGTAG,"Postfix type check plz... (c)");
                        }
                    }else if(oper1.type == TYPE_FLOAT){
                        if (oper2.type == TYPE_INT){
                            t.varInt = oper1.varFloat - oper2.varInt;
                            t.type = TYPE_FLOAT;
                            oper_stack.push(t);
                        }else if(oper2.type == TYPE_FLOAT){
                            t.varFloat = oper1.varFloat - oper2.varFloat;
                            t.type = TYPE_FLOAT;
                            oper_stack.push(t);
                        }else{
                            KETILOG::ERRORLOG(LOGTAG,"Postfix type check plz... (d)");
                        }
                    }
                }else if(projection.value(i) == "*"){
                    T oper2 = oper_stack.top();
                    oper_stack.pop();
                    T oper1 = oper_stack.top();
                    oper_stack.pop();
                    if(oper1.type == TYPE_INT){
                        if (oper2.type == TYPE_INT){
                            t.varInt = oper1.varInt * oper2.varInt;
                            t.type = TYPE_INT;
                            oper_stack.push(t);
                        }else if(oper2.type == TYPE_FLOAT){
                            t.varFloat = oper1.varInt * oper2.varFloat;
                            t.type = TYPE_FLOAT;
                            oper_stack.push(t);
                        }else{
                            KETILOG::ERRORLOG(LOGTAG,"Postfix type check plz... (e)");
                        }
                    }else if(oper1.type == TYPE_FLOAT){
                        if (oper2.type == TYPE_INT){
                            t.varInt = oper1.varFloat * oper2.varInt;
                            t.type = TYPE_FLOAT;
                            oper_stack.push(t);
                        }else if(oper2.type == TYPE_FLOAT){
                            t.varFloat = oper1.varFloat * oper2.varFloat;
                            t.type = TYPE_FLOAT;
                            oper_stack.push(t);
                        }else{
                            KETILOG::ERRORLOG(LOGTAG,"Postfix type check plz... (f)");
                        }
                    }
                }else if(projection.value(i) == "/"){
                    T oper2 = oper_stack.top();
                    oper_stack.pop();
                    T oper1 = oper_stack.top();
                    oper_stack.pop();
                    if(oper1.type == TYPE_INT){
                        if (oper2.type == TYPE_INT){
                            t.varInt = oper1.varInt / oper2.varInt;
                            t.type = TYPE_INT;
                            oper_stack.push(t);
                        }else if(oper2.type == TYPE_FLOAT){
                            t.varFloat = oper1.varInt / oper2.varFloat;
                            t.type = TYPE_FLOAT;
                            oper_stack.push(t);
                        }else{
                            KETILOG::ERRORLOG(LOGTAG,"Postfix type check plz... (g)");
                        }
                    }else if(oper1.type == TYPE_FLOAT){
                        if (oper2.type == TYPE_INT){
                            t.varInt = oper1.varFloat / oper2.varInt;
                            t.type = TYPE_FLOAT;
                            oper_stack.push(t);
                        }else if(oper2.type == TYPE_FLOAT){
                            t.varFloat = oper1.varFloat / oper2.varFloat;
                            t.type = TYPE_FLOAT;
                            oper_stack.push(t);
                        }else{
                            KETILOG::ERRORLOG(LOGTAG,"Postfix type check plz... (h)");
                        }
                    }else{
                        KETILOG::ERRORLOG(LOGTAG,"SnippetRequest_ValueType_OPERATOR '<>' => check plz..");
                    }
                }else if(projection.value(i) == "="){
                    T oper2 = oper_stack.top();
                    oper_stack.pop();
                    T oper1 = oper_stack.top();
                    oper_stack.pop();
                    if(oper1.type == TYPE_STRING && oper2.type == TYPE_STRING){
                        t.boolean = (oper1.type == oper2.type);
                    }else{
                        KETILOG::ERRORLOG(LOGTAG,"SnippetRequest_ValueType_OPERATOR '=' => check plz..");
                    }
                    oper_stack.push(t);
                }else if(projection.value(i) == "<>"){
                    T oper2 = oper_stack.top();
                    oper_stack.pop();
                    T oper1 = oper_stack.top();
                    oper_stack.pop();
                    if(oper1.type == TYPE_STRING && oper2.type == TYPE_STRING){
                        t.boolean = (oper1.type != oper2.type);
                    }else{
                        KETILOG::ERRORLOG(LOGTAG,"SnippetRequest_ValueType_OPERATOR '<>' => check plz..");
                    }
                    oper_stack.push(t);
                }else if(projection.value(i) == "LIKE"){//%A%B%... 케이스 모두 체크 필요
                    T oper2 = oper_stack.top();
                    oper_stack.pop();
                    T oper1 = oper_stack.top();
                    oper_stack.pop();
                    if(oper1.type == TYPE_STRING && oper2.type == TYPE_STRING){
                        string col = oper1.varString;
                        string str = oper2.varString;
                        if(str.substr(0,1) == "%" && str.substr(str.length()-1) == "%"){//'%str%'
                            string find_str = str.substr(1,str.length()-2);
                            t.boolean = (col.find(find_str) != string::npos) ? true : false;
                        }else if(str.substr(0,1) == "%"){//'%str'
                            string find_str = str.substr(1);
                            string comp = col.substr(col.length()-find_str.length());
                            t.boolean = (find_str == comp) ? true : false;
                        }else if(str.substr(str.length()-1) == "%"){//'str%'
                            string find_str = str.substr(0,str.length()-1);
                            string comp = col.substr(0,find_str.length());
                            t.boolean = (find_str == comp) ? true : false;
                        }else{
                            t.boolean = (col == str) ? true : false;
                        }
                        oper_stack.push(t);
                    }else{
                        KETILOG::ERRORLOG(LOGTAG,"SnippetRequest_ValueType_OPERATOR 'LIKE' => check plz..");
                    }
                }else{
                    KETILOG::ERRORLOG(LOGTAG,"Projection => check plz..");
                }
                break;
            }case StorageEngineInstance::SnippetRequest_ValueType_FLOAT32:
             case StorageEngineInstance::SnippetRequest_ValueType_DOUBLE:
             case StorageEngineInstance::SnippetRequest_ValueType_DECIMAL:{
                t.varFloat = stod(projection.value(i));
                t.type = TYPE_FLOAT;
                oper_stack.push(t);
                break;
            }case StorageEngineInstance::SnippetRequest::INT16:
             case StorageEngineInstance::SnippetRequest::INT32:
             case StorageEngineInstance::SnippetRequest::INT64:{
                t.varInt = stoi(projection.value(i));
                t.type = TYPE_INT;
                oper_stack.push(t);
                break;
            }case StorageEngineInstance::SnippetRequest_ValueType_STRING:{
                t.varString = projection.value(i);
                t.type = TYPE_STRING;
                oper_stack.push(t);
                break;
            }default:{
                KETILOG::ERRORLOG(LOGTAG,"projection defualt -> check plz..");
            }
        }
    }
    postfixResult = oper_stack.top();
    oper_stack.pop();
    
    return postfixResult;
}

string MergeQueryManager::makeGroupbyKey(TableData &groupby_table, const RepeatedPtrField<string>& groups, int row_index){
        string key = "";
        for(int g=0; g<groups.size(); g++){
            string groupby_col_name = groups[g];
            int groupby_col_type = groupby_table.table_data[groupby_col_name].type;
            string trim_str;
            switch(groupby_col_type){
            case TYPE_STRING:
                trim_str  = trim(groupby_table.table_data[groupby_col_name].strvec[row_index]);
                key += "|" + trim_str;
                break;
            case TYPE_INT:
                key += "|" + to_string(groupby_table.table_data[groupby_col_name].intvec[row_index]);
                break;
            case TYPE_FLOAT:
                key += "|" + to_string(groupby_table.table_data[groupby_col_name].floatvec[row_index]);
                break;
            }
        }
        return key;
    }

void MergeQueryManager::GroupBy(TableData &groupby_table, const RepeatedPtrField<string>& groups, vector<TableData> &dest){
    int increase_index = 0;
    int target_row_num = groupby_table.row_count;

    if(target_row_num == 0){
        dest.push_back(groupby_table);
    }else{
        for(int r=0; r<target_row_num; r++){
            string key = makeGroupbyKey(groupby_table, groups, r);
            int group_by_index;

            if (group_by_key_.find(key) == group_by_key_.end()) {//group not exist
                group_by_index = increase_index;
                group_by_key_.insert({key,group_by_index});
                increase_index++;
                TableData new_table;
                dest.push_back(new_table);
            }else{//group exist
                group_by_index = group_by_key_[key];
            }
            
            for(auto& col: groupby_table.table_data){
                string col_name = col.first;
                switch(col.second.type){
                    case TYPE_INT:
                        dest[group_by_index].table_data[col_name].intvec.push_back(groupby_table.table_data[col_name].intvec[r]);
                        break;
                    case TYPE_STRING:
                        dest[group_by_index].table_data[col_name].strvec.push_back(groupby_table.table_data[col_name].strvec[r]);
                        break;
                    case TYPE_FLOAT:
                        dest[group_by_index].table_data[col_name].floatvec.push_back(groupby_table.table_data[col_name].floatvec[r]);
                        break;
                    default:
                    KETILOG::ERRORLOG(LOGTAG,"group by type check plz.. "+col_name);
                        break;
                }
                dest[group_by_index].table_data[col_name].isnull.push_back(groupby_table.table_data[col_name].isnull[r]);
                dest[group_by_index].table_data[col_name].row_count++;
            }
            dest[group_by_index].row_count++;
        }

        for(auto& col: groupby_table.table_data){
            string col_name = col.first;
            for(int i=0; i<increase_index; i++){
                dest[i].table_data[col_name].type = col.second.type;
            }
        }
    }
}

void MergeQueryManager::OrderBy(TableData &orderby_table, const SnippetRequest_Order& orders, TableData &dest){
    if(orderby_table.row_count == 0){
        dest = orderby_table;
        return;
    }

    for(int i=0; i<orderby_table.row_count; i++){
        ordered_index_.push_back(i);
    }

    sort(ordered_index_.begin(), ordered_index_.end(), [&](int i, int j) {
        for(int c=0; c<orders.column_name_size(); c++){
            if(orders.ascending(c) == SnippetRequest_Order_OrderDirection_ASC){
                switch(orderby_table.table_data[orders.column_name(c)].type){
                    case TYPE_INT:
                        if(orderby_table.table_data[orders.column_name(c)].intvec[i] != orderby_table.table_data[orders.column_name(c)].intvec[j]){
                            return orderby_table.table_data[orders.column_name(c)].intvec[i] < orderby_table.table_data[orders.column_name(c)].intvec[j];
                        }
                        break;
                    case TYPE_FLOAT:
                        if(orderby_table.table_data[orders.column_name(c)].floatvec[i] != orderby_table.table_data[orders.column_name(c)].floatvec[j]){
                            return orderby_table.table_data[orders.column_name(c)].floatvec[i] < orderby_table.table_data[orders.column_name(c)].floatvec[j];
                        }
                        break;
                    case TYPE_STRING:
                        if(orderby_table.table_data[orders.column_name(c)].strvec[i] != orderby_table.table_data[orders.column_name(c)].strvec[j]){
                            return orderby_table.table_data[orders.column_name(c)].strvec[i] < orderby_table.table_data[orders.column_name(c)].strvec[j];
                        }
                        break;
                    default:
                        KETILOG::ERRORLOG(LOGTAG,"order by sort asc default");
                        return true;
                }
            }else{//SnippetRequest_Order_OrderDirection_DESC
                switch(orderby_table.table_data[orders.column_name(c)].type){
                    case TYPE_INT:
                        if(orderby_table.table_data[orders.column_name(c)].intvec[i] != orderby_table.table_data[orders.column_name(c)].intvec[j]){
                            return orderby_table.table_data[orders.column_name(c)].intvec[i] > orderby_table.table_data[orders.column_name(c)].intvec[j];
                        }
                        break;
                    case TYPE_FLOAT:
                        if(orderby_table.table_data[orders.column_name(c)].floatvec[i] != orderby_table.table_data[orders.column_name(c)].floatvec[j]){
                            return orderby_table.table_data[orders.column_name(c)].floatvec[i] > orderby_table.table_data[orders.column_name(c)].floatvec[j];
                        }
                        break;
                    case TYPE_STRING:
                        if(orderby_table.table_data[orders.column_name(c)].strvec[i] != orderby_table.table_data[orders.column_name(c)].strvec[j]){
                            return orderby_table.table_data[orders.column_name(c)].strvec[i] > orderby_table.table_data[orders.column_name(c)].strvec[j];
                        }
                        break;
                    default:
                        KETILOG::ERRORLOG(LOGTAG,"order by sort desc default");
                        return true;
                }
            }
        }
    });
    
    // if(KETILOG::IsLogLevelUnder(TRACE)){
    //     // 정렬 인덱스 확인 - Debug Code   
    //     cout << "<check ordered index> ";
    //     for(int i=0; i<ordered_index_.size(); i++){
    //         cout << ordered_index_[i] << " ";
    //     }
    //     cout << endl;
    // }

    for(auto table: orderby_table.table_data){
        switch(table.second.type){
            case TYPE_INT:
                for(int i=0; i<ordered_index_.size(); i++){
                    dest.table_data[table.first].intvec.push_back(table.second.intvec[ordered_index_[i]]);
                    dest.table_data[table.first].isnull.push_back(table.second.isnull[ordered_index_[i]]);
                }
                dest.table_data[table.first].type = TYPE_INT;
                break;
            case TYPE_FLOAT:
                for(int i=0; i<ordered_index_.size(); i++){
                    dest.table_data[table.first].floatvec.push_back(table.second.floatvec[ordered_index_[i]]);
                    dest.table_data[table.first].isnull.push_back(table.second.isnull[ordered_index_[i]]);
                }
                dest.table_data[table.first].type = TYPE_FLOAT;
                break;
            case TYPE_STRING:
                for(int i=0; i<ordered_index_.size(); i++){
                    dest.table_data[table.first].strvec.push_back(table.second.strvec[ordered_index_[i]]);
                    dest.table_data[table.first].isnull.push_back(table.second.isnull[ordered_index_[i]]);
                }
                dest.table_data[table.first].type = TYPE_STRING;
                break;
            default:
                KETILOG::ERRORLOG(LOGTAG,"order by type error check plz..");
        }
        dest.table_data[table.first].row_count = table.second.row_count;
    }
    dest.row_count = orderby_table.row_count;
}

void MergeQueryManager::Filtering(TableData &filter_table, const RepeatedPtrField<SnippetRequest_Filter>& filters, TableData &dest_table){
    for (auto it = filter_table.table_data.begin(); it != filter_table.table_data.end(); it++){
        dest_table.table_data[(*it).first].type = (*it).second.type;
    }

    for(int r=0; r<filter_table.row_count; r++){
        bool passed = true;

        for(int f=0; f<snippet_.query_info().filtering_size(); f++){
            if(f%2 == 1){
                if(snippet_.query_info().filtering(f).operator_() == SnippetRequest_Filter_OperType::SnippetRequest_Filter_OperType_AND){
                    if(passed == false) break;
                }else{
                    KETILOG::ERRORLOG(LOGTAG,"Filtering: " + snippet_.query_info().filtering(f).operator_());
                }
            }else{
                string left_col = snippet_.query_info().filtering(f).lv().value(0);

                switch(filter_table.table_data[left_col].type){
                case TYPE_STRING:{
                    string lv = trim(filter_table.table_data[left_col].strvec[r]);
                    string rv = snippet_.query_info().filtering(f).rv().value(0);
                    passed = compareByOperator(snippet_.query_info().filtering(f).operator_(), lv, rv);
                    break;
                }case TYPE_FLOAT:{
                    float lv = filter_table.table_data[left_col].floatvec[r];
                    float rv = stof(snippet_.query_info().filtering(f).rv().value(0));
                    cout << lv << " > " << rv << "=" << (lv > rv) << endl;
                    passed = compareByOperator(snippet_.query_info().filtering(f).operator_(), lv, rv);
                    break;
                }case TYPE_INT:{
                    int lv = filter_table.table_data[left_col].intvec[r];
                    int rv = stoi(snippet_.query_info().filtering(f).rv().value(0));
                    passed = compareByOperator(snippet_.query_info().filtering(f).operator_(), lv, rv);
                    break;
                }default:
                    KETILOG::ERRORLOG(LOGTAG,"Filtering (d)");
                }
            }
        }

        if(passed){ 
            for (auto it = filter_table.table_data.begin(); it != filter_table.table_data.end(); it++){
                switch((*it).second.type){
                case TYPE_INT:
                    dest_table.table_data[(*it).first].intvec.push_back((*it).second.intvec[r]);
                    break;
                case TYPE_STRING:
                    dest_table.table_data[(*it).first].strvec.push_back((*it).second.strvec[r]);
                    break;
                case TYPE_FLOAT:
                    dest_table.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r]);
                    break;
                }
                dest_table.table_data[(*it).first].isnull.push_back((*it).second.isnull[r]);
                dest_table.table_data[(*it).first].row_count++;
            }
            dest_table.row_count++;
        }
    }
    
}

void MergeQueryManager::InnerJoin_hash(){
    int row_index = 0;
    bool first = true;

    left_table_ = BufferManager::GetFinishedTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(0));
    debug_table(1);

    for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
        target_table_.table_data[(*it).first].type = (*it).second.type;
    }

    vector<string> equal_join_column;
    vector<int> equal_join_index; 
    for(int f = 0; f < snippet_.query_info().filtering_size(); f++){
        if(snippet_.query_info().filtering(f).operator_() == SnippetRequest_Filter_OperType::SnippetRequest_Filter_OperType_EQ){
            equal_join_column.push_back(snippet_.query_info().filtering(f).lv().value(0));
            equal_join_index.push_back(f);
        }
    }
    createHashTable(left_table_, equal_join_column);

    while(true){
        right_table_ = BufferManager::GetTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(1), row_index);
        debug_table(2);

        cout << "## " << right_table_.status << endl;

        if(row_index == right_table_.row_count && right_table_.status == WorkDone){
            break;
        }else if(row_index == right_table_.row_count && right_table_.status != WorkDone){
            continue;
        }

        if(first){
            for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
                target_table_.table_data[(*it).first].type = (*it).second.type;
            }
            first = false;
        }

        bool equal_join_exist = (equal_join_column.size() != 0)? true : false;

        if(equal_join_exist){ //hash join::equal filter + (nested loop join::non equal filter)
            for(int r = row_index; r < right_table_.row_count; r++){
                string hash_key = "";

                for(int f = 0; f < equal_join_index.size(); f++){
                    int index = equal_join_index[f];
                    string hash_key_column = snippet_.query_info().filtering(index).rv().value(0);

                    switch(right_table_.table_data[hash_key_column].type){
                    case TYPE_STRING:
                        hash_key += "|" + right_table_.table_data[hash_key_column].strvec[r];
                        break;
                    case TYPE_INT:
                        hash_key += "|" + to_string(right_table_.table_data[hash_key_column].intvec[r]);
                        break;
                    case TYPE_FLOAT:
                        hash_key += "|" + to_string(right_table_.table_data[hash_key_column].floatvec[r]);
                        break;
                    }   
                }

                for(int i = 0; i < hash_table_[hash_key].size(); i++){
                    bool passed = true;
                    for(int f = 0; f < snippet_.query_info().filtering_size(); f++){
                        if(snippet_.query_info().filtering(f).operator_() == SnippetRequest_Filter_OperType::SnippetRequest_Filter_OperType_GE ||
                            snippet_.query_info().filtering(f).operator_() == SnippetRequest_Filter_OperType::SnippetRequest_Filter_OperType_LE ||
                            snippet_.query_info().filtering(f).operator_() == SnippetRequest_Filter_OperType::SnippetRequest_Filter_OperType_GT ||
                            snippet_.query_info().filtering(f).operator_() == SnippetRequest_Filter_OperType::SnippetRequest_Filter_OperType_LT ||
                            snippet_.query_info().filtering(f).operator_() == SnippetRequest_Filter_OperType::SnippetRequest_Filter_OperType_NE){

                            string left_column = snippet_.query_info().filtering(f).lv().value(0);
                            string right_column = snippet_.query_info().filtering(f).rv().value(0);
                            int oper = snippet_.query_info().filtering(f).operator_();

                            switch(left_table_.table_data[left_column].type){
                            case TYPE_STRING:
                                passed = compareByOperator(oper, left_table_.table_data[left_column].strvec[hash_table_[hash_key][i]],
                                                            right_table_.table_data[right_column].strvec[r]);
                                break;
                            case TYPE_INT:
                                passed = compareByOperator(oper, left_table_.table_data[left_column].intvec[hash_table_[hash_key][i]],
                                                            right_table_.table_data[right_column].intvec[r]);
                                break;
                            case TYPE_FLOAT:
                                passed = compareByOperator(oper, left_table_.table_data[left_column].floatvec[hash_table_[hash_key][i]],
                                                            right_table_.table_data[right_column].floatvec[r]);
                                break;
                            }

                            if(passed == false) break;
                        }else{
                            continue;
                        }
                    }

                    if(passed){ 
                        for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
                            switch((*it).second.type){
                            case TYPE_INT:
                                target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[r]);
                                break;
                            case TYPE_STRING:
                                target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[r]);
                                break;
                            case TYPE_FLOAT:
                                target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r]);
                                break;
                            }
                            target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[r]);
                            target_table_.table_data[(*it).first].row_count++;
                        }
                        for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                            switch((*it).second.type){
                            case TYPE_INT:
                                target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[hash_table_[hash_key][i]]);
                                break;
                            case TYPE_STRING:
                                target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[hash_table_[hash_key][i]]);
                                break;
                            case TYPE_FLOAT:
                                target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[hash_table_[hash_key][i]]);
                                break;
                            }
                            target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[hash_table_[hash_key][i]]);
                            target_table_.table_data[(*it).first].row_count++;
                        }
                        target_table_.row_count++;
                    }
                }
            }
            
        }else{ // nested loop join::only non equal filter
            for(int r1=0; r1<left_table_.row_count; r1++){
                for(int r2=0; r2<right_table_.row_count; r2++){
                    bool passed = true;

                    for(int f=0; f<snippet_.query_info().filtering_size(); f++){
                        if(snippet_.query_info().filtering(f).operator_() == SnippetRequest_Filter_OperType::SnippetRequest_Filter_OperType_GE ||
                            snippet_.query_info().filtering(f).operator_() == SnippetRequest_Filter_OperType::SnippetRequest_Filter_OperType_LE ||
                            snippet_.query_info().filtering(f).operator_() == SnippetRequest_Filter_OperType::SnippetRequest_Filter_OperType_GT ||
                            snippet_.query_info().filtering(f).operator_() == SnippetRequest_Filter_OperType::SnippetRequest_Filter_OperType_LT ||
                            snippet_.query_info().filtering(f).operator_() == SnippetRequest_Filter_OperType::SnippetRequest_Filter_OperType_NE){

                            string left_column = snippet_.query_info().filtering(f).lv().value(0);
                            string right_column = snippet_.query_info().filtering(f).rv().value(0);
                            int oper = snippet_.query_info().filtering(f).operator_();

                            // passed = compareByOperator(snippet.query_info().filtering(f).operator_(), left_column, right_column, r1,r2);
                            switch(left_table_.table_data[left_column].type){
                            case TYPE_STRING:
                                passed = compareByOperator(oper, left_table_.table_data[left_column].strvec[r1],
                                                            right_table_.table_data[right_column].strvec[r2]);
                                break;
                            case TYPE_INT:
                                passed = compareByOperator(oper, left_table_.table_data[left_column].intvec[r1],
                                                            right_table_.table_data[right_column].intvec[r2]);
                                break;
                            case TYPE_FLOAT:
                                passed = compareByOperator(oper, left_table_.table_data[left_column].floatvec[r1],
                                                            right_table_.table_data[right_column].floatvec[r2]);
                                break;
                            }

                            if(passed == false) break;
                        }else{
                            continue;
                        }
                    }

                    if(passed){ 
                        for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                            switch((*it).second.type){
                            case TYPE_INT:
                                target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[r1]);
                                break;
                            case TYPE_STRING:
                                target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[r1]);
                                break;
                            case TYPE_FLOAT:
                                target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r1]);
                                break;
                            }
                            target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[r1]);
                            target_table_.table_data[(*it).first].row_count++;
                        }
                        for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
                            switch((*it).second.type){
                            case TYPE_INT:
                                target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[r2]);
                                break;
                            case TYPE_STRING:
                                target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[r2]);
                                break;
                            case TYPE_FLOAT:
                                target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r2]);
                                break;
                            }
                            target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[r2]);
                            target_table_.table_data[(*it).first].row_count++;
                        }
                        target_table_.row_count++;
                    }
                }
            }
        }  

        row_index = right_table_.row_count;

        if(right_table_.status == WorkDone){
            break;
        }
    }

    hash_table_.clear();
}

void MergeQueryManager::InnerJoin_nestedloop(){
    for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
        target_table_.table_data[(*it).first].type = (*it).second.type;
    }
    for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
        target_table_.table_data[(*it).first].type = (*it).second.type;
    }

    for(int r1=0; r1<left_table_.row_count; r1++){

        for(int r2=0; r2<right_table_.row_count; r2++){
            bool passed = false;

            for(int f=0; f<snippet_.query_info().filtering_size(); f+=2){//동등조인,비동등조인 여부 판단 필요
                string left_column = snippet_.query_info().filtering(f).lv().value(0);
                string right_column = snippet_.query_info().filtering(f).rv().value(0);
                int oper = snippet_.query_info().filtering(f).operator_();

                switch(left_table_.table_data[left_column].type){
                case TYPE_STRING:
                    passed = compareByOperator(oper, left_table_.table_data[left_column].strvec[r1],
                                                right_table_.table_data[right_column].strvec[r2]);
                    break;
                case TYPE_INT:
                    passed = compareByOperator(oper, left_table_.table_data[left_column].intvec[r1],
                                                right_table_.table_data[right_column].intvec[r2]);
                    break;
                case TYPE_FLOAT:
                    passed = compareByOperator(oper, left_table_.table_data[left_column].floatvec[r1],
                                                right_table_.table_data[right_column].floatvec[r2]);
                    break;
                }

                if(passed == false) break;
                
            }

            if(passed){ 
                for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                    switch((*it).second.type){
                    case TYPE_INT:
                        target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[r1]);
                        break;
                    case TYPE_STRING:
                        target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[r1]);
                        break;
                    case TYPE_FLOAT:
                        target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r1]);
                        break;
                    }
                    target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[r1]);
                    target_table_.table_data[(*it).first].row_count++;
                }
                for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
                    switch((*it).second.type){
                    case TYPE_INT:
                        target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[r2]);
                        break;
                    case TYPE_STRING:
                        target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[r2]);
                        break;
                    case TYPE_FLOAT:
                        target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r2]);
                        break;
                    }
                    target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[r2]);
                    target_table_.table_data[(*it).first].row_count++;
                }
                target_table_.row_count++;
            }
        }
    }
}

void MergeQueryManager::LeftOuterJoin_hash(){
    int row_index = 0;
    bool first = true;

    right_table_ = BufferManager::GetFinishedTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(1));
    debug_table(2);

    for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
        target_table_.table_data[(*it).first].type = (*it).second.type;
    }

    vector<string> equal_join_column;
    vector<int> equal_join_index;
    for(int f = 0; f < snippet_.query_info().filtering_size(); f++){
        if(snippet_.query_info().filtering(f).operator_() == SnippetRequest_Filter_OperType::SnippetRequest_Filter_OperType_EQ){
            equal_join_column.push_back(snippet_.query_info().filtering(f).rv().value(0));
            equal_join_index.push_back(f);
        }
    }

    createHashTable(right_table_, equal_join_column);

    while(true){
        left_table_ = BufferManager::GetTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(0), row_index);
        debug_table(1);

        if(row_index == left_table_.row_count && left_table_.status == WorkDone){
            break;
        }else if(row_index == left_table_.row_count && left_table_.status != WorkDone){
            continue;
        }

        if(first){
            for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                target_table_.table_data[(*it).first].type = (*it).second.type;
            }
            first = false;
        }
        
        for(int r=row_index; r<left_table_.row_count; r++){
            string hash_key = "";
            bool passed = false;

            for(int f = 0; f < equal_join_index.size(); f++){
                string hash_key_column = snippet_.query_info().filtering(equal_join_index[f]).lv().value(0);

                switch(left_table_.table_data[hash_key_column].type){
                case TYPE_STRING:
                    hash_key += "|" + left_table_.table_data[hash_key_column].strvec[r];
                    break;
                case TYPE_INT:
                    hash_key += "|" + to_string(left_table_.table_data[hash_key_column].intvec[r]);
                    break;
                case TYPE_FLOAT:
                    hash_key += "|" + to_string(left_table_.table_data[hash_key_column].floatvec[r]);
                    break;
                }   
            }

            passed = (hash_table_.find(hash_key) != hash_table_.end())? true : false;

            if(passed){ 
                for(int i = 0; i < hash_table_[hash_key].size(); i++){
                    for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                        switch((*it).second.type){
                        case TYPE_INT:
                            target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[r]);
                            break;
                        case TYPE_STRING:
                            target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[r]);
                            break;
                        case TYPE_FLOAT:
                            target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r]);
                            break;
                        }
                        target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[r]);
                        target_table_.table_data[(*it).first].row_count++;
                    }
                    for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
                        switch((*it).second.type){
                        case TYPE_INT:
                            target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[hash_table_[hash_key][i]]);
                            break;
                        case TYPE_STRING:
                            target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[hash_table_[hash_key][i]]);
                            break;
                        case TYPE_FLOAT:
                            target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[hash_table_[hash_key][i]]);
                            break;
                        }
                        target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[i]);
                        target_table_.table_data[(*it).first].row_count++;
                    }
                    target_table_.row_count++;
                }
            }else{
                for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                    switch((*it).second.type){
                    case TYPE_INT:
                        target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[r]);
                        break;
                    case TYPE_STRING:
                        target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[r]);
                        break;
                    case TYPE_FLOAT:
                        target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r]);
                        break;
                    }
                    target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[r]);
                    target_table_.table_data[(*it).first].row_count++;
                }
                for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
                    switch((*it).second.type){
                    case TYPE_INT:
                        target_table_.table_data[(*it).first].intvec.push_back(0);
                        break;
                    case TYPE_STRING:
                        target_table_.table_data[(*it).first].strvec.push_back("");
                        break;
                    case TYPE_FLOAT:
                        target_table_.table_data[(*it).first].floatvec.push_back(0);
                        break;
                    }
                    target_table_.table_data[(*it).first].isnull.push_back(true);//null check
                    target_table_.table_data[(*it).first].row_count++;
                }
                target_table_.row_count++;
            }
        }

        row_index = left_table_.row_count;

        if(row_index == left_table_.row_count && left_table_.status == WorkDone){
            break;
        }
    }

    hash_table_.clear();
}

// void MergeQueryManager::LeftOuterJoin_nestedloop(){
//     for (auto it = left_table.table_data.begin(); it != left_table.table_data.end(); it++){
//         target_table.table_data[(*it).first].type = (*it).second.type;
//     }
//     for (auto it = right_table.table_data.begin(); it != right_table.table_data.end(); it++){
//         target_table.table_data[(*it).first].type = (*it).second.type;
//     }

//     for(int r1=0; r1<left_table.row_count; r1++){
//         bool exist = false;

//         for(int r2=0; r2<right_table.row_count; r2++){
//             bool passed = false;

//             for(int f=0; f<snippet.table_filter_size(); f++){//동등조인,비동등조인 여부 판단 필요
//                 if(f%2 == 1){
//                     switch(snippet.table_filter(f).operator_())
//                     case KETI_AND:{//and가 아닌경우??
//                         continue;
//                     }
//                 }else{
//                     bool filter_passed = false;
//                     string driving_c1 = snippet.table_filter(f).lv().value(0);
//                     string driven_c2 = snippet.table_filter(f).rv().value(0);

//                     switch(left_table.table_data[driving_c1].type){
//                     case TYPE_INT:
//                         if(left_table.table_data[driving_c1].intvec[r1] == right_table.table_data[driven_c2].intvec[r2]){
//                             filter_passed = true;
//                             break;
//                         }
//                         break;
//                     case TYPE_FLOAT:
//                         if(left_table.table_data[driving_c1].floatvec[r1] == right_table.table_data[driven_c2].floatvec[r2]){
//                             filter_passed = true;
//                             break;
//                         }
//                         break;
//                     case TYPE_STRING:
//                         if(left_table.table_data[driving_c1].strvec[r1] == right_table.table_data[driven_c2].strvec[r2]){
//                             filter_passed = true;
//                             break;
//                         }
//                         break;
//                     default:
//                         KETILOG::ERRORLOG(LOGTAG,"inner join type check plz... " + to_string(left_table.table_data[driving_c1].type));
//                     }

//                     if(filter_passed){
//                         passed = true;
//                         filter_passed = false;
//                     }else{
//                         passed = false;
//                         break;
//                     }
//                 }
//             }

//             if(passed){ 
//                 exist = true;
//                 for (auto it = left_table.table_data.begin(); it != left_table.table_data.end(); it++){
//                     switch((*it).second.type){
//                     case TYPE_INT:
//                         target_table.table_data[(*it).first].intvec.push_back((*it).second.intvec[r1]);
//                         break;
//                     case TYPE_STRING:
//                         target_table.table_data[(*it).first].strvec.push_back((*it).second.strvec[r1]);
//                         break;
//                     case TYPE_FLOAT:
//                         target_table.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r1]);
//                         break;
//                     }
//                     target_table.table_data[(*it).first].isnull.push_back((*it).second.isnull[r1]);
//                     target_table.table_data[(*it).first].row_count++;
//                 }
//                 for (auto it = right_table.table_data.begin(); it != right_table.table_data.end(); it++){
//                     switch((*it).second.type){
//                     case TYPE_INT:
//                         target_table.table_data[(*it).first].intvec.push_back((*it).second.intvec[r2]);
//                         break;
//                     case TYPE_STRING:
//                         target_table.table_data[(*it).first].strvec.push_back((*it).second.strvec[r2]);
//                         break;
//                     case TYPE_FLOAT:
//                         target_table.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r2]);
//                         break;
//                     }
//                     target_table.table_data[(*it).first].isnull.push_back((*it).second.isnull[r2]);
//                     target_table.table_data[(*it).first].row_count++;
//                 }
//                 target_table.row_count++;
//             }
//         }

//         if(!exist){
//             for (auto it = left_table.table_data.begin(); it != left_table.table_data.end(); it++){
//                 switch((*it).second.type){
//                 case TYPE_INT:
//                     target_table.table_data[(*it).first].intvec.push_back((*it).second.intvec[r1]);
//                     break;
//                 case TYPE_STRING:
//                     target_table.table_data[(*it).first].strvec.push_back((*it).second.strvec[r1]);
//                     break;
//                 case TYPE_FLOAT:
//                     target_table.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r1]);
//                     break;
//                 }
//                 target_table.table_data[(*it).first].isnull.push_back((*it).second.isnull[r1]);
//                 target_table.table_data[(*it).first].row_count++;
//             }
//             for (auto it = right_table.table_data.begin(); it != right_table.table_data.end(); it++){
//                 switch((*it).second.type){
//                 case TYPE_INT:
//                     target_table.table_data[(*it).first].intvec.push_back(0);
//                     break;
//                 case TYPE_STRING:
//                     target_table.table_data[(*it).first].strvec.push_back("");
//                     break;
//                 case TYPE_FLOAT:
//                     target_table.table_data[(*it).first].floatvec.push_back(0);
//                     break;
//                 }
//                 target_table.table_data[(*it).first].isnull.push_back(true);//null check
//                 target_table.table_data[(*it).first].row_count++;
//             }
//             target_table.row_count++;
//         }
//     }
// }

void MergeQueryManager::RightOuterJoin_hash(){
    int row_index = 0;
    bool first = true;

    left_table_ = BufferManager::GetFinishedTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(0));
    debug_table(1);

    for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
        target_table_.table_data[(*it).first].type = (*it).second.type;
    }

    vector<string> equal_join_column;
    vector<int> equal_join_index;
    
    for(int f = 0; f < snippet_.query_info().filtering_size(); f++){
        if(snippet_.query_info().filtering(f).operator_() == SnippetRequest_Filter_OperType::SnippetRequest_Filter_OperType_EQ){
            equal_join_column.push_back(snippet_.query_info().filtering(f).lv().value(0));
            equal_join_index.push_back(f);
        }
    }

    createHashTable(left_table_, equal_join_column);

    while(true){
        right_table_ = BufferManager::GetTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(1), row_index);
        debug_table(2);

        if(row_index == right_table_.row_count && right_table_.status == WorkDone){
            break;
        }else if(row_index == right_table_.row_count && right_table_.status != WorkDone){
            continue;
        }

        if(first){
            for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
                target_table_.table_data[(*it).first].type = (*it).second.type;
            }
            first = false;
        }

        for(int r=row_index; r<right_table_.row_count; r++){
            string hash_key = "";
            bool passed = false;

            for(int f = 0; f < equal_join_index.size(); f++){
                string hash_key_column = snippet_.query_info().filtering(equal_join_index[f]).lv().value(0);

                switch(right_table_.table_data[hash_key_column].type){
                case TYPE_STRING:
                    hash_key += "|" + right_table_.table_data[hash_key_column].strvec[r];
                    break;
                case TYPE_INT:
                    hash_key += "|" + to_string(right_table_.table_data[hash_key_column].intvec[r]);
                    break;
                case TYPE_FLOAT:
                    hash_key += "|" + to_string(right_table_.table_data[hash_key_column].floatvec[r]);
                    break;
                }   
            }

            passed = (hash_table_.find(hash_key) != hash_table_.end())? true : false;

            if(passed){ 
                for(int i = 0; i < hash_table_[hash_key].size(); i++){
                    for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
                        switch((*it).second.type){
                        case TYPE_INT:
                            target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[r]);
                            break;
                        case TYPE_STRING:
                            target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[r]);
                            break;
                        case TYPE_FLOAT:
                            target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r]);
                            break;
                        }
                        target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[r]);
                        target_table_.table_data[(*it).first].row_count++;
                    }
                    for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                        switch((*it).second.type){
                        case TYPE_INT:
                            target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[hash_table_[hash_key][i]]);
                            break;
                        case TYPE_STRING:
                            target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[hash_table_[hash_key][i]]);
                            break;
                        case TYPE_FLOAT:
                            target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[hash_table_[hash_key][i]]);
                            break;
                        }
                        target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[i]);
                        target_table_.table_data[(*it).first].row_count++;
                    }
                    target_table_.row_count++;
                }
            }else{
                for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
                    switch((*it).second.type){
                    case TYPE_INT:
                        target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[r]);
                        break;
                    case TYPE_STRING:
                        target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[r]);
                        break;
                    case TYPE_FLOAT:
                        target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r]);
                        break;
                    }
                    target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[r]);
                    target_table_.table_data[(*it).first].row_count++;
                }
                for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                    switch((*it).second.type){
                    case TYPE_INT:
                        target_table_.table_data[(*it).first].intvec.push_back(0);
                        break;
                    case TYPE_STRING:
                        target_table_.table_data[(*it).first].strvec.push_back("");
                        break;
                    case TYPE_FLOAT:
                        target_table_.table_data[(*it).first].floatvec.push_back(0);
                        break;
                    }
                    target_table_.table_data[(*it).first].isnull.push_back(true);//null check
                    target_table_.table_data[(*it).first].row_count++;
                }
                target_table_.row_count++;
            }
        }

        row_index = right_table_.row_count;

        if(right_table_.status == WorkDone){
            break;
        }
    }
    hash_table_.clear();
}

void MergeQueryManager::CrossJoin(){}

void MergeQueryManager::Union(){
    int row_index = 0;
    
    left_table_ = BufferManager::GetFinishedTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(0));
    debug_table(1);

    if(snippet_.query_info().filtering_size() == 0){// Union
        // 중복 로우 포함 X
    }else{ // Union All
        target_table_.table_data = left_table_.table_data;

        while(true){
            right_table_ = BufferManager::GetTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(1), row_index);
            debug_table(2);

            if(row_index == right_table_.row_count && right_table_.status == WorkDone){
                break;
            }else if(row_index == right_table_.row_count && right_table_.status != WorkDone){
                continue;
            }

            int end = right_table_.row_count;

            for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
                switch((*it).second.type){
                case TYPE_INT:
                    target_table_.table_data[(*it).first].intvec.insert(target_table_.table_data[(*it).first].intvec.end(),right_table_.table_data[(*it).first].intvec.begin()+row_index,right_table_.table_data[(*it).first].intvec.begin()+end);
                    break;
                case TYPE_STRING:
                    target_table_.table_data[(*it).first].strvec.insert(target_table_.table_data[(*it).first].strvec.end(),right_table_.table_data[(*it).first].strvec.begin()+row_index,right_table_.table_data[(*it).first].strvec.begin()+end);
                    break;
                case TYPE_FLOAT:
                    target_table_.table_data[(*it).first].floatvec.insert(target_table_.table_data[(*it).first].floatvec.end(),right_table_.table_data[(*it).first].floatvec.begin()+row_index,right_table_.table_data[(*it).first].floatvec.begin()+end);
                    break;
                }
                target_table_.table_data[(*it).first].row_count += right_table_.table_data[(*it).first].row_count;
                target_table_.table_data[(*it).first].isnull.insert(target_table_.table_data[(*it).first].isnull.end(),right_table_.table_data[(*it).first].isnull.begin()+row_index,right_table_.table_data[(*it).first].isnull.begin()+end);
            }

            row_index = right_table_.row_count;

            if(right_table_.status == WorkDone){
                break;
            }
        }

        for(auto it =target_table_.table_data.begin(); it != target_table_.table_data.end(); it++){
            if((*it).second.floatvec.size() != 0){
                (*it).second.type = TYPE_FLOAT;
            }else if((*it).second.intvec.size() != 0){
                (*it).second.type = TYPE_INT;
            }else if((*it).second.strvec.size() != 0){
                (*it).second.type = TYPE_STRING;
            }else{
                (*it).second.type = TYPE_EMPTY;
            }
        }

        target_table_.row_count = left_table_.row_count + right_table_.row_count;
    }
}

void MergeQueryManager::In(){
    int row_index = 0;
    bool first = true;

    right_table_ = BufferManager::GetFinishedTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(1));
    debug_table(2);

    // for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
    //     target_table_.table_data[(*it).first].type = (*it).second.type;
    // }

    vector<string> equal_join_column;
    equal_join_column.push_back(snippet_.query_info().filtering(0).rv().value(0));
    createHashTable(right_table_, equal_join_column);

    while(true){
        left_table_ = BufferManager::GetTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(0), row_index);
        debug_table(1);

        if(row_index == left_table_.row_count && left_table_.status == WorkDone){
            break;
        }else if(row_index == left_table_.row_count && left_table_.status != WorkDone){
            continue;
        }

        if(first){
            for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                target_table_.table_data[(*it).first].type = (*it).second.type;
            }
            first = false;
        }

        for(int r=row_index; r<left_table_.row_count; r++){
            string hash_key = "";
            bool passed = true;
            string hash_key_column = snippet_.query_info().filtering(0).lv().value(0);

            switch(left_table_.table_data[hash_key_column].type){
            case TYPE_STRING:
                hash_key += "|" + left_table_.table_data[hash_key_column].strvec[r];
                break;
            case TYPE_INT:
                hash_key += "|" + to_string(left_table_.table_data[hash_key_column].intvec[r]);
                break;
            case TYPE_FLOAT:
                hash_key += "|" + to_string(left_table_.table_data[hash_key_column].floatvec[r]);
                break;
            } 

            if(snippet_.query_info().filtering(0).operator_() == SnippetRequest_Filter_OperType::SnippetRequest_Filter_OperType_NOTIN){
                passed = (hash_table_.find(hash_key) != hash_table_.end())? false : true;
            }else{
                passed = (hash_table_.find(hash_key) != hash_table_.end())? true : false;
            }

            if(passed){
                for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                    switch((*it).second.type){
                    case TYPE_INT:
                        target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[r]);
                        break;
                    case TYPE_STRING:
                        target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[r]);
                        break;
                    case TYPE_FLOAT:
                        target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r]);
                        break;
                    }
                    target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[r]);
                    target_table_.table_data[(*it).first].row_count++;
                }
                target_table_.row_count++;
            } 
        }
        row_index = left_table_.row_count;



        if(row_index == left_table_.row_count && left_table_.status == WorkDone){
            break;
        }
    }
}

void MergeQueryManager::DependencyInnerJoin(){
    int row_index = 0;
    bool first = true;
    
    right_table_ = BufferManager::GetFinishedTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(1));
    debug_table(2);

    string lv_column = snippet_.query_info().filtering(0).lv().value(0);
    int lv_type = left_table_.table_data[lv_column].type;

    while(true){
        left_table_ = BufferManager::GetTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(0), row_index);
        debug_table(1);
    
        if(row_index == left_table_.row_count && left_table_.status == WorkDone){
            break;
        }else if(row_index == left_table_.row_count && left_table_.status != WorkDone){
            continue;
        }

        if(first){
            for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                target_table_.table_data[(*it).first].type = (*it).second.type;
            }
            first = false;
        }

        for(int r1=row_index; r1<left_table_.row_count; r1++){
            T lv;
            lv.type = lv_type;
            if(lv_type == TYPE_STRING){
                lv.varString = left_table_.table_data[lv_column].strvec[r1];
            }else if(lv_type == TYPE_INT){
                lv.varInt = left_table_.table_data[lv_column].intvec[r1];
            }else{
                lv.varFloat = left_table_.table_data[lv_column].floatvec[r1];
            }

            TableData dependency_subquery_filtered_table;
            for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
                dependency_subquery_filtered_table.table_data[(*it).first].type = (*it).second.type;
            }

            for(int r2=0; r2<right_table_.row_count; r2++){
                bool passed = false;

                for(int f=0; f<snippet_.query_info().dependency().dependency_filter_size(); f+=2){
                    string dependency_lv_column = snippet_.query_info().dependency().dependency_filter(f).lv().value(0);
                    string dependency_rv_column = snippet_.query_info().dependency().dependency_filter(f).rv().value(0);
                    int oper = snippet_.query_info().dependency().dependency_filter(f).operator_();
                    int dependency_lv_type = left_table_.table_data[dependency_lv_column].type;
                    int dependency_rv_type = right_table_.table_data[dependency_rv_column].type;

                    if(dependency_lv_type == TYPE_STRING){
                        string dependency_lv_value = left_table_.table_data[dependency_lv_column].strvec[r1];
                        string dependency_rv_value = right_table_.table_data[dependency_rv_column].strvec[r2];
                        passed = compareByOperator(oper,dependency_lv_value,dependency_rv_value);
                    }else if(dependency_lv_type == TYPE_INT){
                        int dependency_lv_value = left_table_.table_data[dependency_lv_column].intvec[r1];
                        int dependency_rv_value = right_table_.table_data[dependency_rv_column].intvec[r2];
                        passed = compareByOperator(oper,dependency_lv_value,dependency_rv_value);
                    }else if(dependency_lv_type == TYPE_FLOAT){
                        float dependency_lv_value = left_table_.table_data[dependency_lv_column].floatvec[r1];
                        float dependency_rv_value = right_table_.table_data[dependency_rv_column].intvec[r2];
                        passed = compareByOperator(oper,dependency_lv_value,dependency_rv_value);
                    }

                    if(!passed) break;
                }

                if(passed){ 
                    for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
                        switch((*it).second.type){
                        case TYPE_INT:
                            dependency_subquery_filtered_table.table_data[(*it).first].intvec.push_back((*it).second.intvec[r2]);
                            break;
                        case TYPE_STRING:
                            dependency_subquery_filtered_table.table_data[(*it).first].strvec.push_back((*it).second.strvec[r2]);
                            break;
                        case TYPE_FLOAT:
                            dependency_subquery_filtered_table.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r2]);
                            break;
                        }
                        dependency_subquery_filtered_table.table_data[(*it).first].isnull.push_back((*it).second.isnull[r2]);
                        dependency_subquery_filtered_table.table_data[(*it).first].row_count++;
                    }
                    dependency_subquery_filtered_table.row_count++;
                }
            }
            
            TableData dependency_subquery_projection_table;
            RepeatedPtrField<string> dependency_alias;
            for(int a=0; a<snippet_.query_info().dependency().dependency_projection_size(); a++){
                string temp_colname = "depcol" + to_string(a);
                dependency_alias.Add()->assign(temp_colname);
            }
            Aggregation(dependency_subquery_filtered_table, snippet_.query_info().dependency().dependency_projection(), dependency_alias, dependency_subquery_projection_table);

            bool passed = false;

            if(dependency_subquery_projection_table.table_data.begin()->second.type == TYPE_EMPTY){
                passed = false;
            }else if(lv_type == TYPE_STRING){
                passed = compareByOperator(snippet_.query_info().filtering(0).operator_(), lv.varString, dependency_subquery_projection_table.table_data.begin()->second.strvec[0]);
            }else if(lv_type == TYPE_INT){
                if(dependency_subquery_projection_table.table_data.begin()->second.type == TYPE_INT){
                    passed = compareByOperator(snippet_.query_info().filtering(0).operator_(), lv.varInt, dependency_subquery_projection_table.table_data.begin()->second.intvec[0]);
                }else{
                    passed = compareByOperator(snippet_.query_info().filtering(0).operator_(), lv.varInt, dependency_subquery_projection_table.table_data.begin()->second.floatvec[0]);
                }
            }else{
                if(dependency_subquery_projection_table.table_data.begin()->second.type == TYPE_FLOAT){
                    passed = compareByOperator(snippet_.query_info().filtering(0).operator_(), lv.varFloat, dependency_subquery_projection_table.table_data.begin()->second.floatvec[0]);
                }else{
                    passed = compareByOperator(snippet_.query_info().filtering(0).operator_(), lv.varFloat, dependency_subquery_projection_table.table_data.begin()->second.intvec[0]);
                }
            }
            
            if(passed){
                for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                    switch((*it).second.type){
                    case TYPE_INT:
                        target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[r1]);
                        break;
                    case TYPE_STRING:
                        target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[r1]);
                        break;
                    case TYPE_FLOAT:
                        target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r1]);
                        break;
                    }
                    target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[r1]);
                    target_table_.table_data[(*it).first].row_count++;
                }
                target_table_.row_count++;
            }
        }

        row_index = left_table_.row_count;

        if(row_index == left_table_.row_count && left_table_.status == WorkDone){
            break;
        }
    }
}

void MergeQueryManager::DependencyExist(){
    int row_index = 0;
    bool first = true;

    right_table_ = BufferManager::GetFinishedTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(1));
    debug_table(2);

    bool is_not = (snippet_.query_info().filtering_size() != 0)? true : false;

    while(true){
        left_table_ = BufferManager::GetTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(0), row_index);
        debug_table(1);
    
        if(row_index == left_table_.row_count && left_table_.status == WorkDone){
            break;
        }else if(row_index == left_table_.row_count && left_table_.status != WorkDone){
            continue;
        }

        if(first){
            for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                target_table_.table_data[(*it).first].type = (*it).second.type;
            }
            first = false;
        }

        for(int r1=0; r1<left_table_.row_count; r1++){
            bool passed = false;
            bool exist;

            TableData dependency_subquery_filtered_table;
            for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
                dependency_subquery_filtered_table.table_data[(*it).first].type = (*it).second.type;
            }

            for(int r2=0; r2<right_table_.row_count; r2++){
                exist = false;

                for(int f=0; f<snippet_.query_info().dependency().dependency_filter_size(); f+=2){
                    string dependency_lv_column = snippet_.query_info().dependency().dependency_filter(f).lv().value(0);
                    string dependency_rv_column = snippet_.query_info().dependency().dependency_filter(f).rv().value(0);
                    int oper = snippet_.query_info().dependency().dependency_filter(f).operator_();
                    int dependency_lv_type = left_table_.table_data[dependency_lv_column].type;

                    if(dependency_lv_type == TYPE_STRING){
                        string dependency_lv_value = left_table_.table_data[dependency_lv_column].strvec[r1];
                        string dependency_rv_value = right_table_.table_data[dependency_rv_column].strvec[r2];
                        exist = compareByOperator(oper,dependency_lv_value,dependency_rv_value);
                    }else if(dependency_lv_type == TYPE_INT){
                        int dependency_lv_value = left_table_.table_data[dependency_lv_column].intvec[r1];
                        if(right_table_.table_data[dependency_rv_column].type == TYPE_INT){
                            int dependency_rv_value = right_table_.table_data[dependency_rv_column].intvec[r2];
                            exist = compareByOperator(oper,dependency_lv_value,dependency_rv_value);
                        }else{
                            float dependency_rv_value = right_table_.table_data[dependency_rv_column].intvec[r2];
                            exist = compareByOperator(oper,dependency_lv_value,dependency_rv_value);
                        }
                    }else if(dependency_lv_type == TYPE_FLOAT){
                        float dependency_lv_value = left_table_.table_data[dependency_lv_column].floatvec[r1];
                        if(right_table_.table_data[dependency_rv_column].type == TYPE_FLOAT){
                            float dependency_rv_value = right_table_.table_data[dependency_rv_column].intvec[r2];
                            exist = compareByOperator(oper,dependency_lv_value,dependency_rv_value);
                        }else{
                            int dependency_rv_value = right_table_.table_data[dependency_rv_column].intvec[r2];
                            exist = compareByOperator(oper,dependency_lv_value,dependency_rv_value);
                        }
                    }

                    if(!exist) break;
                }

                if(exist) break;
            }

            passed = (is_not)? !exist : exist;

            if(passed){
                for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                    switch((*it).second.type){
                    case TYPE_INT:
                        target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[r1]);
                        break;
                    case TYPE_STRING:
                        target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[r1]);
                        break;
                    case TYPE_FLOAT:
                        target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r1]);
                        break;
                    }
                    target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[r1]);
                    target_table_.table_data[(*it).first].row_count++;
                }
                target_table_.row_count++;
                continue;
            }
        }

        row_index = left_table_.row_count;

        if(row_index == left_table_.row_count && left_table_.status == WorkDone){
            break;
        }
    }
}    

void MergeQueryManager::DependencyIn(){
    int row_index = 0;
    bool first = true;
    
    right_table_ = BufferManager::GetFinishedTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(1));
    debug_table(2);

    bool is_not = (snippet_.query_info().filtering_size() != 0)? true : false;

    string lv_column = snippet_.query_info().filtering(0).lv().value(0);
    int lv_type = left_table_.table_data[lv_column].type;

    while(true){
        left_table_ = BufferManager::GetTableData(snippet_.query_id(),-1,snippet_.query_info().table_name(0), row_index);
        debug_table(1);
    
        if(row_index == left_table_.row_count && left_table_.status == WorkDone){
            break;
        }else if(row_index == left_table_.row_count && left_table_.status != WorkDone){
            continue;
        }

        if(first){
            for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                target_table_.table_data[(*it).first].type = (*it).second.type;
            }
            first = false;
        }

        for(int r1=0; r1<left_table_.row_count; r1++){
            T lv;
            lv.type = lv_type;
            if(lv_type == TYPE_STRING){
                lv.varString = left_table_.table_data[lv_column].strvec[r1];
            }else if(lv_type == TYPE_INT){
                lv.varInt = left_table_.table_data[lv_column].intvec[r1];
            }else{
                lv.varFloat = left_table_.table_data[lv_column].floatvec[r1];
            }

            TableData dependency_subquery_filtered_table;
            for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
                dependency_subquery_filtered_table.table_data[(*it).first].type = (*it).second.type;
            }

            for(int r2=0; r2<right_table_.row_count; r2++){
                bool passed = true;

                for(int f=0; f<snippet_.query_info().dependency().dependency_filter_size(); f+=2){
                    string dependency_lv_column = snippet_.query_info().dependency().dependency_filter(f).lv().value(0);
                    string dependency_rv_column = snippet_.query_info().dependency().dependency_filter(f).rv().value(0);
                    int oper = snippet_.query_info().dependency().dependency_filter(f).operator_();
                    int dependency_lv_type = left_table_.table_data[dependency_lv_column].type;

                    if(dependency_lv_type == TYPE_STRING){
                        string dependency_lv_value = left_table_.table_data[dependency_lv_column].strvec[r1];
                        string dependency_rv_value = right_table_.table_data[dependency_rv_column].strvec[r2];
                        passed = compareByOperator(oper,dependency_lv_value,dependency_rv_value);
                    }else if(dependency_lv_type == TYPE_INT){
                        int dependency_lv_value = left_table_.table_data[dependency_lv_column].intvec[r1];
                        if(right_table_.table_data[dependency_rv_column].type == TYPE_INT){
                            int dependency_rv_value = right_table_.table_data[dependency_rv_column].intvec[r2];
                            passed = compareByOperator(oper,dependency_lv_value,dependency_rv_value);
                        }else{
                            float dependency_rv_value = right_table_.table_data[dependency_rv_column].intvec[r2];
                            passed = compareByOperator(oper,dependency_lv_value,dependency_rv_value);
                        }
                    }else if(dependency_lv_type == TYPE_FLOAT){
                        float dependency_lv_value = left_table_.table_data[dependency_lv_column].floatvec[r1];
                        if(right_table_.table_data[dependency_rv_column].type == TYPE_FLOAT){
                            float dependency_rv_value = right_table_.table_data[dependency_rv_column].intvec[r2];
                            passed = compareByOperator(oper,dependency_lv_value,dependency_rv_value);
                        }else{
                            int dependency_rv_value = right_table_.table_data[dependency_rv_column].intvec[r2];
                            passed = compareByOperator(oper,dependency_lv_value,dependency_rv_value);
                        }
                    }

                    if(!passed) break;
                }

                if(passed){ 
                    for (auto it = right_table_.table_data.begin(); it != right_table_.table_data.end(); it++){
                        switch((*it).second.type){
                        case TYPE_INT:
                            dependency_subquery_filtered_table.table_data[(*it).first].intvec.push_back((*it).second.intvec[r2]);
                            break;
                        case TYPE_STRING:
                            dependency_subquery_filtered_table.table_data[(*it).first].strvec.push_back((*it).second.strvec[r2]);
                            break;
                        case TYPE_FLOAT:
                            dependency_subquery_filtered_table.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r2]);
                            break;
                        }
                        dependency_subquery_filtered_table.table_data[(*it).first].isnull.push_back((*it).second.isnull[r2]);
                        dependency_subquery_filtered_table.table_data[(*it).first].row_count++;
                    }
                    dependency_subquery_filtered_table.row_count++;
                }
            }
            
            TableData dependency_subquery_projection_table;
            RepeatedPtrField<string> dependency_alias;
            vector<string> alias_list;
            for(int a=0; a<snippet_.query_info().dependency().dependency_projection_size(); a++){
                string temp_colname = "depcol" + to_string(a);
                dependency_alias.Add()->assign(temp_colname);
                alias_list.push_back(temp_colname);
            }
            Aggregation(dependency_subquery_filtered_table, snippet_.query_info().dependency().dependency_projection(), dependency_alias, dependency_subquery_projection_table);
            
            createHashTable(dependency_subquery_filtered_table,alias_list);

            bool passed = false;
            if(lv_type == TYPE_STRING){
                string hash_key = "|" + lv.varString;
                bool in = (hash_table_.find(hash_key) != hash_table_.end());
                passed = (is_not)? !in : in;
            }else if(lv_type == TYPE_INT){
                string hash_key = "|" + to_string(lv.varInt);
                bool in = (hash_table_.find(hash_key) != hash_table_.end());
                passed = (is_not)? !in : in;
            }else{
                string hash_key = "|" + to_string(lv.varFloat);
                bool in = (hash_table_.find(hash_key) != hash_table_.end());
                passed = (is_not)? !in : in;
            }
            
            if(passed){
                for (auto it = left_table_.table_data.begin(); it != left_table_.table_data.end(); it++){
                    switch((*it).second.type){
                    case TYPE_INT:
                        target_table_.table_data[(*it).first].intvec.push_back((*it).second.intvec[r1]);
                        break;
                    case TYPE_STRING:
                        target_table_.table_data[(*it).first].strvec.push_back((*it).second.strvec[r1]);
                        break;
                    case TYPE_FLOAT:
                        target_table_.table_data[(*it).first].floatvec.push_back((*it).second.floatvec[r1]);
                        break;
                    }
                    target_table_.table_data[(*it).first].isnull.push_back((*it).second.isnull[r1]);
                    target_table_.table_data[(*it).first].row_count++;
                }
                target_table_.row_count++;
            }
        }

        row_index = left_table_.row_count;

        if(row_index == left_table_.row_count && left_table_.status == WorkDone){
            break;
        }
    }
}

void MergeQueryManager::createHashTable(TableData &hash_table, vector<string> equal_join_column){

    for (int i = 0; i < hash_table.row_count; i++){
        string hash_key = "";

        for(int j = 0; j < equal_join_column.size(); j++){
            string hash_key_column = equal_join_column[j];

            switch(hash_table.table_data[hash_key_column].type){
            case TYPE_STRING:
                hash_key += "|" + hash_table.table_data[hash_key_column].strvec[i];
                break;
            case TYPE_INT:
                hash_key += "|" + to_string(hash_table.table_data[hash_key_column].intvec[i]);
                break;
            case TYPE_FLOAT:
                hash_key += "|" + to_string(hash_table.table_data[hash_key_column].floatvec[i]);
                break;
            }
        }
        hash_table_[hash_key].push_back(i);
    }    
}

template <typename T, typename U>
bool MergeQueryManager::compareByOperator(int oper, const T& arg1, const U& arg2) {
    bool passed = false;

    if (oper == SnippetRequest_Filter::EQ) {
        passed = (static_cast<T>(arg1) == static_cast<T>(arg2));
    } else if (oper == SnippetRequest_Filter::GE) {
        passed = (static_cast<T>(arg1) >= static_cast<T>(arg2));
    } else if (oper == SnippetRequest_Filter::LE) {
        passed = (static_cast<T>(arg1) <= static_cast<T>(arg2));
    } else if (oper == SnippetRequest_Filter::GT) {
        passed = (static_cast<T>(arg1) > static_cast<T>(arg2));
    } else if (oper == SnippetRequest_Filter::LT) {
        passed = (static_cast<T>(arg1) < static_cast<T>(arg2));
    } else if (oper == SnippetRequest_Filter::NE) {
        passed = (static_cast<T>(arg1) != static_cast<T>(arg2));
    }
    
    return passed;
}
