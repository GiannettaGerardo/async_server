#pragma once

#include "epoll_event_queue.hpp"
#include "request.hpp"

namespace async
{

class RequestDispatcher {
    async::EpollEventQueue* eventQueue;

public:
	RequestDispatcher(async::EpollEventQueue* _eventQueue)
    :   eventQueue(_eventQueue) 
    {}

	~RequestDispatcher() {}
    
	void dispatch(async::Request& request) 
    {
        async::Promise& promise = request.getPromise();
        promise.then([&r=request](std::any param) {
            auto buffer = std::any_cast<std::array<char, 1000>>(param);
            int fd = r.getFd();
            __uint64_t size = buffer.size();
        
            if(-1 == write(1, buffer.data(), size)) {
                std::cerr << "Cannot write on the terminal...\n";
            }
        
            if(-1 == write(fd, buffer.data(), size)) {
                std::cerr << "Cannot write on the client socket " << fd << "\n";
            }
        })
        .then([&r=request](std::any param) {
            int fd = r.getFd();
            const char* testo = "FINE";
            if(-1 == write(fd, testo, 4)) {
                std::cerr << "Cannot write on the client socket " << fd << "\n";
            }
        })
        .finally([&](std::any param) {
            int fd = request.getFd();
            std::cout << "Closed connection on descriptor " << fd << "...\n";
            eventQueue->removeEvent(fd);
        });
        /*bool done = false;

        while(true) {
            char buf[5] = {};

            const ssize_t count = read(client_fd, buf, sizeof(buf));
            if(count == -1) {
                if(errno != EAGAIN && errno != EWOULDBLOCK) {
                    done = true;
                }
                break;
            }
            else if (count == 0) {
                done = true;
                break;
            }

            if(-1 == write(1, buf, (count < 5) ? count : 5)) {
                fprintf(stderr, "Cannot write on the terminal...\n");
            }

            if(-1 == write(client_fd, buf, (count < 5) ? count : 5)) {
                fprintf(stderr, "Cannot write on the client socket %d...\n", client_fd);
                done = true;
                break;
            }
        }
        if (done) {
            printf("Closed connection on descriptor %d...\n", client_fd);
            eventQueue->removeEvent(client_fd);
        }*/
    }
};

} // namespace async