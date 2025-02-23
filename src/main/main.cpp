#include <cstdlib>
#include <iostream>
#include "async_epoll_event_queue.hpp"
#include "async_server.hpp"

int main(int argc, char *argv[]) 
{
    if (argc != 4) {
        std::cerr << "Invalid arguments number...\nEnter in this order: host port max_events_number\nNote: host can be a DNS domain or an IPv4 address.\n";
        return 1;
    }

    try {

        async::AsyncSocketData server_socket(argv[1], argv[2], argv[3]);
        auto event_queue = std::make_unique<async::EpollEventQueue>(server_socket.getMaxEvents());
        async::AsyncServer async_server(std::move(server_socket), std::move(event_queue));
        async_server.runEventLoop();

    }
    /*catch(async::AsyncSocketException& e) {
        std::cerr << e.what() << std::endl;
    }
    catch(async::ServerSettingsException& e) {
        std::cerr << e.what() << std::endl;
    }
    catch(async::EventQueueException& e) {
        std::cerr << e.what() << std::endl;
    }*/
    catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}