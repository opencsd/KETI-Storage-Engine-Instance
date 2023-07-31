#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>


// CPU usage 구하는 함수
double get_cpu_usage() {
    std::ifstream proc_stat("/proc/stat");
    std::string line;
    std::getline(proc_stat, line); // 첫 번째 라인은 'cpu' 정보
    std::istringstream iss(line);
    std::string cpuLabel;
    iss >> cpuLabel; // 'cpu' 레이블 버리기

    // 'cpu' 이후의 값들을 읽어와서 사용량 계산
    long user, nice, sys, idle, iowait, irq, softirq, steal, guest, guest_nice;
    iss >> user >> nice >> sys >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;

    long idle_time = idle + iowait;
    long active_time = user + nice + sys + irq + softirq + steal;
    long total_time = active_time + idle_time;

    return static_cast<double>(active_time) / total_time * 100.0;
}
int main(){
    //쿼리 시작


    //Metric 값 측정
    //CPU, Power, Network


    //쿼리 끝
}