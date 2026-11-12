#include "HT_server.hpp"

int main()
{
    HT_Server server(8080);
    server.run();
    return 0;
}