#ifndef HT_SERVER_HPP
#define HT_SERVER_HPP

#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

struct ServerConfig
{
    int port = 8080;
    size_t worker_count = 4;
    size_t max_queue_size = 128;
    bool verbose = false;
};

class HT_Server
{
    private:
        ServerConfig config_;
        int server_fd;
        int port;

        std::vector<std::thread> workers;
        std::queue<int> clients;

        std::mutex queue_mutex;
        std::condition_variable cv;
        std::atomic<bool> stopping{false};

        void workerLoop();
        void handleClient(int client_fd);
        void rejectConnection(int fd);

    public:
        explicit HT_Server(const ServerConfig& config);
        ~HT_Server();
    
        void run();
        void stop();

        bool sendAll(int socket, const std::string& response);  //send() does not gurantee that all the bytes will not be sent in one call

        HT_Server(const HT_Server&) = delete;
        HT_Server& operator=(const HT_Server&) = delete;
};

#endif