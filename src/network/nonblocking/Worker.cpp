#include "Worker.h"

#include <iostream>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "Utils.h"
#include <../src/protocol/Parser.h>
#include <afina/execute/Command.h>

namespace Afina {
namespace Network {
namespace NonBlocking {

// See Worker.h
Worker::Worker(std::shared_ptr<Afina::Storage> ps) : pStorage(ps) {
    // TODO: implementation here
}

// See Worker.h
Worker::~Worker() {
    // TODO: implementation here
}

// See Worker.h
void Worker::Start(int server_socket) {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    this->server_socket = server_socket;
    running.store(true);
    auto args = new std::pair<Worker *, int>(this, server_socket);
    if (pthread_create(&thread, NULL, OnRunProxy, args) < 0) {
        throw std::runtime_error("Could not create worker thread");
    }
}

// See Worker.h
void Worker::Stop() {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    running.store(false);
    shutdown(server_socket, SHUT_RDWR);
}

// See Worker.h
void Worker::Join() {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    pthread_join(thread, NULL);
}

void *Worker::OnRunProxy(void *_args) {
    auto args = reinterpret_cast<std::pair<Worker *, int> *>(_args);
    Worker *worker = args->first;
    int server_socket = args->second;
    worker->OnRun(server_socket);
    return NULL;
}

bool Worker::readd(int socket){
    Protocol::Parser parser;
    size_t buf_size = 2048;
    uint32_t body_size;
    size_t parsed, all_parsed;
    ssize_t input_size;
    std::string command = "";
    std::string args = "";
    char buf[buf_size];

    while (running.load()) {
        if ((input_size = read(socket, buf, buf_size)) <= 0) {
            break;
        }
        command += buf;
        if (parser.Parse(buf, input_size, parsed)) {
            command.erase(0, parsed);
            auto command_ptr = parser.Build(body_size);
            parser.Reset();
            if (body_size > 0) {
                while (body_size + 2 > command.size()) {
                    input_size = read(socket, buf, buf_size);
                    command += buf;
                }

                args = command.substr(0, body_size);
                command.erase(0, body_size + 2);
            }
            std::string result;
            try {
                command_ptr->Execute(*pStorage, args, result);
            } catch (...) {
                result = "SERVER_ERROR";
            }
            result += "\r\n";
            if (result.size() && send(socket, result.data(), result.size(), 0) <= 0) {
                break;
            }
            parser.Reset();
        } else {
            command.substr(parsed);
            continue;
        }
    }
}

// See Worker.h
void Worker::OnRun(int server_socket) {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;

    server_socket = server_socket;
    // TODO: implementation here
    // 1. Create epoll_context here
    int epoll_fd = epoll_create(EPOLL_MAX_EVENTS);
    if (epoll_fd == -1) {
        throw std::runtime_error("Failed to epoll_context");
    }
    // 2. Add server_socket to context
    struct epoll_event server_event;
    server_event.events = EPOLLIN | EPOLLET | EPOLLHUP;
    server_event.data.ptr = (void *)&server_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &server_event) == -1) {
        throw std::runtime_error("Failed to epoll_ctl");
    }
    struct epoll_event events[EPOLL_MAX_EVENTS];
    while (running.load()) {
        int n = epoll_wait(epoll_fd, events, EPOLL_MAX_EVENTS, -1);
        if (n == -1) {
            throw std::runtime_error("Failed to epoll_wait");
        }

        for (int i = 0; i < n; ++i) {
            if (events[i].data.ptr == &server_socket) {
                // 3. Accept new connections, don't forget to call make_socket_nonblocking on
                //    the client socket descriptor
                int client_socket = accept(server_socket, NULL, NULL);
                if (client_socket == -1) {
                    if (errno == EINVAL || errno == EAGAIN || errno == EWOULDBLOCK) {
                        std::cout << "Accept returned EINVAL\n";
                        break;
                    } else {
                        throw std::runtime_error("Accept failed");
                    }
                }
                make_socket_non_blocking(client_socket);

                // 4. Add connections to the local context
                struct epoll_event client_event;
                client_event.events = EPOLLIN | EPOLLHUP | EPOLLERR;
                //auto connection = new Connection(client_socket);
                std::unique_ptr<Connection> p(new Connection(client_socket));
                client_event.data.ptr = &p;
                connections.emplace(client_socket, std::move(p));
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &client_event) == -1) {
                    throw std::runtime_error("Failed to client socket epoll_ctl");
                }
            } else {
                int client_socket = *(int *)events[i].data.ptr;
                // 5. Process connection events
                if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, NULL);
                    connections.erase(client_socket);
                } else if (events[i].events & EPOLLIN) {
                    if (readd(client_socket)) {
                        close(client_socket);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, NULL);
                        connections.erase(client_socket);
                    }
                } else {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, NULL);
                    connections.erase(client_socket);
                    throw std::runtime_error("Unknown event");
                }
            }
        }

        for (const auto &p : connections) {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, p.first, NULL);
        }
        connections.clear();
    }
}
} // namespace NonBlocking
} // namespace NonBlocking
} // namespace Network
