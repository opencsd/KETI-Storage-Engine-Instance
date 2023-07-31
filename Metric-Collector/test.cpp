#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

// 함수: /proc/stat 파일에서 CPU 사용량을 읽어와서 계산
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

int main() {
    while (true) {
        double cpu_usage = get_cpu_usage();
        std::cout << "CPU Usage: " << cpu_usage << "%" << std::endl;

        // 1분 대기
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    return 0;
}
