#include <sys/socket.h> //for socket(),bind(),listen(),accpet() system call
#include <netinet/in.h> //for sockaddr_in
#include <unistd.h>     //for close() system call
#include <cstring>

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));

    listen(server_fd, 1);

    int addrlen = sizeof(address);
    int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

    const char* message = "Hello";
    send(client_socket, message, strlen(message), 0);

    close(client_socket);
    close(server_fd);

    return 0;
}


