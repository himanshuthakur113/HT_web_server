#ifndef HT_SERVER_HPP
#define HT_SERVER_HPP

class HT_Server
{
    private:
        int server_fd;
        int port;

    public:
        explicit HT_Server(int p);
        ~HT_Server();
    
        void run();

        HT_Server(const HT_Server&) = delete;
        HT_Server& operator=(const HT_Server&) = delete;
};

#endif