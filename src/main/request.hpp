#pragma once

#include "epoll_event_queue.hpp"
#include "promise.hpp"
#include <unistd.h>

namespace async
{

class Request {
    using event_queue_ptr = async::EpollEventQueue * ;

    int fd;
    async::Promise promise;
    event_queue_ptr eventQueue;

public:
    Request(int _fd, event_queue_ptr _eventQueue)
    :   fd{_fd}, eventQueue{_eventQueue} 
    {
        promise
        .then([this](std::any param) {
            printf("Hello from file descriptor %d\n", fd);

            const int bytes = 1000;
            std::array<char, bytes> buffer = {};

            const ssize_t count = read(fd, buffer.data(), bytes);
            if (count <= 0) {
                return promise.reject(count);
            }
            return promise.resolve(buffer);
        })
        .error([](std::any param) {
            ssize_t errorCount = std::any_cast<ssize_t>(param);
            fprintf(stderr, "Error count returned %ld\n", errorCount);
        })
        .finally([this](std::any param) {
            printf("Closed connection on descriptor %d...\n", fd);
            eventQueue->removeEvent(fd);
        });
    }

    inline int getFd() { return fd; }

    inline async::Promise &getPromise() { return promise; }
};

} // namespace async