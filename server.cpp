#include <sys/socket.h> //for socket(),bind(),listen(),accpet() system call
#include <netinet/in.h> //for sockaddr_in
#include <unistd.h>     //for close() system call
#include <cstring>
#include <iostream>
#include <thread>

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

        ScopedSocket& operator=(ScopedSocket&& other) noexcept
        {
            if(this != &other)
            {
                if(fd >= 0) close(fd);
                fd = other.fd;
                other.fd =  -1;
            }
            return *this;
        }

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
                
                std::thread([client_fd = raw_client_socket]()
                {   
                    ScopedSocket client_socket(client_fd);
                    std::cout << "Thread " << std::this_thread::get_id << "\n";

                    const char* message = "Welcome to HT server\n";
                    send(client_socket.get(), message, strlen(message), 0);

                    char buffer[1024];

                    while(true)
                    {
                        memset(buffer, 0, sizeof(buffer));

                        ssize_t bytes_recieved = recv(client_socket.get(), buffer, sizeof(buffer) - 1,0);

                        if(bytes_recieved <= 0)
                        {
                            std::cout << "Thread " << std::this_thread::get_id() << ": Client Disconnected or Error\n";
                        }

                        std::cout << "Thread " << std::this_thread::get_id() << ": Received: " << buffer;

                        send(client_socket.get(), buffer, bytes_recieved, 0);
                    }

                }).detach();

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


