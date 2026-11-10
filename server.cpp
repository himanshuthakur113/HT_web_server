#include <sys/socket.h> //for socket(),bind(),listen(),accpet() system call
#include <netinet/in.h> //for sockaddr_in
#include <unistd.h>     //for close() system call
#include <cstring>
#include <iostream>

class HT_Socket
{   
    private:
        int fd;

    public:

        explicit HT_Socket(int socket_fd)
        {
            fd = socket_fd;
        }

        ~HT_Socket()
        {
            if(fd >= 0)
            {
                close(fd);
                std::cout << "Socket " << fd << " closed automatically\n";
            }
        }

        int get() const
        {
            return fd;
        }

        //delete the copy constructor tell the compiler to not generate one if one try to create it give error
        HT_Socket(const HT_Socket&) = delete; 

        //similarly delete the copy assignment
        HT_Socket& operator=(const HT_Socket&) = delete;

};

int main()
{
    int raw_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(raw_server_fd < 0)
    {
        perror("Socket creation failed");    //failed call write gloabal var "errno" and perror automatically report it 
        return 1;
    }

    HT_Socket server_fd(raw_server_fd);

    //if server stop and restart immediately the os say port already in use for solving that using setsockopt()
    int opt = 1;
    if(setsockopt(server_fd.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("socket option set failed");
        //close(server_fd);
        return 1;
    } 

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if(bind(server_fd.get(), (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        //close(server_fd);
        return 1;
    }

    if(listen(server_fd.get(), 1) < 0)
    {
        perror("Listen failed");
        //close(server_fd);
        return 1;
    }

    std::cout << "Server listening on port 8080...\n";

    int addrlen = sizeof(address);
    int raw_client_socket = accept(server_fd.get(), (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if(raw_client_socket < 0)
    {
        perror("Accept failed");
        //close(server_fd);
        return 1;
    }

    HT_Socket client_socket(raw_client_socket);
    

    std::cout << "Client connected!\n";

    const char* message = "Hello";
    send(client_socket.get(), message, strlen(message), 0);

    //close(client_socket);
    //close(server_fd);

    return 0;
}


