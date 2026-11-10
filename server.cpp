#include <sys/socket.h> //for socket(),bind(),listen(),accpet() system call
#include <netinet/in.h> //for sockaddr_in
#include <unistd.h>     //for close() system call
#include <cstring>
#include <iostream>

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0)
    {
        perror("Socket creation failed");    //failed call write gloabal var "errno" and perror automatically report it 
        return 1;
    }

    //if server stop and restart immediately the os say port already in use for solving that using setsockopt()
    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("socket option set failed");
        close(server_fd);
        return 1;
    } 

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    if(listen(server_fd, 1) < 0)
    {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    std::cout << "Server listening on port 8080...\n";

    int addrlen = sizeof(address);
    int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if(client_socket < 0)
    {
        perror("Accept failed");
        close(server_fd);
        return 1;
    }

    std::cout << "Client connected!\n";

    const char* message = "Hello";
    send(client_socket, message, strlen(message), 0);

    close(client_socket);
    close(server_fd);

    return 0;
}


