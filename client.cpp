#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>  //for inet_pton
#include <iostream>
#include <string>
int main()
{
    int c_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(c_socket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    sockaddr_in serv_addr{};

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) < 0)
    {
        perror("Invalid address/ Address not supported\n");
        close(c_socket);
        return 1;
    }

    if(connect(c_socket,(struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Server Not Connected");
        close(c_socket);
        return 1;
    }
    
    std::cout << "Server connected successfully\n";

    char buffer[1024] = {0};
    ssize_t bytes_received = read(c_socket, buffer, sizeof(buffer) - 1);
    if(bytes_received > 0)
    {
        std::cout << buffer;
    }

    std::cout << "enter message: ";
    std::string line;
    std::getline(std::cin,line);
    
    line += "\n";
    
    send(c_socket, line.c_str(), line.length(), 0);
    
    char buffer_2[1024] = {0};
    ssize_t bytes_received_2 = read(c_socket, buffer_2, sizeof(buffer_2) - 1);
    
    if(bytes_received_2 > 0)
    {
        std::cout << "Echo from server: " << buffer_2;
    }
    close(c_socket);
    return 0;
    
}