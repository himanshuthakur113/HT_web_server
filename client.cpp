#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>  //for inet_pton
#include <iostream>
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
    
    size_t bytes_received = read(c_socket, buffer, sizeof(buffer) - 1);
    
    if(bytes_received > 0)
    std::cout << buffer <<"\n";

    close(c_socket);
    return 0;
    
}