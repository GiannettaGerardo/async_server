#include <iostream>
#include <memory>
#include "epoll_event_queue.hpp"
#include "request_dispatcher.hpp"
#include "server.hpp"

int main(int argc, char *argv[]) 
{
    if (argc != 4) {
        std::cerr 
            << "Invalid arguments number...\n" 
            << "Use in this way: ./async_server <host> <port> <max_events_number>\n"
            << "Note: host can be a DNS domain or an IPv4 address.\n";
        return 1;
    }
    int return_value = 0;
    async::EpollEventQueue* event_queue = nullptr;

    try {

        async::AsyncSocketData server_socket(argv[1], argv[2], argv[3]);
        event_queue = new async::EpollEventQueue(server_socket.getMaxEvents());
        async::AsyncServer async_server(
            server_socket, 
            event_queue,
            std::make_unique<async::RequestDispatcher>(event_queue)
        );
        async_server.runEventLoop();

    }
    catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return_value = 1;
    }
    if (event_queue != nullptr)
        delete event_queue;

    return return_value;
}