#pragma once

#include "storage_engine_instance.grpc.pb.h"

#include "keti_log.h"
#include "ip_config.h"

using namespace std;

class CSDStatusManager { 

public:
	struct CSDInfo {
        string csd_ip;
        float csd_score = 0;
        int block_count = 0;
    };

	static int SetCSDInfo(string id, CSDInfo csd_info){
		return GetInstance().setCSDInfo(id, csd_info);
	}

	static CSDInfo GetCSDInfo(string id){
		return GetInstance().getCSDInfo(id);
	}

    static unordered_map<string,struct CSDInfo> GetCSDInfoAll(){
		return GetInstance().getCSDInfoAll();
	}
    
private:
	CSDStatusManager() {};
    CSDStatusManager(const CSDStatusManager&);
    CSDStatusManager& operator=(const CSDStatusManager&){
        return *this;
    };
    
    int setCSDInfo(string id, CSDInfo csd_info){
        GetInstance().m_CSDInfo[id] = csd_info;
    }

    CSDInfo getCSDInfo(string id){
        return GetInstance().m_CSDInfo[id];
    }

    unordered_map<string,struct CSDInfo> getCSDInfoAll(){
        return GetInstance().m_CSDInfo;
    }

    static CSDStatusManager& GetInstance() {
        static CSDStatusManager _instance;
        return _instance;
    }


/* Variables */
public:
    inline const static string LOGTAG = "Offloading Container::CSD Status Manager";

private:
    mutex Status_Mutex;
	unordered_map<string,struct CSDInfo> m_CSDInfo;//key:id, value:CSDInfo
};

