#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

struct CPU_Power {
    double start_CPUPower0;
    double start_CPUPower1;
    double end_CPUPower0;
    double end_CPUPower1;
};

// CPU usage 구하는 함수
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

int main(){
    //쿼리 시작 
    //interface container에서 통신으로 쿼리 시작 받음
    auto start = std::chrono::high_resolution_clock::now();


    //Metric 값 측정
    //CPU, Power, Network


    //쿼리 끝
    //interface container에서 통신으로 쿼리 시작 받음
    auto end = std::chrono::high_resolution_clock::now();

}