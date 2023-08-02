#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8080

void a(){
    std::cout << "Query Start!!!" << std::endl;
}
void b(){
    std::cout << "Query End!!!" << std::endl;
}

int main() {
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
        // if(valread > 0) {
        //     buffer[valread] = '\0';
        //     /* 쿼리 시작 */
        //     if(strcmp(buffer, "Query Start") == 0){
        //         std::cout << "---------Query Start-----------" << std::endl;
        //         // QueryStart(querydu, cpu_usg, cpu_power, net_usg);
        //         a();
        //     }
        //     /* 쿼리 끝 */
        //     std::cout << "---------Query End-----------" << std::endl;
        //     // QueryEnd(querydu, cpu_usg, cpu_power, net_usg, result);
        //     b();
        //     break;
        // }
        buffer[valread] = '\0';
        /* 쿼리 시작 */
        if(strcmp(buffer, "Query Start") == 0){
            std::cout << "---------Query Start-----------" << std::endl;
            // QueryStart(querydu, cpu_usg, cpu_power, net_usg);
            a();
        }
        /* 쿼리 끝 */
        else{
            std::cout << "---------Query End-----------" << std::endl;
            // QueryEnd(querydu, cpu_usg, cpu_power, net_usg, result);
            b();
            break;
        }
        close(new_socket);
    }
    close(server_fd);

    return 0;
}
