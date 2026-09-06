/*
Code for setting up a multi-client server.

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

#ifndef TCPSETUP_COT
#define TCPSETUP_COT

#include "dependencies_cot.h"
#include "init_cot.h"
#include "error_cot.h"
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

/*
Struct that holds data for polling multiple clients.
@param init_fd -> file descriptor needed to init poll.
@param sitter -> captures new clients to add to client list.
@param clients -> list of clients to communicate with.
@param maxClientCount -> max number of clients at a time.
*/
typedef struct CotPoll {
    int init_fd;
    struct epoll_event sitter;
    struct epoll_event* clients;
    int maxClientCount;
} CotPoll;

/*
Struct that holds data for setting up a cottage server.
@param address -> address of server.
@param port -> port of server.
@param server_fd -> file descriptor of server socket.
@param poll -> polling data for multi-client functionality.
*/
typedef struct ServerConfig {
    char address[50];
    char port[50];
    int server_fd;
    CotPoll poll;
} ServerConfig;


void serverClose(ServerConfig* server);
cotResult CotPollInit(CotPoll* input, int server_fd, int maxClientCount);
void CotPollClose(CotPoll poll);
void serverCloseClient(int clientfd);

//helper function for making non-blocking socket
/*
Applies the non-blocking attribute to a file descriptor for reading data.
@arg fd -> target to apply attribute to.
@return status of apply.
*/
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


/*
Initialises a TCP server.
@arg address -> address of server.
@arg port -> port of server.
@arg client_max -> max number of clients at a time.
*/
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
        close(fd);
        newResultError("serverInit: could not free input port.");
        return NULL;
    }

    if (bind(fd, results->ai_addr, results->ai_addrlen) <= -1) {
        freeaddrinfo(results);
        close(fd);
        newResultError("serverInit: could not bind server.");
        return NULL;
    }

    //Done with results now.
    freeaddrinfo(results);

    if (listen(fd, client_max) <= -1) {
        close(fd);
        newResultError("serverInit: failed to listen succesfully.");
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
    target->server_fd = fd;
    target->poll = server_poll;

    return target;
}

/*
Checks if a client is available to connect with.
@arg server -> cottage server in use.
@return file descriptor of client or -1.
*/
int serverAcceptClient(ServerConfig* server) {
    cottageCheck(-1);
    int fd = accept(server->server_fd, NULL, NULL); 
    if (fd <= -1) newResultError("serverAcceptClient: failed to accept client.");
    else if (!applyNonBlocking(fd)) {
        serverCloseClient(fd);
        return -1;
    }

    return fd;
}

/*
Reads data sent by client.
@arg clientfd -> file descriptor of client.
@return dynamically created buffer with client data. 
*/
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

/*
Gracefully closes a client connection.
@arg clientfd -> file descriptor of client.
*/
void serverCloseClient(int clientfd) {
    cottageCheck();
    shutdown(clientfd, SHUT_WR);
    close(clientfd); //end current interraction
}

/*
Gracefully closes a server and frees it from memory.
@arg server -> cottage server to close.
*/
void serverClose(ServerConfig* server) {
    cottageCheck();
    if (!server) return;
    close(server->server_fd);
    CotPollClose(server->poll);
}

/*
Initialises a CotPoll object.
@arg input -> stores created CotPoll.
@arg server_fd -> server file descriptor, needed for setup.
@arg maxClientCount -> max number of clients at time.
@return error status of init.
*/
cotResult CotPollInit(CotPoll* input, int server_fd, int maxClientCount) {
    cottageCheck(newResultError("CotPollInit: cottage not initialised."));

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

/*
Checks for number of clients waiting to be polled.
@arg poll -> CotPoll to check.
@return number of clients to be polled.
*/
int CotPollPoll(CotPoll poll) {
    cottageCheck(0);
    return epoll_wait(poll.init_fd, poll.clients, poll.maxClientCount, -1);
}

/*
Accesses a client fd in a polling list.
@arg poll -> CotPoll to access.
@arg index -> index to access poll data at.
@return fd of client at index.
*/
int CotPollAccess(CotPoll poll, int index) {
    cottageCheck(-1);
    if (index < 0 || index >= poll.maxClientCount) return -1;
    return poll.clients[index].data.fd;
}

/*
Pushes a client fd into the poll data.
@arg poll -> CotPoll to push data onto.
@arg clientfd -> client file descriptor to push.
*/
void CotPollPush(CotPoll poll, int clientfd) {
    cottageCheck();
    poll.sitter.events = EPOLLIN;
    poll.sitter.data.fd = clientfd;
    epoll_ctl(poll.init_fd, EPOLL_CTL_ADD, clientfd, &poll.sitter);
}

/*
Removes a client fd from a poll.
@arg poll -> CotPoll to remove data from.
@arg clientfd -> client file descriptor to remove.
*/
void CotPollPop(CotPoll poll, int clientfd) {
    epoll_ctl(poll.init_fd, EPOLL_CTL_DEL, clientfd, NULL);
}

/*
Gracefully closes and frees a CotPoll.
@arg poll -> CotPoll to close.
*/
void CotPollClose(CotPoll poll) {
    close(poll.init_fd);
    if (poll.clients) free(poll.clients);
}

#endif