#pragma once
#include <thread>

#include "StorageEngineMetricCollector.h"
#include "OffloadingContainerInterface.h"

#include "storage_engine_instance.grpc.pb.h"

#include "keti_log.h"
#include "ip_config.h"

using namespace std;

class MetricAnalysisModule { 

public:
    static MetricAnalysisModule& GetInstance() {
        static MetricAnalysisModule _instance;
        return _instance;
    }

	static void SetCSDStatus(string id, CSDStatus csd_status){
		return GetInstance().setCSDStatus(id, csd_status);
	}

	static CSDStatus GetCSDStatus(string id){
		return GetInstance().getCSDStatus(id);
	}

    static unordered_map<string,struct CSDStatus> GetCSDStatusAll(){
		return GetInstance().getCSDStatusAll();
	}

private:
	MetricAnalysisModule() {
        std::thread Metric_Analysis_Thread(&MetricAnalysisModule::runMetricAnalysis,this);
        std::thread Metric_Push_Thread(&MetricAnalysisModule::runMetricPush,this);
        Metric_Analysis_Thread.detach();
        Metric_Push_Thread.detach();
    };
    MetricAnalysisModule(const MetricAnalysisModule&);
    MetricAnalysisModule& operator=(const MetricAnalysisModule&){
        return *this;
    };
    
    void setCSDStatus(string id, CSDStatus csd_status){
        GetInstance().m_MetricAnalysisModule[id] = csd_status;
    }

    CSDStatus getCSDStatus(string id){
        return GetInstance().m_MetricAnalysisModule[id];
    }

    unordered_map<string,struct CSDStatus> getCSDStatusAll(){
        return GetInstance().m_MetricAnalysisModule;
    }

    void runMetricAnalysis(){
        while (true){
            unordered_map<string,StorageEngineMetricCollector::CSDMetric> csdMetricList;
            csdMetricList = StorageEngineMetricCollector::GetCSDMetricAll();
            
            for (const auto pair : csdMetricList) {
                string id = pair.first;
                float score = 100 - ((cpu_weight * pair.second.cpu_usage) + (memory_weight * pair.second.mem_usage));

                CSDStatus csd_status;
                csd_status.ip = pair.second.ip;
                csd_status.score = score;
                // csd_status.block_count = pair.second.block_count;
                SetCSDStatus(id,csd_status);
            }

            sleep(3);
        }
    }

    void runMetricPush(){
        while (true){
            unordered_map<string,CSDStatus> csdStatusList;
            csdStatusList = MetricAnalysisModule::GetCSDStatusAll();
            
            Offloading_Container_Interface oc(grpc::CreateChannel((std::string)LOCALHOST+":"+std::to_string(SE_OFFLOADING_CONTAINER_PORT), grpc::InsecureChannelCredentials()));
            oc.PushCSDMetric(csdStatusList);
            sleep(3);
        }
    }

public:
    inline const static string LOGTAG = "Monitoring Container::Storage Engine Metric Collector";

private:
    mutex Analysis_Mutex;
	unordered_map<string,struct CSDStatus> m_MetricAnalysisModule;//key:id, value:CSDStatus
    thread Metric_Analysis_Thread;//Running Metric Analysis
    thread Metric_Push_Thread;//Running Metric Push to CSDStatusManager
    float cpu_weight = 0.5;
    float memory_weight = 0.5;
    float disk_weight = 0.0;
    float network_weight = 0.0;
};

