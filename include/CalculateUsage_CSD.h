#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <unistd.h>

struct CpuUsage {
unsigned long long utime;
unsigned long long stime;
};

struct NetworkInterface{
  std::string name;
  unsigned long long rxBytes;
  unsigned long long txBytes;
};

struct CPUStats {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
    unsigned long long guest;
    unsigned long long guestNice;
};

unsigned long long rxBytesBeforeEno;
unsigned long long txBytesBeforeEno;
unsigned long long rxBytesBeforeLo;
unsigned long long txBytesBeforeLo;

unsigned long long rxBytesAfterEno;
unsigned long long txBytesAfterEno;
unsigned long long rxBytesAfterLo;
unsigned long long txBytesAfterLo;

CpuUsage previousCpuUsage;
CpuUsage afterCpuUsage;

unsigned long long totalCPUTimeBefore = 0;
unsigned long long totalCPUTimeAfter = 0;

auto start = std::chrono::high_resolution_clock::now();


CPUStats getCPUStats() {
    CPUStats stats;

    std::ifstream file("/proc/stat");
    if (!file) {
        std::cerr << "Failed to open /proc/stat" << std::endl;
        return stats;
    }

    std::string line;
    std::getline(file, line);
    std::istringstream iss(line);
    std::string cpuLabel;
    iss >> cpuLabel >> stats.user >> stats.nice >> stats.system >> stats.idle >> stats.iowait >> stats.irq >> stats.softirq >> stats.steal >> stats.guest >> stats.guestNice;

    return stats;
}

unsigned long long getTotalCPUTime(const CPUStats& stats) {
    return stats.user + stats.nice + stats.system + stats.idle + stats.iowait + stats.irq + stats.softirq + stats.steal + stats.guest + stats.guestNice;
}


std::vector<NetworkInterface> getNetworkInterfaces() {
    std::vector<NetworkInterface> interfaces;

    std::ifstream file("/proc/net/dev");
    if (!file) {
        std::cerr << "Failed to open /proc/net/dev" << std::endl;
        return interfaces;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find(":") != std::string::npos) {
            std::istringstream iss(line);
            std::string name;
            unsigned long long rxBytes, txBytes;
            char colon;
            iss >> name >> colon >> rxBytes >> txBytes;

            NetworkInterface interface;
            interface.name = name.substr(0, name.size() - 1);
            interface.rxBytes = rxBytes;
            interface.txBytes = txBytes;

            interfaces.push_back(interface);
        }
    }

    return interfaces;
}



unsigned long long getInterfaceRxBytes(const std::vector<NetworkInterface>& interfaces, const std::string& interfaceName) {
    for (const auto& interface : interfaces) {
        if (interface.name == interfaceName) {
            return interface.rxBytes;
        }
    }
    return 0;
}

unsigned long long getInterfaceTxBytes(const std::vector<NetworkInterface>& interfaces, const std::string& interfaceName) {
    for (const auto& interface : interfaces) {
        if (interface.name == interfaceName) {
            return interface.txBytes;
        }
    }
    return 0;
}

std::string executeCommand(const std::string& command) {
  std::string result;
  std::cout<<command<<"\n";
  FILE* pipe = popen(command.c_str(),"r");
    if (pipe) {
        char buffer[128];
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != nullptr)
                result += buffer;
        }
        pclose(pipe);
    }
    return result;
}

CpuUsage GetProcessCpuUsage(int pid) { // get cpu usage by tick
CpuUsage cpuUsage{0, 0};
std::ifstream file("/proc/" + std::to_string(pid) + "/stat");
std::string line;

if (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string token;

    // Extract the required fields from the stat file
    for (int i = 1; i <= 13; ++i)
        iss >> token;
    iss >> cpuUsage.utime;
    for (int i = 15; i <= 16; ++i)
        iss >> token;
    iss >> cpuUsage.stime;
}
return cpuUsage;
}

float CalculateCpuUsagePercentage(const CpuUsage& previousUsage, const CpuUsage& currentUsage, int intervalMilliseconds) {
unsigned long long totalTicksDiff = currentUsage.utime + currentUsage.stime - previousUsage.utime - previousUsage.stime;
std::cout<<"Total Ticks Difference : "<<totalTicksDiff<<"\n";
float cpuUsagePercentage = 100.0f * (totalTicksDiff / static_cast<float>(intervalMilliseconds));
return cpuUsagePercentage;
}


