#include "HT_server.hpp"

#include <iostream>
#include <string>
#include <cstdlib>
#include <stdexcept>

int main(int argc, char* argv[]) {
    ServerConfig config;

    try 
    {
        for (int i = 1; i < argc; ++i) 
        {
            std::string arg = argv[i];

            if (i + 1 >= argc)
            {
                throw std::invalid_argument("Missing value for " + arg);
            }

            std::string value = argv[++i];
            int number = std::stoi(value);

            if (arg == "--workers") 
            {
                if (number <= 0)
                {
                    throw std::invalid_argument("Workers must be greater than zero");
                }
                config.worker_count = static_cast<size_t>(number);
            }
            else if (arg == "--queue") 
            {
                if (number <= 0)
                {
                    throw std::invalid_argument("Queue size must be greater than zero");
                }
                config.max_queue_size = static_cast<size_t>(number);
            }
            else if (arg == "--port") 
            {
                if (number < 1 || number > 65535)
                {
                    throw std::invalid_argument("Port must be between 1 and 65535");
                }
                config.port = number;
            }
            else 
            {
                throw std::invalid_argument("Unknown argument: " + arg);
            }
        }

        HT_Server server(config);
        server.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}