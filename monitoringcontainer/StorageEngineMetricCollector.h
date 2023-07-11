#pragma once

#include "storage_engine_instance.grpc.pb.h"

#include "keti_log.h"
#include "ip_config.h"

using namespace std;

class StorageEngineMetricCollector { 

public:
	struct CSDMetric {
        string ip = "";
        float cpu_usage = 0;
        float mem_usage = 0;
        float disk_usage = 0;
        float network = 0;
        int block_count = 0;
    };

	static int SetCSDMetric(string id, CSDMetric csd_info){
		return GetInstance().setCSDMetric(id, csd_info);
	}

	static CSDMetric GetCSDMetric(string id){
		return GetInstance().getCSDMetric(id);
	}

    static unordered_map<string,struct CSDMetric> GetCSDMetricAll(){
		return GetInstance().getCSDMetricAll();
	}
    
private:
	StorageEngineMetricCollector() {};
    StorageEngineMetricCollector(const StorageEngineMetricCollector&);
    StorageEngineMetricCollector& operator=(const StorageEngineMetricCollector&){
        return *this;
    };
    
    int setCSDMetric(string id, CSDMetric csd_info){
        GetInstance().m_MetricCollector[id] = csd_info;
    }

    CSDMetric getCSDMetric(string id){
        return GetInstance().m_MetricCollector[id];
    }

    unordered_map<string,struct CSDMetric> getCSDMetricAll(){
        return GetInstance().m_MetricCollector;
    }

    static StorageEngineMetricCollector& GetInstance() {
        static StorageEngineMetricCollector _instance;
        return _instance;
    }


/* Variables */
public:
    inline const static string LOGTAG = "Monitoring Container::Storage Engine Metric Collector";

private:
    mutex Collector_Mutex;
	unordered_map<string,struct CSDMetric> m_MetricCollector;//key:id, value:CSDMetric
};