void CalculateStart(){
    start = std::chrono::high_resolution_clock::now();
    CPUStats statsBefore = getCPUStats();
    totalCPUTimeBefore = getTotalCPUTime(statsBefore);

    std::vector<NetworkInterface> interfacesBefore = getNetworkInterfaces();
    rxBytesBeforeEno = getInterfaceRxBytes(interfacesBefore, "ngdtap0");
    txBytesBeforeEno = getInterfaceTxBytes(interfacesBefore, "ngdtap0");
    rxBytesBeforeLo = getInterfaceRxBytes(interfacesBefore, "lo");
    txBytesBeforeLo = getInterfaceTxBytes(interfacesBefore, "lo");
    pid_t pid = getpid();
    previousCpuUsage = GetProcessCpuUsage(pid);

}

void CalculateEnd(){
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout<<"Query execute time : "<<duration.count()<<"s\n";

    CPUStats statsAfter = getCPUStats();
    totalCPUTimeAfter = getTotalCPUTime(statsAfter);
    unsigned long long cpuTimeUsed = totalCPUTimeAfter - totalCPUTimeBefore;
    double cpuUsage = static_cast<double>(cpuTimeUsed) / ((duration.count()) * sysconf(_SC_CLK_TCK)) * 100.0;
    std::cout<<"CPU Usage of Host "<<": " <<cpuUsage <<" %\n";

    pid_t pid = getpid();
    afterCpuUsage = GetProcessCpuUsage(pid);
    float cpuUsagePercentage = CalculateCpuUsagePercentage(previousCpuUsage, afterCpuUsage, duration.count());
    unsigned long long totalTicksDiff = afterCpuUsage.utime + afterCpuUsage.stime - previousCpuUsage.utime - previousCpuUsage.stime;
    std::cout << "CPU Usage of Process " << pid << ": " << cpuUsagePercentage << "%" << std::endl;
    std::cout << "CPU Use Tick " <<pid<<": "<<totalTicksDiff <<" ns"<<std::endl;

    std::vector<NetworkInterface> interfacesAfter = getNetworkInterfaces();
    rxBytesAfterEno = getInterfaceRxBytes(interfacesAfter, "ngdtap0"); 
    rxBytesAfterLo = getInterfaceRxBytes(interfacesAfter, "lo");
    txBytesAfterEno = getInterfaceTxBytes(interfacesAfter, "ngdtap0");
    txBytesAfterLo = getInterfaceTxBytes(interfacesAfter, "lo");

    unsigned long long rxBytesUsedEno = rxBytesAfterEno - rxBytesBeforeEno;
    unsigned long long rxBytesUsedLo = rxBytesAfterLo - rxBytesBeforeLo;
    unsigned long long txBytesUsedEno = txBytesAfterEno - txBytesBeforeEno;
    unsigned long long txBytesUsedLo = txBytesAfterLo - txBytesBeforeLo;

    std::cout << "Network Usage during process execution:" << std::endl;
    std::cout << "RX bytes used ngdtap0: " << rxBytesUsedEno << std::endl;
    std::cout << "RX bytes used Lo : " << rxBytesUsedLo << std::endl;
    std::cout << "TX bytes used ngdtap0: " << txBytesUsedEno << std::endl;
    std::cout << "TX bytes used Lo : " << txBytesUsedLo << std::endl;

    unsigned long long totalBytesUsed = rxBytesUsedEno+rxBytesUsedLo+txBytesUsedEno+txBytesUsedLo;
    
    double powerUsage = 12 * cpuUsagePercentage * duration.count();

    std::cout<<"\nstart TPCH Query with CSD..."<<std::endl;
    std::cout<<"Query Done... Time -> "<<duration.count()/1000.0<<" s"<<std::endl;
    std::cout<<"CPU Usage During Query Execution     -> "<<cpuUsagePercentage<<" %"<<std::endl;
    std::cout<<"Energy Usage during query execution  -> "<<powerUsage<<" W"<<std::endl;
    std::cout<<"Network Usage during query execution -> "<<totalBytesUsed/1000000.0<<" MB"<<std::endl;

    return;
}