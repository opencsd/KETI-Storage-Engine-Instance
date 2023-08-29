#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>

#define PORT 8080


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
    unsigned long long rx;
    unsigned long long tx;
};

struct result_Net_Usage {
    unsigned long long start_rx;
    unsigned long long start_tx;
    unsigned long long end_rx;
    unsigned long long end_tx;
};

struct Metric_Result {
    double cpu_usage;
    double power_usage;
    unsigned long long network_usage;
};

// CPU 총 사용량 구하는 함수
unsigned long long get_cpu_usage() {
    // std::cout << "get_cpu_usage start!" << std::endl;
    std::ifstream proc_stat("/proc/stat");
    std::string line;
    std::getline(proc_stat, line); // 첫 번째 라인은 'cpu' 정보
    std::istringstream iss(line);
    std::string cpuLabel;
    iss >> cpuLabel; // 'cpu' 레이블 버리기

    // 'cpu' 이후의 값들을 읽어와서 사용량 계산
    unsigned long long user, nice, sys, idle, iowait, irq, softirq, steal, guest, guest_nice;
    iss >> user >> nice >> sys >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;

    proc_stat.close();

    // 현재 시점의 CPU total time 계산
    unsigned long long total_time = 0;
    total_time = user + nice + sys + idle + iowait + irq + softirq + steal + guest + guest_nice;

   // std::cout << "get_cpu_usage : " << total_time << std::endl;

    return total_time;
}

// 쿼리 시간 동안 CPU 평균 사용량 구하는 함수
double get_CPU_Usage_Percentage(const CPU_Usage& cpu_usage, const QueryDuration& querydur) {
    // std::cout << "get_CPU_Usage_Percentage start!" << std::endl;
    double total_cpu = cpu_usage.end_cpu - cpu_usage.start_cpu;
    if(total_cpu <= 0) {
        printf("get CPU Usage error!");
        return -1;
    }
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(querydur.end_time - querydur.start_time);
    double cpu_usage_percentage = (static_cast<double>(total_cpu) / sysconf(_SC_CLK_TCK)) / duration.count() * 100.0;

    //std::cout << "get_CPU_Usage_Percentage : " << cpu_usage_percentage << std::endl;
    
    return cpu_usage_percentage;
}

// Power 사용량 구하는 함수
unsigned long long get_CPU_Power(const std::string& filePath) {
    // std::cout << "get_CPU_Power start!" << std::endl;
    std::ifstream file(filePath);
    std::string line;
    std::getline(file, line);
    file.close();
    unsigned long long cpu_power;
    cpu_power = std::stoull(line) / 1000000.0;

    //std::cout << "get_CPU_Power : " << cpu_power << std::endl;

    return cpu_power;
}

// Network 사용량 
void get_Network_Usage(Net_Usage& net_usg) {
    // std::cout << "get_Network_Usage start!" << std::endl;
    // Net_Usage net_usg;
    unsigned long long rxByte;
    unsigned long long txByte;
    std::ifstream file("/proc/net/dev");
    if (!file) {
        std::cerr << "Failed to open /proc/net/dev" << std::endl;
    }

    std::string line;
    std::string name;

    while (getline(file, line)){
        if(line.find("enp96s0f0") != std::string::npos){
            std::istringstream iss(line);
            iss >> name >> rxByte;
            // std::cout << rxByte << std::endl;
            for (int i = 3; i < 11; i++){
                iss >> txByte;
                //std::cout << txByte << std::endl;
            }
            break;
        }
    }

    net_usg.rx = rxByte;
    net_usg.tx = txByte;

    //std::cout << "get_Network_Usage : " << rxByte << "   " << txByte << std::endl;

    // return net_usg;
}

