#ifndef HT_SERVER_HPP
#define HT_SERVER_HPP

#include <string>

class HT_Server
{
    private:
        int server_fd;
        int port;

    public:
        explicit HT_Server(int p);
        ~HT_Server();
    
        void run();
        bool sendAll(int socket, const std::string& response);  //send() does not gurantee that all the bytes will not be sent in one call

        HT_Server(const HT_Server&) = delete;
        HT_Server& operator=(const HT_Server&) = delete;
};

#endif