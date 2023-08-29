#include "metric_check.h"

//테스트용 메인문
// int main(){
//     CPU_Power cpu_power;
//     QueryDuration querydu;
//     CPU_Usage cpu_usg;
//     result_Net_Usage net_usg;
//     Metric_Result result;

//     QueryStart(querydu, cpu_usg, cpu_power, net_usg);

//     std::cout << "-------main start-------" << std::endl;
//     std::cout << "cpu 사용량 : " << cpu_usg.start_cpu << std::endl;
//     std::cout << "cpu power1 : " << cpu_power.start_CPUPower0 << std::endl;
//     std::cout << "cpu power2 : " << cpu_power.start_CPUPower1 << std::endl;
//     std::cout << "Net 사용량 rx : " << net_usg.start_rx << std::endl;
//     std::cout << "Net 사용량 tx : " << net_usg.start_tx << std::endl;

//     sleep(5);

//     QueryEnd(querydu, cpu_usg, cpu_power, net_usg, result);

//     std::cout << "-------main end-------" << std::endl;
//     std::cout << "cpu 사용량 : " << cpu_usg.end_cpu << std::endl;
//     std::cout << "cpu power1 : " << cpu_power.end_CPUPower0 << std::endl;
//     std::cout << "cpu power2 : " << cpu_power.end_CPUPower1 << std::endl;
//     std::cout << "Net 사용량 rx : " << net_usg.end_rx << std::endl;
//     std::cout << "Net 사용량 tx : " << net_usg.end_tx << std::endl;


//     std::cout << "쿼리 수행동안 CPU 사용량 : " << result.cpu_usage << std::endl;
//     std::cout << "쿼리 수행동안 Power 사용량 : " << result.power_usage << std::endl;
//     std::cout << "쿼리 수행동안 Net 사용량 : " << result.network_usage << std::endl;
    

//     return 0;
// }

int main(){
    CPU_Power cpu_power;
    QueryDuration querydu;
    CPU_Usage cpu_usg;
    result_Net_Usage net_usg;
    Metric_Result result;

    // Interface와 통신으로 쿼리 시작, 종료 받음
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    // Attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "Setsockopt error" << std::endl;
        return -1;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        std::cerr << "Bind failed" << std::endl;
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen error" << std::endl;
        return -1;
    }

    while (true)
    {
        // Accept incoming connections
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            std::cerr << "Accept error" << std::endl;
            return -1;
        }

        // Receive message from the client
        valread = read(new_socket, buffer, 1024);

        // Query Start Message Recieved
        if(strcmp(buffer, "Query Start") == 0){
            std::cout << "---------Query Start main-----------" << std::endl;
            QueryStart(querydu, cpu_usg, cpu_power, net_usg);
        }
        // Query End Message Recieved
        else{
            std::cout << "---------Query End-----------" << std::endl;
            QueryEnd(querydu, cpu_usg, cpu_power, net_usg, result);
            break;
        }
        close(new_socket);
    }
    close(server_fd);


    std::cout << "쿼리 수행동안 CPU 사용량 : " << result.cpu_usage << std::endl;
    std::cout << "쿼리 수행동안 Power 사용량 : " << result.power_usage << std::endl;
    std::cout << "쿼리 수행동안 Net 사용량 : " << result.network_usage << std::endl;

    return 0;
}