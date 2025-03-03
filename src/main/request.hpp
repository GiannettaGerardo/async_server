#pragma once

#include "promise.hpp"
#include <unistd.h>
#include <iostream>

namespace async
{

class Request {
    int fd;
    async::Promise promise;

public:
    Request(int _fd)
    :   fd{_fd}
    {
        promise
        .then([this](std::any param) {
            std::cout << "Hello from file descriptor " << fd << "\n";

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
            std::cerr << "Error count returned " << errorCount << "\n";
        });
    }

    inline int getFd() { return fd; }

    inline async::Promise &getPromise() { return promise; }
};

} // namespace async