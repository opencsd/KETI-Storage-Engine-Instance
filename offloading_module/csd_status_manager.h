#pragma once

#include "storage_engine_instance.grpc.pb.h"

#include "keti_log.h"
#include "ip_config.h"

using namespace std;

class CSDStatusManager { 

public:
	struct CSDInfo {
        string csd_ip;
        float cpu_usage;
        float memory_usage;
        float disk_usage;
        float network;
        float working_block_count;
        float analysis_score;
    };

	static void SetCSDInfo(string id, CSDInfo csd_info){
		return GetInstance().setCSDInfo(id, csd_info);
	}

	static CSDInfo GetCSDInfo(string id){
		return GetInstance().getCSDInfo(id);
	}

    static bool isCSDIDAvailable(string id){
        return !(GetInstance().CSDStatusManager_.find(id) == GetInstance().CSDStatusManager_.end());
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
    
    void setCSDInfo(string id, CSDInfo csd_info){
        std::lock_guard<std::mutex> lock(mutex_);
        GetInstance().CSDStatusManager_[id] = csd_info;
    }

    CSDInfo getCSDInfo(string id){
        std::lock_guard<std::mutex> lock(mutex_);
        return GetInstance().CSDStatusManager_[id];
    }

    unordered_map<string,struct CSDInfo> getCSDInfoAll(){
        return GetInstance().CSDStatusManager_;
    }

    static CSDStatusManager& GetInstance() {
        static CSDStatusManager _instance;
        return _instance;
    }

/* Variables */
public:
    inline const static string LOGTAG = "Offloading::CSD Status Manager";

private:
    std::mutex mutex_;
	unordered_map<string,struct CSDInfo> CSDStatusManager_;//key:id, value:CSDInfo
};