void QueryStart(QueryDuration& querydu, CPU_Usage& cpu_usg, CPU_Power& cpu_power, result_Net_Usage& net_usg){
    // 쿼리 시작 시점 저장
    querydu.start_time = std::chrono::high_resolution_clock::now();
    
    // 쿼리 시작 시점 cpu 사용량 저장
    cpu_usg.start_cpu = get_cpu_usage();

    //쿼리 시작 시점 cpu power 저장
    cpu_power.start_CPUPower0 = get_CPU_Power("/sys/class/powercap/intel-rapl:0/energy_uj");
    cpu_power.start_CPUPower1 = get_CPU_Power("/sys/class/powercap/intel-rapl:1/energy_uj");

    //쿼리 시작 시점 net 값 저장
    Net_Usage temp_net;
    get_Network_Usage(temp_net);
    net_usg.start_rx = temp_net.rx;
    net_usg.start_tx = temp_net.tx;

    //std::cout << "---------metric check start----------" << std::endl;
    //std::cout << "cpu 사용량 : " << cpu_usg.start_cpu << std::endl;
    //std::cout << "cpu power1 : " << cpu_power.start_CPUPower0 << std::endl;
    //std::cout << "cpu power2 : " << cpu_power.start_CPUPower1 << std::endl;
    //std::cout << "Net 사용량 rx : " << net_usg.start_rx << std::endl;
    //std::cout << "Net 사용량 tx : " << net_usg.start_tx << std::endl;
}

void QueryEnd(QueryDuration& querydu, CPU_Usage& cpu_usg, CPU_Power& cpu_power, result_Net_Usage& net_usg, Metric_Result& result){
    //쿼리 끝 시점 저장
    querydu.end_time = std::chrono::high_resolution_clock::now();
    
    //쿼리 끝 시점 cpu 사용량 저장
    cpu_usg.end_cpu = get_cpu_usage();

    //쿼리 수행 시간 동안의 cpu 사용량 계산
    result.cpu_usage = get_CPU_Usage_Percentage(cpu_usg, querydu);
    if(result.cpu_usage < 0){
        printf("CPU_Percentage error!\n");
    }

    //쿼리 끝 시점 cpu power 저장
    cpu_power.end_CPUPower0 = get_CPU_Power("/sys/class/powercap/intel-rapl:0/energy_uj");
    cpu_power.end_CPUPower1 = get_CPU_Power("/sys/class/powercap/intel-rapl:1/energy_uj");
    
    //쿼리 수행 시간 동안의 cpu power 계산
    double cpu_power_result = (cpu_power.end_CPUPower0 - cpu_power.start_CPUPower0) + (cpu_power.end_CPUPower1 - cpu_power.start_CPUPower1);
    result.power_usage = cpu_power_result;

    //쿼리 끝 시점 net 값 저장
    Net_Usage temp_net;
    get_Network_Usage(temp_net);
    net_usg.end_rx = temp_net.rx;
    net_usg.end_tx = temp_net.tx;

    //쿼리 수행 시간 동안의 net 계산
    unsigned long long rxByte_usage, txByte_usage, totalByte;
    rxByte_usage = net_usg.end_rx - net_usg.start_rx;
    txByte_usage = net_usg.end_tx - net_usg.start_tx;
    totalByte = rxByte_usage + txByte_usage;
    // totalByte = totalByte/1000000.0;
    result.network_usage = totalByte;

    //std::cout << "---------query end----------" << std::endl;
    //std::cout << "cpu 사용량 : " << cpu_usg.end_cpu << std::endl;
    //std::cout << "cpu power1 : " << cpu_power.end_CPUPower0 << std::endl;
    //std::cout << "cpu power2 : " << cpu_power.end_CPUPower1 << std::endl;
    //std::cout << "Net 사용량 rx : " << net_usg.end_rx << std::endl;
    //std::cout << "Net 사용량 tx : " << net_usg.end_tx << std::endl;
    // std::cout << "쿼리 수행동안 CPU 사용량 : " << result.cpu_usage << std::endl;
    // std::cout << "쿼리 수행동안 Power 사용량 : " << result.power_usage << std::endl;
    // std::cout << "쿼리 수행동안 Net 사용량 : " << result.network_usage << std::endl;
}

