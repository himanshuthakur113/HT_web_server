#include "HT_server.hpp"
#include "ScopedSocket.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "StaticFileHandler.hpp"

#include <sys/socket.h> //for socket(),bind(),listen(),accpet() system call
#include <netinet/in.h> //for sockaddr_in
#include <unistd.h>     //for close() system call
#include <cstring>
#include <iostream>
#include <thread>
#include <cerrno>

HT_Server::HT_Server(const ServerConfig& config) : config_(config)
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
    address.sin_port = htons(config_.port);

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

    std::cout << "Server successfully started and listening on port " <<config_.port << "...\n";
}

HT_Server::~HT_Server()
{
    stop();

    if(server_fd >= 0)
    {
        close(server_fd);
        server_fd = -1;
    }

    std::cout << "Server closed successfully\n";
}

void HT_Server::run()
{   
    for(size_t i = 0; i < config_.worker_count; ++i)
    {
        workers.emplace_back(&HT_Server::workerLoop, this);
    }

    while(!stopping.load())
    {
        sockaddr_in client_address{};
        socklen_t addrlen = sizeof(client_address);

        int client_fd =  accept(server_fd, (struct sockaddr*)&client_address, &addrlen);
        if(client_fd < 0)
        {   
            if(stopping.load())
                break;
            
            if(errno == EINTR)
                continue;
            

            perror("Accept failed");
            break;
        }
        //so after the accept call when client(browser) connected to the server
        // it will send a block of raw text formatted according to HTTP/1.1 

        bool queued = false;
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            
            if(!stopping.load() && clients.size() < config_.max_queue_size)
            {   
                clients.push(client_fd);
                queued = true;
            }
        }
        if(queued)
        {
            cv.notify_one();
        }
        else
        {
            close(client_fd);
        }
        
    }

    stopping.store(true);
    cv.notify_all();

    for(std::thread& worker : workers)
    {
        if(worker.joinable())
            worker.join();
    }

    workers.clear();
}       


bool HT_Server::sendAll(int socket, const std::string& response)
{
    size_t total = 0;

    while(total < response.length())
    {
        ssize_t n = send(socket, response.c_str() + total, response.length() - total, MSG_NOSIGNAL);

        if(n <= 0)
        {
            return false;
        }

        total += static_cast<size_t>(n);
    }
    return true;
}

