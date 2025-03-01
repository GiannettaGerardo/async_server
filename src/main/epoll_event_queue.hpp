#pragma once

#include <cstring>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

namespace async 
{

class EventQueueException : public std::runtime_error {
public:
    EventQueueException(const std::string msg) : std::runtime_error{msg} {}
};

class EpollEventQueue {
private:
    using EpollEvent = struct epoll_event;

    int epoll_fd;
    const int max_events;
    EpollEvent event;
    EpollEvent* events;
    std::unordered_map<int, bool> fd_map;

    int customClose(int file_descriptor, bool isSocket) {
        if (isSocket) {
            #define CLOSE_SOCKET_MAX_BUFFER 500
            char buffer[CLOSE_SOCKET_MAX_BUFFER];
            shutdown(file_descriptor, SHUT_WR);
            while (read(file_descriptor, buffer, CLOSE_SOCKET_MAX_BUFFER) > 0);
            #undef CLOSE_SOCKET_MAX_BUFFER
        }
        return close(file_descriptor);
    }

public:
    EpollEventQueue(const int _max_events) 
    :   max_events{_max_events}
    {
        if (max_events <= 0) {
            throw EventQueueException("Invalid max events number ...");
        }

        epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            throw EventQueueException("Failed to intantiate epoll ...");
        }

        events = new EpollEvent[max_events];
        memset(events, 0, _max_events);
    }

    ~EpollEventQueue() {
        fprintf(stdout, "Closing and cleaning the Epoll Event Queue ...\n");

        for (auto [fd, isSocket] : fd_map) {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
            customClose(fd, isSocket);
        }

        close(epoll_fd);

        delete[] events;

        fprintf(stdout, "Epoll Event Queue cleared.\n");
    }

    bool addEvent(const int file_descriptor, const bool isSocket) {
        event.data.fd = file_descriptor; 
        event.events = EPOLLIN;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, file_descriptor, &event) != -1) {
            fd_map[file_descriptor] = isSocket;
            return true;
        }
        return false;
    }

    bool removeEvent(const int file_descriptor) {
        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, file_descriptor, NULL) != -1) {
            customClose(file_descriptor, fd_map[file_descriptor]);
            fd_map.erase(file_descriptor);
            return true;
        }
        return false;
    }

    inline int getEvent(const int eventIdx) {
        return events[eventIdx].data.fd;
    }

    int waitForEvents(const int timeout) {
        if (timeout == -1) 
            std::cout << "\nWaiting for epoll events ...\n";

        errno = 0;
        const int event_count = epoll_wait(epoll_fd, events, max_events, timeout);
        if (event_count < 0) {
            switch (errno) {
            case EBADF  : throw EventQueueException("epoll_fd is not a valid file descriptor...");
            case EFAULT : throw EventQueueException("The memory area pointed to by events is not accessible with write permissions....");
            case EINTR  : throw EventQueueException("The call was interrupted by a signal handler before either any of the requested events occurred or the timeout expired...");
            case EINVAL : throw EventQueueException("epoll_fd is not an epoll file descriptor....");
            default     : throw EventQueueException("Unrecognized error...");
            }
        }

        if (timeout != 0) 
            std::cout << "Received " << event_count << " events.\n";
        
        return event_count;
    }

    inline bool isError(const int eventIdx) {
        /*if((events[eventIdx].events & EPOLLERR) ||
            (events[eventIdx].events & EPOLLHUP) ||
            (!(events[eventIdx].events & EPOLLIN)))*/
        return ! (events[eventIdx].events & EPOLLIN);
    }
};

} // namespace async