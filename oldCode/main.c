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
    qmapInsert(linkmap, "/", "./templates/main.html");
    qmapInsert(linkmap, "/main", "./templates/main.html");
    qmapInsert(linkmap, "/cool", "./templates/cool.html");
    qmapInsert(linkmap, "/input", "./templates/input.html");
    qmapInsert(linkmap, "/main.css", "./styles/main.css");

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

            //lets store some variables
            //we should have a function for combining two queryMaps together, with the structure looking like
            //qmapCombine(q1, q2) --> (q1 gets inserted into q2)
            //this is to keep getdata.variables NULL, and to make life cleaner.
            queryMap* temp = qmapInit();
            queryMap* vars = qmapCombine(getdata.variables, temp);
            qmapFree(temp);

            qmapInsert(vars, "fart", "poop");

            //we get a path from the linkmap. if sending this path doesnt work, we just ask for the link
            //this is very dangerous as we essentially open up our whole folder here.
            //we will fix this security issue later
            //like probably will require string cleaning and blocking paths
            //idk

            //we need to add the . to the front of getdata.link


            char* path = qmapGet(linkmap, getdata.link);
            printf("path is %s\n", path);
            if (!sendFile(path, clientfd, vars)) {
                char* rawLink = (char*) calloc(strlen(getdata.link) + 10, sizeof(char));
                for (int i = 1, j = 0; j < strlen(getdata.link); i++, j++) {
                    rawLink[i] = getdata.link[j];
                }
                rawLink[0] = '.';
                if (!sendFile(rawLink, clientfd, vars)) sendError(clientfd, ERROR_404);
                free(rawLink);
            }
            
            free(getdata.link);
            
            //also, print every key value pair in query
            if (getdata.variables) {
                printf("key value pairs:\n");
                for (int i = 0; i < getdata.variables->capacity; i++) {
                    if (getdata.variables->items[i] != NULL) {
                        printf("%s->%s\n", getdata.variables->items[i]->key, getdata.variables->items[i]->value);
                    }
                }

                qmapFree(getdata.variables);
            }

            qmapFree(vars);

        }
        else if (request.type == POST) {
            printf("POST REQUEST GOT\n");
            //just print out the data and resend input
            //we need a splitPOST function eventually
            postSplit postdata = splitPOST(request);
            printf("\n\nlink is %s,\ninput is %i\n\n", postdata.link, (postdata.input != 0));
            //just send the html and send the link
            queryMap* temp = qmapInit();
            queryMap* vars = qmapCombine(postdata.input, temp);
            qmapFree(temp);

            qmapInsert(vars, "fart", "ass");

            printf("\n\nvars done\n\n");
            if (postdata.input) {
                printf("key value pairs:\n");
                for (int i = 0; i < postdata.input->capacity; i++) {
                    if (postdata.input->items[i] != NULL) {
                        printf("%s->%s\n", postdata.input->items[i]->key, postdata.input->items[i]->value);
                    }
                }

            }

            char* path = qmapGet(linkmap, postdata.link);
            printf("path gotten\n");
            if (path != NULL) {    
                // char* file = (char*) calloc(strlen(path) + sizeof(".html") + 2, sizeof(char));
                // //strcpy(file, ".");
                // strcpy(file, path);
                // strcat(file, ".html");

                //now check this file exists.
                if (access(path, F_OK) == 0) {
                    printf("sending %s...\n", path);
                    if (!sendHTML(path, clientfd, vars)) {
                        sendError(clientfd, ERROR_404); //change the error
                    }
                    else printf("HTML send\n");
                }
                else { //send an error eventually
                    printf("error sending %s\n", path);
                } 
                free(postdata.link);
            }
            else {
                printf("no path\n");
                sendError(clientfd, ERROR_404);
            }
            //we just have to concat .html to render it.

            //getdata stores a link. we have to map this link to our server path
            //sendRediret(postdata.link, clientfd);
            //printf("send REDIRECT\n");
            
            // if (postdata.input) {
            //     printf("key value pairs:\n");
            //     for (int i = 0; i < postdata.input->capacity; i++) {
            //         if (postdata.input->items[i] != NULL) {
            //             printf("%s->%s\n", postdata.input->items[i]->key, postdata.input->items[i]->value);
            //         }
            //     }

            //     qmapFree(postdata.input);
            // }

            qmapFree(vars);

        }

        //const char* filepath = "./main.html";
        //sendHTML(filepath, clientfd);
    
        //int bytesrecv = recv(clientfd, NULL, 0, 0);
        
        //we need to end gracefully and safely
        clientreqfree(request);
        shutdown(clientfd, SHUT_WR);
        close(clientfd); //end current interraction
    }
    
    close(socketfd);
    return 0;
}
