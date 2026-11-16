#include "HT_server.hpp"
#include "ScopedSocket.hpp"
#include "HTTPRequest.hpp"
#include <sys/socket.h> //for socket(),bind(),listen(),accpet() system call
#include <netinet/in.h> //for sockaddr_in
#include <unistd.h>     //for close() system call
#include <cstring>
#include <iostream>
#include <thread>

HT_Server::HT_Server(int p) : server_fd(-1), port(p)
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

HT_Server::~HT_Server()
{
    if(server_fd >= 0)
    {
        close(server_fd);
        std::cout << "Server closed successfully\n";
    }
}

void HT_Server::run()
{
    while(true)
    {
        sockaddr_in client_address{};
        socklen_t addrlen = sizeof(client_address);

        int raw_client_socket =  accept(server_fd, (struct sockaddr*)&client_address, &addrlen);
        if(raw_client_socket < 0)
            {
                perror("Accept failed");
                continue;
            }
        //so after the accept call when client(browser) connected to the server
        // it will send a block of raw text formatted according to HTTP/1.1 
                
        std::thread([client_fd = raw_client_socket]()
        {   
            ScopedSocket client_socket(client_fd);
            std::cout << "Thread " << std::this_thread::get_id() << "\n";

            std::string data;
            char buffer[4096];
            size_t header_end;

            while((header_end = data.find("\r\n\r\n")) == std::string::npos)
            {   
                if(data.size() > 1024 * 1024) return;

                //find return npos until \r\n\r\n arrived,so loop keep reading until  the headers are complete
                ssize_t n = recv(client_socket.get(), buffer, sizeof(buffer) - 1, 0);
                if(n <= 0)
                {
                    return;
                }
                data.append(buffer, n);
            }

            std::string headers = data.substr(0, header_end);
            size_t body_length = 0;
            size_t pos = headers.find("Content-Length:"); //pos will hold index of start index at C in Content-Length
            
            if(pos != std::string::npos)
            {
                try { body_length = std::stoul(headers.substr(pos + 15)); } //15 = lenght of Content length
                catch(...) {return; }
                //stoul will skip leading spaces and stops at \r header end
            }

            while(data.size() < header_end + 4 + body_length)
            {   
                if(data.size() > 1024 * 1024) return;

                ssize_t n = recv(client_socket.get(), buffer, sizeof(buffer) - 1, 0);
                if(n <= 0)
                {
                    return;
                }
                data.append(buffer,n);
            }

           
            std::cout << "\nHTTP Request Received\n" << data <<"\n";

            //the text received from the client(browser) is stored inside this buffer

            HTTPRequest request;
            bool success = request.parse(data);

            std::string http_response;

            if(success)
            {
                std::cout << "[Parser] Method: " << request.getMethod()
                          << "  | URI: " << request.getUri()
                          << "  | Host: " << request.getHeader("Host") << "\n";

                std::string html_body = 
                "<!DOCTYPE html>\n"
                "<html>\n"
                "<head>\n"
                "    <title>C++ Custom Web Server</title>\n"
                "</head>\n"
                "<body>\n"
                "        <h1>Hello from your HT Web Server</h1>\n"
                "</body>\n"
                "</html>\n";

                http_response = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=UTF-8\r\n"
                "Content-Length: " + std::to_string(html_body.length()) + "\r\n"
                "Connection: close\r\n"
                "\r\n" + 
                html_body;

            }
            else
            {
                std::string error_body = 
                "<!DOCTYPE html>\n"
                "<html>\n"
                "<head>\n"
                "    <title>C++ Custom Web Server</title>\n"
                "</head>\n"
                "<body>\n"
                "        <h1><h1>400 Bad Request</h1><p>Your browser sent a malformed request.</p></h1>\n"
                "</body>\n"
                "</html>\n"; 

                http_response = 
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Type: text/html; charset=UTF-8\r\n"
                "Content-Length: " + std::to_string(error_body.length()) + "\r\n"
                "Connection: close\r\n"
                "\r\n" + 
                error_body;

            }

            send(client_socket.get(), http_response.c_str(), http_response.length(), 0);
            std::cout << "[Thread " <<std::this_thread::get_id() << "] Http response send.Closing connection\n";

        }).detach();
    }
}       





