#include <sys/socket.h> //for socket(),bind(),listen(),accpet() system call
#include <netinet/in.h> //for sockaddr_in
#include <unistd.h>     //for close() system call
#include <cstring>
#include <iostream>

class ScopedSocket
{
    private:
        int fd;
    
    public:

        explicit ScopedSocket(int socket_fd) : fd(socket_fd) {}

        ~ScopedSocket()
        {
            if(fd >= 0)
            {
                close(fd);
                std::cout << "Client socket closed automatically\n";
            }
        }

        int get() const
        {
            return fd;
        }

        ScopedSocket(const ScopedSocket&) = delete;
        ScopedSocket& operator=(const ScopedSocket&) = delete;

};

class HT_Server
{   
    private:
        int server_fd;
        int port;

    public:

        explicit HT_Server(int p) : server_fd(-1), port(p)
        {

            server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if(server_fd < 0)
            {
                perror("Socket Creation Failed");
                exit(1);
            }

            int opt = 1;
            if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
            {
                perror("socket option set failed");
                close(server_fd);
                exit(1);
            } 

            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(port);

            if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
            {
                perror("Bind failed");
                close(server_fd);
                exit(1);
            }

            if(listen(server_fd, 5) < 0)
            {
                perror("Listen failed");
                close(server_fd);
                exit(1);
            }

            std::cout << "Server successfully started and listening on port " <<port << "...\n";
        }

        ~HT_Server()
        {
            if(server_fd >= 0)
            {
                close(server_fd);
                std::cout << "Server closed successfully\n";
            }
        }

        void run()
        {
            while(true)
            {
                sockaddr_in client_address{};
                socklen_t addrlen = sizeof(client_address);

                int raw_client_socket =  accept(server_fd, (struct sockaddr*)&client_address, &addrlen);
                if(raw_client_socket < 0)
                {
                    perror("Accept failed");
                    return;
                }

                ScopedSocket client_socket(raw_client_socket);
                std::cout << "Client Connected\n";

                const char* message = "Hello Komal Sharma";
                send(client_socket.get(), message, strlen(message), 0);

            }
        } 
        
        HT_Server(const HT_Server&) = delete;
        HT_Server& operator=(const HT_Server&) = delete;


};

int main()
{
    HT_Server server(8080);

    server.run();

    return 0;
}


