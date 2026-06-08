// test_boost.cpp
#include <boost/filesystem.hpp>
#include <iostream>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <boost/bind/bind.hpp>
#include <muduo/net/EventLoop.h>

int main() {
    boost::filesystem::path p = "/usr/include/boost";
    std::cout << "Exists: " << boost::filesystem::exists(p) << std::endl;
    return 0;
}