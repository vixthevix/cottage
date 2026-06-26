#ifndef TCPSETUP_COT
#define TCPSETUP_COT

#include "dependencies_cot.h"

//TCP stuff
int serverInit(const char* address, const char* port, bool passive) {
    struct addrinfo settings, *results;
    int status, fd;
    
    memset(&settings, 0, sizeof(struct addrinfo));
    settings.ai_family = AF_UNSPEC;
    settings.ai_socktype = SOCK_STREAM; //TCP
    if (passive) settings.ai_flags = AI_PASSIVE;

    if (!passive) status = getaddrinfo(address, port, &settings, &results);
    else status = getaddrinfo(NULL, port, &settings, &results);

    if (status < 0) {
        printf("error getting addrinfo\n");
        freeaddrinfo(results);
        return -1;
    }


    fd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
    if (fd <= -1) {
        printf("error getting fd\n");
        freeaddrinfo(results);
        return -1;
    }

    //free the port for other programs so its safe to use for this one
    //the last two parameters are for setting the change to true (1) ie yeah make the change 
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &((int){1}), sizeof(int)) <= -1) {
        printf("error setting reuse address option for socket\n");
        freeaddrinfo(results);
        return -1;
    }

    //now bind
    if (bind(fd, results->ai_addr, results->ai_addrlen) <= -1) {
        printf("error binding\n");
        freeaddrinfo(results);
        return -1;
    }

    //now we are done

    freeaddrinfo(results);
    return fd;
}

bool serverListen(int socketfd, int maxClientCount) {
    return listen(socketfd, maxClientCount) > -1;
}

//may change to one parameter only 
//if client address specification really not needed
int serverAcceptClient(int socketfd, struct sockaddr* clientAddress, socklen_t* clientAddressLength) {
    return accept(socketfd, clientAddress, clientAddressLength);
}

char* serverGetRequest(int clientfd) {
    const int bufferSize = 2048;

    char* buffer = (char*) calloc(bufferSize, sizeof(char));
    int bytesrecv = recv(clientfd, buffer, bufferSize, 0);
    if (bytesrecv > 0) return buffer; 
    else {
        free(buffer);
        return NULL;
    }
}

bool serverCloseClient(int clientfd) {
    //clientreqfree(request);
    shutdown(clientfd, SHUT_WR);
    close(clientfd); //end current interraction
}

int serverClose(int socketfd) {
    return close(socketfd);
}


#endif