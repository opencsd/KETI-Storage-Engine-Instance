#pragma once

#include <curl/curl.h>
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

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

    static CSDStatusManager& GetInstance() {
        static CSDStatusManager _instance;
        return _instance;
    }

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
	CSDStatusManager() {
        std::thread csd_stastus_pull_thread_(&CSDStatusManager::pullCycle,this);
        csd_stastus_pull_thread_.detach();
    };
    CSDStatusManager(const CSDStatusManager&);
    CSDStatusManager& operator=(const CSDStatusManager&){
        return *this;
    };

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp){
        std::string* buffer = static_cast<std::string*>(userp);
        buffer->append(static_cast<char*>(contents), size * nmemb);
        return size * nmemb;
    }

    void pullCycle(){
        while(true){
            pullCSDStatus();
            // for (const auto& kv : CSDStatusManager_) {
            //     const std::string& key = kv.first;
            //     const CSDInfo& info = kv.second;

            //     std::cout << "Key (CSD ID): " << key << "\n"
            //             << "  IP: " << info.csd_ip << "\n"
            //             << "  CPU: " << info.cpu_usage << "\n"
            //             << "  Memory: " << info.memory_usage << "\n"
            //             << "  Disk: " << info.disk_usage << "\n"
            //             << "  Network: " << info.network << "\n"
            //             << "  Working Block: " << info.working_block_count << "\n"
            //             << "  Score: " << info.analysis_score << "\n"
            //             << std::endl;
            // }
            sleep(5);
        }
    }

    void pullCSDStatus(){
        CURL* curl;
        CURLcode res;
        std::string readBuffer;

        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();

        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, "http://10.0.4.83:40306/storage/metric/all");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "[ERROR] curl_easy_perform() failed: "
                        << curl_easy_strerror(res) << std::endl;
            }
            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();

        rapidjson::Document doc;
        doc.Parse(readBuffer.c_str());

        if (doc.HasParseError()) {
            std::cerr << "[ERROR] JSON parse error at offset "
                    << doc.GetErrorOffset() << ": "
                    << rapidjson::GetParseError_En(doc.GetParseError()) << std::endl;
            return;
        }

        if (!doc.HasMember("csdList")) {
            std::cerr << "[ERROR] JSON does not contain 'csdList' key.\n";
            return;
        }

        const rapidjson::Value& csdList = doc["csdList"];
        if (!csdList.IsObject()) {
            std::cerr << "[ERROR] 'csdList' is not an object.\n";
            return;
        }

        for (auto itr = csdList.MemberBegin(); itr != csdList.MemberEnd(); ++itr) {
            const char* csdKey = itr->name.GetString();
            const rapidjson::Value& csdArray = itr->value;

            if (!csdArray.IsArray()) {
                std::cerr << "[ERROR] Value of " << csdKey << " is not an array.\n";
                continue;
            }

            for (rapidjson::SizeType i = 0; i < csdArray.Size(); i++) {
                const auto& elem = csdArray[i];
                if (!elem.IsObject()) {
                    std::cerr << "[WARNING] Element in " << csdKey << " is not an object.\n";
                    continue;
                }

                CSDInfo info;
                if (elem.HasMember("ip") && elem["ip"].IsString()) {
                    info.csd_ip = elem["ip"].GetString();
                } else {
                    info.csd_ip = "";
                }

                if (elem.HasMember("cpuUtilization") && elem["cpuUtilization"].IsNumber()) {
                    info.cpu_usage = static_cast<float>(elem["cpuUtilization"].GetDouble());
                } else {
                    info.cpu_usage = 0.0f;
                }

                if (elem.HasMember("memoryUtilization") && elem["memoryUtilization"].IsNumber()) {
                    info.memory_usage = static_cast<float>(elem["memoryUtilization"].GetDouble());
                } else {
                    info.memory_usage = 0.0f;
                }

                if (elem.HasMember("diskUtilization") && elem["diskUtilization"].IsNumber()) {
                    info.disk_usage = static_cast<float>(elem["diskUtilization"].GetDouble());
                } else {
                    info.disk_usage = 0.0f;
                }

                if (elem.HasMember("networkBandwidth") && elem["networkBandwidth"].IsNumber()) {
                    info.network = static_cast<float>(elem["networkBandwidth"].GetDouble());
                } else {
                    info.network = 0.0f;
                }

                if (elem.HasMember("csdWorkingBlockCount") && elem["csdWorkingBlockCount"].IsNumber()) {
                    info.working_block_count = static_cast<float>(elem["csdWorkingBlockCount"].GetDouble());
                } else {
                    info.working_block_count = 0.0f;
                }

                if (elem.HasMember("csdMetricScore") && elem["csdMetricScore"].IsNumber()) {
                    info.analysis_score = static_cast<float>(elem["csdMetricScore"].GetDouble());
                } else {
                    info.analysis_score = 0.0f;
                }

                std::string csd_id;
                if (elem.HasMember("id") && elem["id"].IsString()) {
                    csd_id = elem["id"].GetString();
                } else {
                    csd_id = csdKey;
                }

                CSDStatusManager_[csd_id] = info;
            }
        }
    }
    
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
    thread csd_stastus_pull_thread_;
};
