#include "serversource/serversource.h"


int main(void) {
    
    const char* ADDRESS = "0.0.0.0";
    const char* PORT = "8080";

    int socketfd = getMainFD(ADDRESS, PORT, true);
    if (socketfd <= -1) return 1;

    //now listen for any incoming connections
    
    //FOR THE SAKE OF TESTING, i will be doing only one connection at a time.
    //However, eventually polling will be introduced


    const int clientInit = 1; //initial size
    
    if (listen(socketfd, clientInit) <= -1) {
        printf("error listening\n");
        close(socketfd);
        return 1;
    }

    //after listening, get the client
    
    //this is where the main loop goes. in the most basic sense, a server gets an accept requests, sends the html page, and then closes the connection. this repeats over and over. its continousy opening and closing a connection.
    
    //another thing we can do is set up a hashmap of website links to server directories
    //e.g. map www.website.com/main -> ./templates/main
    //we can still use our qmap

    queryMap* linkmap = qmapInit();
    qmapInsert(linkmap, "/", "./templates/main");
    qmapInsert(linkmap, "/main", "./templates/main");
    qmapInsert(linkmap, "/cool", "./templates/cool");

    while (true) {

        int clientfd = accept(socketfd, NULL, NULL); //set to NULL for now since idk if we need any client info
        if (clientfd < 0) continue; //error handling

        //after getting a client, send them some html
        //to do this, we must answer the GET request maybe we'll see
        
        //we will now not ignore GET
        clientreq request = getClientRequest(clientfd);

        //for now, we dont care about what the request is. though it is a get request.
        //we are testing the clientreq struct
        if (request.type == ERROR || request.data == NULL) printf("request resulted in error\n");
        else printf("the request type is %i, and the data contained is:\n%s\n", request.type, request.data);
        
        //now we want to check that request
        //in order to do that, we have to split up our GET data into 3 sections
        //file, query (?) and section (#) 
        if (request.type == GET) {
            getSplit getdata = splitGET(request);
            printf("getdata.link->%s\n", getdata.link);
            //getdata stores a link. we have to map this link to our server path
            char* path = qmapGet(linkmap, getdata.link);
            printf("path gotten\n");
            if (path != NULL) {    
                char* file = (char*) calloc(strlen(path) + sizeof(".html") + 2, sizeof(char));
                //strcpy(file, ".");
                strcpy(file, path);
                strcat(file, ".html");

                //now check this file exists.
                if (access(file, F_OK) == 0) {
                    printf("sending %s...\n", file);
                    sendHTML(file, clientfd);
                }
                else { //send an error eventually
                    printf("error sending %s\n", file);
                } 
                free(getdata.link);
                free(file);
            }
            else printf("no path\n");
            //we just have to concat .html to render it.
            
            //also, print every key value pair in query
            if (getdata.qmap) {
                printf("key value pairs:\n");
                for (int i = 0; i < getdata.qmap->capacity; i++) {
                    if (getdata.qmap->items[i] != NULL) {
                        printf("%s->%s\n", getdata.qmap->items[i]->key, getdata.qmap->items[i]->value);
                    }
                }

                qmapFree(getdata.qmap);
            }

        }

        //const char* filepath = "./main.html";
        //sendHTML(filepath, clientfd);
    
        //int bytesrecv = recv(clientfd, NULL, 0, 0);
        
        //we need to end gracefully and safely
        clientreqfree(request);
        shutdown(clientfd, SHUT_WR);
        close(clientfd); //end current interraction
    }
    
    close (socketfd);
    return 0;
}
