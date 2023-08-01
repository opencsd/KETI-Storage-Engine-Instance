#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <unistd.h>



struct QueryDuration {
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
};

struct CPU_Usage {
    unsigned long long start_cpu;
    unsigned long long end_cpu;
};

struct CPU_Power {
    double start_CPUPower0;
    double start_CPUPower1;
    double end_CPUPower0;
    double end_CPUPower1;
};

struct Net_Usage {
    unsigned long long start_rxByte;
    unsigned long long start_txByte;
    unsigned long long end_rxByte;
    unsigned long long end_txByte;
};

struct Metric_Result {
    float cpu_usage;
    float power_usage;
    unsigned long long network_usage;
};

// CPU 총 사용량 구하는 함수
unsigned long long get_cpu_usage() {
    std::ifstream proc_stat("/proc/stat");
    std::string line;
    std::getline(proc_stat, line); // 첫 번째 라인은 'cpu' 정보
    std::istringstream iss(line);
    std::string cpuLabel;
    iss >> cpuLabel; // 'cpu' 레이블 버리기

    // 'cpu' 이후의 값들을 읽어와서 사용량 계산
    long user, nice, sys, idle, iowait, irq, softirq, steal, guest, guest_nice;
    iss >> user >> nice >> sys >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;

    proc_stat.close();

    // 현재 시점의 CPU total time 계산
    unsigned long long total_time = 0;
    total_time = user + nice + sys + idle + iowait + irq + softirq + steal + guest + guest_nice;

    return total_time;
}

// 쿼리 시간 동안 CPU 평균 사용량 구하는 함수
float get_CPU_Usage_Percentage(const CPU_Usage& cpu_usage, const QueryDuration& querydur) {
    unsigned long long total_cpu = cpu_usage.end_cpu - cpu_usage.start_cpu;
    if(total_cpu == 0) {
        printf("get CPU Usage error!");
        return -1;
    }
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(querydur.end_time - querydur.start_time);
    float cpu_usage_percentage = (static_cast<double>(total_cpu) / sysconf(_SC_CLK_TCK)) / duration.count() * 100.0;

    return cpu_usage_percentage;
}

// Power 사용량 구하는 함수
void get_CPU_Power(double power0, double power1) {
    // 0번째 CPU_Power
    std::ifstream file("/sys/class/powercap/intel-rapl:0:0/energy_uj");
    std::string line;
    std::getline(file, line);
    file.close();
    power0 = std::stoull(line) / 1000000.0;

    // 1번째 CPU_Power
    std::ifstream file("/sys/class/powercap/intel-rapl:1:0/energy_uj");
    std::string line;
    std::getline(file, line);
    file.close();
    power1 = std::stoull(line) / 1000000.0;

}

// Network 사용량 
void get_Network_Usage(unsigned long long rxByte, unsigned long long txByte) {
    std::ifstream file("/proc/net/dev");
    if (!file) {
        std::cerr << "Failed to open /proc/net/dev" << std::endl;
    }

    std::string line;
    std::string name;

    while (getline(file, line)){
        if(line.find("eno1") != std::string::npos){
            std::istringstream iss(line);
            iss >> name >> rxByte;
            for (int i = 3; i < 11; i++){
                iss >> txByte;
            }
            break;
        }
    }
}

void QueryStart(QueryDuration& querydu, CPU_Usage& cpu_usg, CPU_Power& cpu_power, Net_Usage& net_usg){
    
    // 쿼리 시작 시점 저장
    querydu.start_time = std::chrono::high_resolution_clock::now();
    
    // 쿼리 시작 시점 cpu 사용량 저장
    cpu_usg.start_cpu = get_cpu_usage();

    //쿼리 시작 시점 cpu power 저장
    get_CPU_Power(cpu_power.start_CPUPower0, cpu_power.start_CPUPower1);

    //쿼리 시작 시점 net 값 저장
    get_Network_Usage(net_usg.start_rxByte, net_usg.start_txByte);
}

void QueryEnd(){
    //쿼리 끝 시점 저장
    //쿼리 끝 시점 cpu 사용량 저장
    //쿼리 수행 시간 동안의 cpu 사용량 계산

    //쿼리 끝 시점 cpu power 저장
    //쿼리 수행 시간 동안의 cpu power 계산

    //쿼리 끝 시점 net 값 저장
    //쿼리 수행 시간 동안의 net 계산
}

int main(){
    /* 쿼리 시작 */
    //interface container에서 통신으로 쿼리 시작 받음

    CPU_Power cpu_power;
    QueryDuration querydu;
    CPU_Usage cpu_usg;
    Net_Usage net_usg;

    QueryStart(querydu, cpu_usg, cpu_power, net_usg);
    


    /* 쿼리 끝 */
    //interface container에서 통신으로 쿼리 시작 받음

    
    

}