#pragma once

#include <any>
#include <arpa/inet.h>
#include <memory>
#include <stdexcept>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <csignal>
#include <sys/signalfd.h>
#include <utility>
#include <vector>
#include "epoll_event_queue.hpp"
#include "request.hpp"
#include "request_dispatcher.hpp"
#include "socket.hpp"
#include "promise.hpp"

namespace async {

class ServerSettingsException : public std::runtime_error {        
public:
    ServerSettingsException(const std::string msg): std::runtime_error{msg} {}
};

class AsyncServer {
private:
    using event_queue_ptr = async::EpollEventQueue * ;
    using dispatcher_ptr = std::unique_ptr<async::RequestDispatcher>;
    
    int server_fd;
    int sigint_fd;
    event_queue_ptr eventQueue;
    dispatcher_ptr dispatcher;

    void setSIGINT(sigset_t& mask) 
    {
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        if (-1 == sigprocmask(SIG_BLOCK, &mask, 0)) {
            throw async::ServerSettingsException("Server.setSIGINT() error...");
        }
    }

    void createAndBindSocket(async::AsyncSocketData& socket_data) 
    {
        // Definizione dei parametri della connessione
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_port = htons(socket_data.getPort());

        // Creazione del file descriptor per la socket
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            throw async::ServerSettingsException("Server socket initialization error...");
        }
    
        const std::string str_host = socket_data.getHost();
        if (! socket_data.isHostIPv4Address()) {
            // Risolviamo il nome di dominio in un indirizzo IP
            struct hostent* server = gethostbyname(str_host.c_str());
            if (server == NULL) {
                throw async::ServerSettingsException("Domain Name resolution error...");
            }
            memcpy((char *)&address.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
        }
        else {
            // Converto l'indirizzo IP da stringa a formato binario
            if (inet_pton(AF_INET, str_host.c_str(), &address.sin_addr) <= 0) {
                throw async::ServerSettingsException("Invalid IPv4 address in AsyncServer...");
            }
        }

        // Associazione della socket all'indirizzo e alla porta specificati
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            throw async::ServerSettingsException("Failed socket bind...");
        }
    }

    void startListening() 
    {
        if(listen(server_fd, SOMAXCONN) == -1) {
            close(server_fd);
            throw async::ServerSettingsException("Server.startListening() error...");
        }
    }

    bool makeSocketNonBlocking(const int sfd) 
    {
        int flags = fcntl(sfd, F_GETFL, 0);
        if(flags == -1)
            return false;
    
        flags |= O_NONBLOCK;
        return fcntl(sfd, F_SETFL, flags) != -1;
    }

    void setupSignalFd(sigset_t& mask) {
        sigint_fd = signalfd(-1, &mask, 0);
        if (sigint_fd == -1) {
            throw async::ServerSettingsException("Server.setupSignalFd() error...\n");
        }
    }

    void acceptNewClientSocket() 
    {
        struct sockaddr in_addr;
        char hbuf[NI_MAXHOST] = {};
        char sbuf[NI_MAXSERV] = {};
        bool has_client_info = false;

        socklen_t in_len = sizeof(in_addr);
        int client_socket_fd = accept(server_fd, &in_addr, &in_len);
        if(client_socket_fd == -1) {
            fprintf(stderr, "Cannot accept this client socket...\n");
            return;
        }
        
        if (0 == getnameinfo(&in_addr, in_len,
                        hbuf, NI_MAXHOST,
                        sbuf, NI_MAXSERV,
                        NI_NUMERICHOST | NI_NUMERICSERV)) 
        {
            printf("Accepted connection on descriptor %d(host=%s, port=%s)\n", client_socket_fd, hbuf, sbuf);
            has_client_info = true;
        }

        if(! makeSocketNonBlocking(client_socket_fd)) {
            if (has_client_info)
                fprintf(stderr, "Cannot make client socket %d (host=%s, port=%s) non-blocking. Closing socket...\n", client_socket_fd, hbuf, sbuf);
            else
                fprintf(stderr, "Cannot make client socket %d non-blocking. Closing socket...\n", client_socket_fd);
            close(client_socket_fd);
            return;
        }

        if (! eventQueue->addEvent(client_socket_fd, true)) {
            if (has_client_info)
                fprintf(stderr, "Cannot add client socket %d (host=%s, port=%s) to the Event Queue. Closing socket...\n", client_socket_fd, hbuf, sbuf);
            else
                fprintf(stderr, "Cannot add client socket %d to the Event Queue. Closing socket...\n", client_socket_fd);
            close(client_socket_fd);
            return;
        }
    }

public:
    AsyncServer(async::AsyncSocketData& socket_data, event_queue_ptr _eventQueue, dispatcher_ptr&& _dispatcher)
    :   eventQueue{_eventQueue},
        dispatcher{std::move(_dispatcher)}
    {
        sigset_t mask;
        setSIGINT(mask);
        createAndBindSocket(socket_data);
        if(! makeSocketNonBlocking(server_fd)) {
            close(server_fd);
            throw async::ServerSettingsException("Server Socket cannot be made non blocking...");
        }
        startListening();
        if (! eventQueue->addEvent(server_fd, true)) {
            close(server_fd);
            throw async::ServerSettingsException("Cannot add server_fd to event queue...");
        }
        setupSignalFd(mask);
        if (! eventQueue->addEvent(sigint_fd, false)) {
            close(server_fd);
            close(sigint_fd);
            throw async::ServerSettingsException("Cannot add sigint_fd to event queue...");
        }

        printf("Async Server created successfully.\n");
    }

    ~AsyncServer() {
        fprintf(stdout, "Async Server closed.\n");
    }

    void runEventLoop() 
    {
        std::vector<Request> requests;

        printf("Async Server event loop started...\n");
        while (true) 
        {
            const int event_count = eventQueue->waitForEvents(requests.empty() ? -1 : 0);

            for(int i = 0; i < event_count; ++i)
            {
                const int client_fd = eventQueue->getEvent(i);

                if (eventQueue->isError(i)) {
                    eventQueue->removeEvent(client_fd);
                    continue;
                }
                else if (client_fd == sigint_fd) {
                    printf("Server interrupted...\n");
                    return;
                }
                else if (client_fd == server_fd) {
                    acceptNewClientSocket();
                }
                else {
                    Request& request = requests.emplace_back(client_fd, eventQueue);
                    dispatcher->dispatch(request);
                    async::Promise& promise = request.getPromise();
                    promise.then([&r=request](std::any param) {
                        auto buffer = std::any_cast<std::array<char, 1000>>(param);
                        int fd = r.getFd();
                        __uint64_t size = buffer.size();

                        if(-1 == write(1, buffer.data(), size)) {
                            fprintf(stderr, "Cannot write on the terminal...\n");
                        }

                        if(-1 == write(fd, buffer.data(), size)) {
                            fprintf(stderr, "Cannot write on the client socket %d...\n", fd);
                        }
                    })
                    .then([&r=request](std::any param) {
                        int fd = r.getFd();
                        const char* testo = "FINE";
                        if(-1 == write(fd, testo, strlen(testo))) {
                            fprintf(stderr, "Cannot write on the client socket %d...\n", fd);
                        }
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
            }

            printf("Requests List Size: %ld\n", requests.size());
            __uint64_t size = requests.size();
            for (__uint64_t i = 0; i < size; ++i) {
                if (! requests[i].getPromise().poll()) {
                    requests[i--] = requests.back();
                    requests.pop_back();
                    --size;
                }
            }
        }
    }
};

} // namespace async