void HT_Server::handleClient(int client_fd)
{
    ScopedSocket client_socket(client_fd);
    std::cout << "Thread " << std::this_thread::get_id() << "\n";

    constexpr size_t MAX_HEADER_SIZE = 64 * 1024;
    constexpr size_t MAX_BODY_SIZE = 1024 * 1024;
    //constexpr size_t MAX_REQUEST_SIZE = MAX_HEADER_SIZE + MAX_BODY_SIZE;

    auto sendError = [&](int statusCode, const std::string& statusMessage, const std::string& message)
    {
        HTTPResponse response;

        response.setStatusCode(statusCode, statusMessage);
        response.setHeader("Content-Type", "text/plain");
        response.setHeader("Connection", "close");
        response.setBody(message);

        std::string rawResponse = response.toString();

        if(!sendAll(client_socket.get(), rawResponse))
        {
            perror("Failed to send error response");
        }
    };

    std::string data;
    char buffer[4096];
    size_t header_end;

    while((header_end = data.find("\r\n\r\n")) == std::string::npos)
    {   
        if(data.size() >= MAX_HEADER_SIZE)
        {
            sendError(431,"Request Header Fields Too Large", "Request headers excced the allowed size");
                return;
        }

        //find return npos until \r\n\r\n arrived,so loop keep reading until  the headers are complete
        ssize_t n = recv(client_socket.get(), buffer, sizeof(buffer) - 1, 0);
        if(n <= 0)
        {
            if(!data.empty())
            {
                sendError(400, "Bad Request", "Incomplete HTTP headers");
            }
            return;
        }
        data.append(buffer, static_cast<size_t> (n));
    }

    if(header_end + 4 > MAX_HEADER_SIZE)
        {
            sendError(431, "Request Header Fields Too Large", "Request headers exceeds the allowed size");
            return;
        }

            std::string headers = data.substr(0, header_end + 4);

            HTTPRequest request;

            bool success = request.parseHeaders(headers);


            if(!success)
            {
                sendError(400, "Bad Request", "Malformed HTTP header");
                return;
            }

            size_t body_length = 0;
                        
            std::string content_length = request.getHeader("content-length");
                
            if(!content_length.empty())
            {
                for(char c: content_length)
                {
                    if(c < '0' || c > '9')
                    {
                        sendError(400, "Bad Request", "Invalid Content-Length");
                        return;
                    }

                    size_t digit = static_cast<size_t>(c- '0');

                    if(body_length > (MAX_BODY_SIZE - digit)/10)
                    {
                        sendError(413, "Content Too Large", "Request body exceeds 1 MB");
                        return;
                    }

                    body_length = body_length * 10 + digit;
                }
            }

            size_t expected_size = header_end + 4 + body_length;

            while(data.size() < expected_size)
            {   
                size_t remaining = expected_size - data.size();

                size_t bytes_to_read;
                if(sizeof(buffer) <= remaining)
                {
                    bytes_to_read = sizeof(buffer);
                }
                else
                {
                    bytes_to_read = remaining;
                }

                ssize_t n = recv(client_socket.get(), buffer, bytes_to_read, 0);

                if(n <= 0)
                {   
                    sendError(400, "Bad Request", "Incomplete HTTP request body");
                    return;
                }

                data.append(buffer,static_cast<size_t>(n));
            }

            success = request.parse(data);

            if(!success)
            {
                sendError(400, "Bad Request", "Malformed HTTP Request");
                return;
            }

            std::cout << "\nHTTP Request Received\n" << data <<"\n";

            HTTPResponse response;

            std::cout << "[Parser] Method: " << request.getMethod()
                        << "  | URI: " << request.getUri()
                       << "  | Host: " << request.getHeader("host") << "\n";

            if(request.getMethod() != "GET" && request.getMethod() != "HEAD")
            {
                response.setStatusCode(405, "Method Not Allowed");
                response.setHeader("Allow", "GET, HEAD");
                response.setHeader("Content-Type", "text/plain");
                response.setHeader("Connection", "close");
                response.setBody("405 Method Not Allowed");
            }
            else
            {
                StaticFileHandler fileHandler("public");

                std::string body;
                std::string contentType;

                if(fileHandler.serve(request.getUri(), body, contentType))
                {
                    response.setStatusCode(200, "OK");
                    response.setHeader("Content-Type", contentType);
                    response.setHeader("Connection", "close");
                    response.setBody(body);
                }
                else
                {  
                    std::string errorBody = "<h1>404 Not Found</h1>";
                    response.setStatusCode(404, "Not Found");
                    response.setHeader("Content-Type", "text/html");
                    response.setHeader("Connection", "close");
                    response.setBody(errorBody);
                }      
            }              

            std::string http_response = response.toString(request.getMethod() != "HEAD" && success);

            if(!sendAll(client_socket.get(), http_response))
            {
                perror("Failed to send HTTP response\n");
            }
            
            std::cout << "[Thread " <<std::this_thread::get_id() << "] Http response send.Closing connection\n";
}

void HT_Server::workerLoop()
{
    while(true)
    {
        int client_fd;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            cv.wait(lock,[this]()
            {
                return !clients.empty() || stopping.load();
            });

            if(clients.empty() && stopping.load())
            {
                return;
            }

            client_fd = clients.front();
            clients.pop();
        }
        
        handleClient(client_fd);
    }
}

void HT_Server::stop()
{
    stopping.store(true);
    shutdown(server_fd, SHUT_RDWR);
    cv.notify_all();
}