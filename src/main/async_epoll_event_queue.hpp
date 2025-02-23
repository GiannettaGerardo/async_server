#pragma once

#include "async_event_queue.hpp"
#include <cstring>
#include <iostream>
#include <strings.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace async 
{

/*class SocketInfo {
private:
    const int fd;
    const std::string host;
    const std::string port;

public:
    SocketInfo(const int fd, std::string&& host, std::string&& port)
    :   fd{fd}, 
        host{std::move(host)}, 
        port{std::move(port)} 
    {}

    ~SocketInfo() {}

    inline int getFileDescriptor() {
        return fd;
    }

    inline std::string const& getHost() {
        return host;
    }

    inline std::string const& getPort() {
        return port;
    }
};*/

class EpollEventQueue : public EventQueue<int, int> {
private:
    using EpollEvent = struct epoll_event;

    int epoll_fd;
    const int max_events;
    EpollEvent event;
    EpollEvent* events;
    int event_count;
    //std::unordered_map<int, SocketInfo> openSockets;

public:
    EpollEventQueue(const int _max_events) 
    :   max_events{_max_events},
        event_count(0) 
    {
        if (max_events <= 0) {
            throw async::EventQueueException("Invalid max events number...");
        }

        epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            throw async::EventQueueException("Failed to intantiate epoll...");
        }

        events = new EpollEvent[max_events];
        memset(events, 0, _max_events);
    }

    ~EpollEventQueue() {
        fprintf(stdout, "Closing and cleaning the Epoll Event Queue...\n");
        close(epoll_fd);
        fprintf(stdout, "Epoll Event Queue closed.\n");
        delete[] events;
    }

    bool addEvent(const int file_descriptor) {
        event.data.fd = file_descriptor; 
        event.events = EPOLLIN;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, file_descriptor, &event) != -1) {
            //openSockets.emplace(file_descriptor, std::move(info));
            return true;
        }
        return false;
    }

    inline int getEvent(const int eventIdx) {
        return events[eventIdx].data.fd;
    }

    // TODO
    void clearQueue() {
       
    }

    int waitForEvents(const int timeout) {
        std::cout << "Blocking and waiting for epoll event...\n";
        errno = 0;
        event_count = epoll_wait(epoll_fd, events, max_events, timeout);
        if (event_count < 0) {
            switch (errno) {
            case EBADF  : throw async::EventQueueException("epoll_fd is not a valid file descriptor...");
            case EFAULT : throw async::EventQueueException("The memory area pointed to by events is not accessible with write permissions....");
            case EINTR  : throw async::EventQueueException("The call was interrupted by a signal handler before either any of the requested events occurred or the timeout expired...");
            case EINVAL : throw async::EventQueueException("epoll_fd is not an epoll file descriptor....");
            default     : throw async::EventQueueException("Unrecognized error...");
            }
        }

        std::cout << "Received " << event_count << " events.\n";
        return event_count;
    }

    bool safeSkipErrorEvent(const int eventIdx) {
        if((events[eventIdx].events & EPOLLERR) ||
            (events[eventIdx].events & EPOLLHUP) ||
            (!(events[eventIdx].events & EPOLLIN)))
        {
            std::cout << "Closing a client socket...\n";
            close(events[eventIdx].data.fd);
            return true;
        }
        return false;
    }

    inline bool isEvent(const int eventIdx, const int fd) {
        return fd == events[eventIdx].data.fd;
    }
};

} // namespace async