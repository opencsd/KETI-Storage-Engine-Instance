#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <sstream>

struct Net_Usage {
    unsigned long long rx;
    unsigned long long tx;
};

struct Net_Usage get_Network_Usage() {
    unsigned long long rxByte, txByte;
    Net_Usage net_usg;
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
    std::cout << rxByte << std::endl;
    std::cout << txByte << std::endl;

    net_usg.rx = rxByte;
    net_usg.tx = txByte;

    return net_usg;
}


int main() {
    Net_Usage net_usg;
    
    net_usg = get_Network_Usage();

    std::cout << net_usg.rx << std::endl;
    std::cout << net_usg.tx << std::endl;

    return 0;
}
