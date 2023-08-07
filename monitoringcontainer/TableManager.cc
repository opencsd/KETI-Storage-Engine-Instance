#include "TableManager.h"

using namespace rapidjson;
//table manager 초기화
//테이블 스키마 정보 임의로 저장 >> 이후 DB File Monitoring을 통해 데이터 저장 -> 업데이트시 DB Connector Instance로 반영
int TableManager::initTableManager(){
	KETILOG::DEBUGLOG(LOGTAG, "# Init TableManager");

	//read TableManager.json
	string json = "";
	//std::ifstream openFile("/root/workspace/Storage-Engine-Instance/storage-engine-instance-container/monitoringcontainer/TableManager_tpch_small.json");
	std::ifstream openFile("/root/workspace/Storage-Engine-Instance/HUN/storage-engine-instance-container/monitoringcontainer/TableManager_template.json");
	if(openFile.is_open() ){
		std::string line;
		while(getline(openFile, line)){
			json += line;
		}
		openFile.close();
	}
	
	//parse json	
	Document document;
	document.Parse(json.c_str());

	Value &TableList = document["Table List"];
	
	//각 테이블당 행해지는 작업들 -> 테이블 단위로 실행
	for(int i=0;i<TableList.Size();i++){
		Value &TableObject = TableList[i];
		auto tbl = new Table(); 

		Value &tablenameObject = TableObject["tablename"]; 

		tbl->tablename = tablenameObject.GetString(); 
		tbl->PkExist = tablenameObject.GetBool(); //추가 : Table 구조체에 pk 존재 여부 저장
		// tbl.tablesize = TableObject["Size"].GetInt();

		// if(TableObject.HasMember("PK")){
		// 	for(int i = 0; i < TableObject["PK"].Size(); i++){
		// 		tbl.PK.push_back(TableObject["PK"][i].GetString());
		// 	}
		// }
		// if(TableObject.HasMember("Index")){
		// 	for(int i = 0; i < TableObject["Index"].Size(); i++){
		// 		vector<string> index;
		// 		for(int j = 0; j < TableObject["Index"][i].Size(); j++){
		// 			index.push_back(TableObject["Index"][i][j].GetString());
		// 		}
		// 		tbl.IndexList.push_back(index);
		// 	}
		// }
		
		Value &SchemaObject = TableObject["Schema"];
		for(int j=0;j<SchemaObject.Size();j++){
			Value &ColumnObject = SchemaObject[j];
			auto Column = new ColumnSchema;

			//column 구조체에 컬럼 관련 정보들 저장 
			Column->column_name = ColumnObject["column_name"].GetString();
			Column->type = ColumnObject["type"].GetInt();
			Column->length = ColumnObject["length"].GetInt();
			Column->offset = ColumnObject["offset"].GetInt();
			
			Column->isPk = ColumnObject["isPk"].GetBool(); //추가 : 해당 컬럼이 pk인지의 여부
			Column->isIndex = ColumnObject["isIndex"].GetBool(); //추가 : 해당 컬럼이 index인지의 여부
			if (Column->isPk == true){ //pk가 존재한다면, 해당 pk에 대한 컬럼 이름 및 바이트 사이즈 저장
				tbl->PkColumnNames.push_back(Column->column_name);
				tbl->PkColumnBytes.push_back(Column->type*Column->length); //바이트 사이즈를 어떤 식으로 정해야 하나..
				tbl->PkCnt++;
			}
			if (Column->isIndex == true){ //index가 존재한다면, 해당 index에 대한 컬럼 이름 및 바이트 사이즈 저장
				tbl->IndexColumnNames.push_back(Column->column_name);
				tbl->IndexColumnBytes.push_back(Column->type*Column->length); //바이트 사이즈를 어떤 식으로 정해야 하나..
				tbl->IndexCnt++;
			}			
			//column 구조체를 테이블 구조체의 컬럼스키마를 모아두는 스키마 벡터에 푸시백
			tbl->Schema.push_back(*Column);
		}

		Value &SSTList = TableObject["SST List"];
		for(int j=0;j<SSTList.Size();j++){ 
			Value &SSTObject = SSTList[j];
			auto SstFile = new SSTFile;
			map<string, vector<string>> indexScanMap; //추가 : index table map

			//data block handle 저장 과정
			Value &BlockList = SSTObject["Block List"]; 
			for(int k=0;k<BlockList.Size();k++){
				Value &BlockHandleObject = BlockList[k];
				struct DataBlockHandle DataBlockHandle; 
				//index block handle 
				// if(BlockHandleObject.HasMember("IndexBlockHandle")){ 
				// 	DataBlockHandle.IndexBlockHandle = BlockHandleObject["IndexBlockHandle"].GetString();
				// }
				DataBlockHandle.IndexBlockHandle = BlockHandleObject["IndexBlockHandle"].GetString(); //추가 : sst 파일 내부의 인덱스 블록들의 block handle
				DataBlockHandle.Offset = BlockHandleObject["Offset"].GetInt();
				DataBlockHandle.Length = BlockHandleObject["Length"].GetInt();

				SstFile->BlockList.push_back(DataBlockHandle);
			}

			//index table 생성 과정 -> 각 테이블 단위로 진행되어야 함
			SstFile->filename = SSTObject["filename"].GetString(); //sst file 이름 가져오기
			//const char *file_path = "/root/workspace/HUN/csd_file_scan/keti/sst/" + SstFile->filename; 
			std::string file_path = "/root/workspace/HUN/csd_file_scan/keti/sst/" + SstFile->filename; //2. 임의의 경로 지정
			//const char *file_path = filename.c_str();  //1. 직접 입력받기
			Options options; 
			ReadOptions readOptions;
			SstFileReader sstFileReader(options); 
			Status s = sstFileReader.Open(file_path.c_str()); //const char* 형으로 변환 -> 일부 C 스타일의 API나 라이브러리에서는 const char* 형식의 C 스타일 문자열을 요구함
			// Status s = sstFileReader.KetiOpen(file_path);
			Iterator* it = sstFileReader.NewIterator(readOptions); 
			if(!s.ok()){
			std::cout << "open error" << std::endl;
			}

			

			SstFile->IndexTable = indexScanMap; //sst file에 index table 추가
			tbl->SSTList.push_back(*SstFile);
		}
		m_TableManager.insert({tbl->tablename,*tbl});

		//index table 생성 과정 -> 각 테이블 단위로 시행되어야 함
		//std::string filename = argv[1];  
		//필요한 정보들이 담겨있는 구조체 예시
		// struct tableMetaData { 
		//     string tableIndexNum = "0000018B";
		//     bool pkExist = true;
		//     int indexCnt = 2;
		//     int pkCnt = 2;
		//     //각 index와 pk 컬럼들에 대한 길이 정보
		//     vector<string> indexColumnNames = {"id", "age"};
		//     vector<string> pkColumnNames = {"age", "id"};
		//     vector<int> indexColumnBytes = {4,4}; 
		//     vector<int> pkColumnBytes = {4,4};
		// };

		//index table 생성 과정 -> 각 테이블 단위로 진행되어야 함
		
		//table 생성 전 데이터 세팅 과정 
		//data block handle이 사용되어야 할까?
		bool pkExist = tbl->PkExist;
		int indexCnt = tbl->IndexCnt;
		int pkCnt = tbl->PkCnt;
		vector<string> indexColumnNames = tbl->IndexColumnNames;
		vector<string> pkColumnNames = tbl->PkColumnNames;
		vector<int> indexColumnBytes = tbl->IndexColumnBytes;
		vector<int> pkColumnBytes = tbl->PkColumnBytes;
		
		std:string = BlockList[j]["IndexBlockHandle"].GetString();
		//sst 파일 읽어서 seek 진행
		for (it->SeekToFirst(); it->Valid(); it->Next()) { //sst file에서 key, value 가져오기
			//std::cout << "Key: " << it->key().ToString(true) << ", Value: " << it->value().ToString(true) << std::endl;
			const Slice& key = it->key();
			const Slice& value = it->value();     
			string indexList = " ";
			//vector<std::string> pkList;
			vector<std::string> noPkIndexList;     
			int BlockOffset = 8; //블록 첫 부분의 table index num은 4바이트 고정 -> 시작 오프셋은 4x2=8
			
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
				pk.push_back();

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

		/*
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
		*/
	}
	return 0;
}



//이미 JSON 파일을 파싱하는 과정으로 데이터를 저장 후, 원하는 데이터들을 뽑아내기 위해서 아래 함수들을 정의해둠
//IndexTblGenManager에서 TableManager에서 세팅해둔 데이터를 사용하기 위해서 해당 함수들로 값을 가져올 수 있음
//어떤 함수가 더 추가되어야 할지?

/*
void TableManager::getIndexScanMap(struct &table){ //각 테이블에 들어있는 index scan table 맵 뽑기 -> 테이블 구조체를 인자로 받음
	struct Table tbl = table;
	vector<struct SSTFile> SstList = tbl.SSTList;
	for(int i = 0; i < SstList.size(); i++){
		struct SstFile = SstList[i];
		for (const auto& entry : SstFile.IndexTable) {
			const std::string& key = entry.first;
			const std::vector<std::string>& values = entry.second;
			std::cout << "Key: " << key << ", Values: ";
			for (const auto& value : values) {
				std::cout << value << " ";
			}
			std::cout << std::endl;
		}
	}
}
*/

vector<string> TableManager::getSSTList(string tablename){
	vector<string> sstlist;
	for(int i = 0; i < m_TableManager[tablename].SSTList.size(); i++){
		sstlist.push_back(m_TableManager[tablename].SSTList[i].filename);
	}
	return sstlist;
}

int TableManager::getTableSchema(std::string tablename,vector<struct ColumnSchema> &dst){	
    if (m_TableManager.find(tablename) == m_TableManager.end()){ //맵에 일치하는 테이블 이름이 없을 때
		KETILOG::FATALLOG(LOGTAG,"Table [" + tablename + "] Not Present"); //해당 테이블이 존재하지 않음
		    return -1;
	}

	struct Table tbl = m_TableManager[tablename];
	dst = tbl.Schema;
	return 0;
}

vector<string> TableManager::getOrderedTableBySize(vector<string> tablenames){
	vector<pair<string,int>> tablesize;
	for(int i = 0; i < tablenames.size(); i++){
		struct Table tbl = m_TableManager[tablenames[i]];
		tablesize.push_back(make_pair(tbl.tablename,tbl.tablesize));
	}
	sort(tablesize.begin(),tablesize.end(),[](pair<string,int> a, pair<string,int> b){
		return a.second < b.second;
	});
	vector<string> ret;
	for(int i = 0; i < tablesize.size(); i++){
		ret.push_back(tablesize[i].first);
	}
	return ret;
}

int TableManager::getIndexList(string tablename, vector<string> &dst){
	if (m_TableManager.find(tablename) == m_TableManager.end()){
		KETILOG::FATALLOG(LOGTAG,"Table [" + tablename + "] Not Present");
        return -1;
	}
	struct Table tbl = m_TableManager[tablename];
	dst = tbl.IndexList;
	return 0;
}

void TableManager::printTableManager(){
	unordered_map <string,struct Table> :: iterator it;

	for(it = m_TableManager.begin(); it != m_TableManager.end() ; it++){		
		struct Table tmp = it->second;
		std::cout << "tablename : " << tmp.tablename << std::endl;
		std::cout << "Column name     Type  Length Offset" << std::endl;
		vector<struct ColumnSchema>::iterator itor = tmp.Schema.begin();

		for (; itor != tmp.Schema.end(); itor++) {
			std::cout << left << setw(15) << (*itor).column_name << " " << left<< setw(5) << (*itor).type << " " << left << setw(6) <<(*itor).length << " " << left << setw(5) << (*itor).offset << std::endl;
		}

		vector<struct SSTFile>::iterator itor2 = tmp.SSTList.begin();

		for (; itor2 != tmp.SSTList.end(); itor2++) {
			std::cout << "SST filename : " << (*itor2).filename << std::endl;

			vector<struct DataBlockHandle>::iterator itor3 = (*itor2).BlockList.begin();
			std::cout << " StartOffset-Length" << endl;
			for (; itor3 != (*itor2).BlockList.end(); itor3++) {
				std::cout << "Block Handle : " << (*itor3).Offset << "-" << (*itor3).Length << std::endl;				
			}
		}
	}
}