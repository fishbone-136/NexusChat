// #include "ChatServer.hpp"
// #include <iostream>
// #include <signal.h>
// #include "ChatService.hpp"
// using namespace std;

// void resetHandler(int)
// {
//     ChatService::instance()->reset();
//     exit(0);
// }

// int main()
// {
//     signal(SIGINT, resetHandler);

//     EventLoop loop;
//     InetAddress addr("127.0.0.1", 6000);
//     ChatServer server(&loop, addr, "ChatServer");

//     server.start();
//     loop.loop();

//     return 0;
// }


#include "ChatServer.hpp"
#include <iostream>
#include <signal.h>
#include "ChatService.hpp"
using namespace std;

void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <ip> <port>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 127.0.0.1 6000" << std::endl;
        return -1;
    }

    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress addr(argv[1], atoi(argv[2]));   // 使用命令行参数
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}