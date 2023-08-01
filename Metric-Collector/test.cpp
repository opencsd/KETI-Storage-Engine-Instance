#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <sstream>


int main() {
    std::ifstream file("/proc/net/dev");
    if (!file) {
        std::cerr << "Failed to open /proc/net/dev" << std::endl;
    }

    const std::string interface = "eno1";

    std::string line;
    
    std::string name;
    unsigned long long rxByte, txByte;


    while (getline(file, line)){
        if(line.find("eno1") != std::string::npos){
            std::istringstream iss(line);
            iss >> name >> rxByte;
            for (int i = 3; i < 11; i++){
                iss >> txByte;
            }
            std::cout << line << std::endl;
            break;
        }

       
    }

    std::cout << name << std::endl;
    std::cout << rxByte << std::endl;
    std::cout << txByte << std::endl;
    
    return 0;
}
