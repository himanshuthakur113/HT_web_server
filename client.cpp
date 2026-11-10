#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>  //for inet_pton
#include <iostream>
int main()
{
    int c_socket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serv_addr{};

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    connect(c_socket,(struct sockaddr*)&serv_addr, sizeof(serv_addr));

    char buffer[1024] = {0};
    
    size_t data = read(c_socket, buffer, sizeof(buffer) - 1);
    
    std::cout << buffer <<"\n";

    close(c_socket);
    return 0;
    
}