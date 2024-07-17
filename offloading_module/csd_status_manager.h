#pragma once

#include "storage_engine_instance.grpc.pb.h"

#include "keti_log.h"
#include "ip_config.h"

using StorageEngineInstance::CSDMetricList;
using StorageEngineInstance::CSDMetricList_CSDMetric;
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

    static CSDMetricList T_get_csd_status(){
        return GetInstance().t_get_csd_status();
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

    CSDMetricList t_get_csd_status(){
        CSDMetricList csd_metric_list;

        for(auto& status : CSDStatusManager_){
            CSDMetricList_CSDMetric csd_metric;
            csd_metric.set_id(status.first);
            csd_metric.set_cpu_usage(status.second.cpu_usage);
            csd_metric.set_memory_usage(status.second.memory_usage);
            csd_metric.set_disk_usage(status.second.disk_usage);
            csd_metric.set_network(status.second.network);
            csd_metric.set_working_block_count(status.second.working_block_count);
            csd_metric.set_score(status.second.analysis_score);

            csd_metric_list.add_csd_metric_list()->CopyFrom(csd_metric);
        }

        return csd_metric_list;
    }

/* Variables */
public:
    inline const static string LOGTAG = "Offloading::CSD Status Manager";

private:
    std::mutex mutex_;
	unordered_map<string,struct CSDInfo> CSDStatusManager_;//key:id, value:CSDInfo
};
