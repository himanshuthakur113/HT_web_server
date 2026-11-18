#ifndef SCOPED_SOCKET_HPP
#define SCOPED_SOCKET_HPP

#include <unistd.h>
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

#endif