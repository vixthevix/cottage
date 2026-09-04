#ifndef TCPSETUP_COT
#define TCPSETUP_COT

#include "dependencies_cot.h"
#include "init_cot.h"
#include "error_cot.h"
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

//structs and stuff for multi-user server

typedef struct CotPoll {
    int init_fd;
    struct epoll_event sitter;
    struct epoll_event* clients;
    int maxClientCount;
} CotPoll;

typedef struct ServerConfig {
    char address[50];
    char port[50];
    uint32_t client_max;
    int server_fd;
    CotPoll poll;
} ServerConfig;


bool serverListen(int socketfd, int maxClientCount);
void serverClose(ServerConfig* server);
cotResult CotPollInit(CotPoll* input, int server_fd, int maxClientCount);
bool CotPollClose(CotPoll list);
void serverCloseClient(int clientfd);

//helper function for making non-blocking socket
bool applyNonBlocking(int fd) {
    //get existing config flags
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags <= -1) {
        newResultError("applyNonBlocking: could not get fd flags.");
        return false;
    }

    //add the non blocking flag
    flags |= O_NONBLOCK;

    //set the flag
    int status = fcntl(fd, F_SETFL, flags);
    if (status <= -1) {
        newResultError("applyNonBlocking: could not set fd flags.");
        return false;
    }

    return true;
}


//TCP stuff
ServerConfig* serverInit(const char* address, const char* port, uint32_t client_max) {
    cottageCheck(NULL);
    if (!port || strlen(port) <= 0) return NULL;
    if (client_max == 0) return NULL;

    struct addrinfo settings, *results;
    int status, fd;
    bool passive = (address == NULL);

    memset(&settings, 0, sizeof(struct addrinfo));
    settings.ai_family = AF_UNSPEC;
    settings.ai_socktype = SOCK_STREAM; //TCP
    if (passive) settings.ai_flags = AI_PASSIVE;

    if (!passive) status = getaddrinfo(address, port, &settings, &results);
    else status = getaddrinfo(NULL, port, &settings, &results);

    if (status < 0) {
        freeaddrinfo(results);
        newResultError("serverInit: could not get address info from parameters.");
        return NULL;
    }


    fd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
    if (fd <= -1) {
        freeaddrinfo(results);
        newResultError("serverInit: could not setup socket.");
        return NULL;
    }

    //free the port for other programs so its safe to use for this one
    //the last two parameters are for setting the change to true (1) ie yeah make the change 
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &((int){1}), sizeof(int)) <= -1) {
        freeaddrinfo(results);
        newResultError("serverInit: could not free input port.");
        return NULL;
    }

    //now bind
    if (bind(fd, results->ai_addr, results->ai_addrlen) <= -1) {
        freeaddrinfo(results);
        newResultError("serverInit: could not bind server.");
        return NULL;
    }

    //now we are done
    freeaddrinfo(results);

    //to automate the process, we also set to listening and non blocking

    if (!serverListen(fd, client_max)) {
        close(fd);
        return NULL;
    }

    if (!applyNonBlocking(fd)) {
        close(fd);
        return NULL;
    }

    CotPoll server_poll;
    if (CotPollInit(&server_poll, fd, client_max).status == COT_ERROR) {
        close(fd);
        return NULL;
    }

    ServerConfig* target = (ServerConfig*)malloc(sizeof(ServerConfig));
    strncpy(target->address, address, 50);
    strncpy(target->port, port, 50);
    target->client_max = client_max;
    target->server_fd = fd;
    target->poll = server_poll;

    return target;
}

bool serverListen(int socketfd, int maxClientCount) {
    cottageCheck(false);
    int status = listen(socketfd, maxClientCount);
    if (status <= -1) {
        newResultError("serverListen: failed to listen succesfully.");
        return false;
    }
    return true;
}

//may change to one parameter only 
//if client address specification really not needed
int serverAcceptClient(ServerConfig* server) {
    cottageCheck(-1);
    int fd = accept(server->server_fd, NULL, NULL); 
    if (fd <= -1) newResultError("serverAcceptClient: failed to accept client.");

    if (!applyNonBlocking(fd)) {
        serverCloseClient(fd);
        return -1;
    }

    return fd;
}

char* serverRecvClient(int clientfd) {
    cottageCheck(NULL);
    const int bufferSize = 2048;

    char* buffer = (char*) calloc(bufferSize, sizeof(char));
    int bytesrecv = recv(clientfd, buffer, bufferSize, 0);
    if (bytesrecv > 0) return buffer; 
    else {
        free(buffer);
        newResultError("serverRecvClient: failed to receive any bytes.");
        return NULL;
    }
}

void serverCloseClient(int clientfd) {
    cottageCheck();
    //clientreqfree(request);
    shutdown(clientfd, SHUT_WR);
    close(clientfd); //end current interraction
}

void serverClose(ServerConfig* server) {
    cottageCheck();
    if (!server) return;
    close(server->server_fd);
    CotPollClose(server->poll);
}


cotResult CotPollInit(CotPoll* input, int server_fd, int maxClientCount) {
    //cottageCheck(target);

    CotPoll target;
    memset(&target, 0, sizeof(CotPoll));

    //start up fd
    target.init_fd = epoll_create1(0); //no flags
    if (target.init_fd <= -1) return newResultError("CotPollInit: could not make init fd.");

    //set up the sitter, which will listen for new clients
    target.sitter.events = EPOLLIN;
    target.sitter.data.fd = server_fd;
    epoll_ctl(target.init_fd, EPOLL_CTL_ADD, server_fd, &target.sitter);

    //make clients list
    target.clients = (struct epoll_event*) calloc(maxClientCount, sizeof(struct epoll_event));
    if (!target.clients) {
        close(target.init_fd);
        return newResultError("CotPollInit: could not set up client buffer.");
    }
    
    target.maxClientCount = maxClientCount;

    *input = target;

    return newResultOK();
} 

int CotPollPoll(CotPoll list) {
    return epoll_wait(list.init_fd, list.clients, list.maxClientCount, -1);
}

int CotPollAccess(CotPoll list, int index) {
    if (index < 0 || index >= list.maxClientCount) return -1;
    return list.clients[index].data.fd;
}

bool CotPollPush(CotPoll list, int clientfd) {
    list.sitter.events = EPOLLIN;
    list.sitter.data.fd = clientfd;
    epoll_ctl(list.init_fd, EPOLL_CTL_ADD, clientfd, &list.sitter);
    return true;
}

bool CotPollPop(CotPoll list, int clientfd) {
    epoll_ctl(list.init_fd, EPOLL_CTL_DEL, clientfd, NULL);
    return true;
}

bool CotPollClose(CotPoll list) {
    close(list.init_fd);
    if (list.clients) free(list.clients);
    return true;
}

#endif