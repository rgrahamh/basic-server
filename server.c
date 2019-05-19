#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define MAX_IN_BUFF_SIZE 4096 //The max request size
#define MAX_OUT_BUFF_SIZE 4194304 //The max response size
#define MAX_PATH_LEN 120 //The maximum path length
#define MAX_REQ 30 //The max number of header lines parsed
#define MAX_REQ_SIZE 135 //The max length of each header

//Enum for the type of requests
enum request_types {GET, HEAD};

/**
 * Gets the time in the HTTP Date header format (a lot of credit goes to pmg on Stack Overflow for this one)
 * @return The char array that contains a properly formatted timestamp for the current time
 */
char *getTime(){
    char* buff = malloc(30);
    time_t current_time = time(NULL);
    struct tm *time = gmtime(&current_time);

    strftime(buff, sizeof buff, "%a, %d, %b, %Y, %H:%M:%S %Z", time);

    return buff;
}

/**
 * Parses a file extension for a given path and returns the Content-Type
 * @param path The filepath
 * @return The Content-Type
 */
char *parseExtension(const char *path, int path_len){
    //Holds the content type
    char *cont = malloc(22);
    memset(cont, 0, 22);

    for(int i = 0; i + 2 < path_len; i++){
        //Files with two character extensions
        if(path[i] == '.' && path[i+1] == 'j' && path[i+2] == 's'){
            cont = "application/javascript";
        }

        //Files with three character extensions
        if(i + 3 < path_len){
            if(path[i] == '.' && path[i+1] == 't' && path[i+2] == 'x' && path[i+3] == 't'){
                cont = "text/plain";
            }
            else if(path[i] == '.' && path[i+1] == 'c' && path[i+2] == 's' && path[i+3] == 's'){
                cont = "text/css";
            }
            else if(path[i] == '.' && path[i+1] == 'p' && path[i+2] == 'd' && path[i+3] == 'f'){
                cont = "application/pdf";
            }
            else if(path[i] == '.' && path[i+1] == 'p' && path[i+2] == 'n' && path[i+3] == 'g'){
                cont = "image/png";
            }
            else if(path[i] == '.' && path[i+1] == 'g' && path[i+2] == 'i' && path[i+3] == 'f'){
                cont = "image/gif";
            }
            else if(path[i] == '.' && path[i+1] == 'j' && path[i+2] == 'p' && path[i+3] == 'g'){
                cont = "image/jpeg";
            }

            //Files with four character extensions
            if(i + 4 < path_len){
                if(path[i] == '.' && path[i+1] == 'h' && path[i+2] == 't' && path[i+3] == 'm' && path[i+4] == 'l'){
                    cont = "text/html";
                }
                else if((path[i] == '.' && path[i+1] == 'j' && path[i+2] == 'p' && path[i+3] == 'e' && path[i+4] == 'g')){
                    cont = "image/jpeg";
                }
            }
        }
    }

    return cont;
}

/**
 * Handles an HTTP request
 * @param sockfd The socket file descriptor to send a response over
 * @param in_buff The in buffer containing the HTTP request
 * @param bytes_recv The length of in_buff
 * @param path The path to the reqeusted file
 * @return 0 if successful in sending a request,
 */
int handleRequest(int sockfd, char *in_buff, int bytes_recv, char** path){
    //The request 2D array
    char request[MAX_REQ][MAX_REQ_SIZE];
    memset(request, 0, MAX_REQ * MAX_REQ_SIZE);

    //The out buffer
    char out_buff[MAX_OUT_BUFF_SIZE];
    memset(out_buff, 0, MAX_OUT_BUFF_SIZE);

    //Parsing the request
    int req = 0;        //The current request number
    int req_offset = 0; //The current character offset value for writing to request

    //Holds the request type
    int req_type = GET;

    //Stores the buffer size
    int content_size = -1;
    int message_size = 0;

    //Loading from the buffer into the 2D request array
    for(int i = 0; i < bytes_recv; i++){
        if(in_buff[i] == '\n'){
            req++;
            req_offset = 0;
            continue;
        }
        if(in_buff[i] != '\r'){
            request[req][req_offset] = in_buff[i];
            req_offset++;
        }
    }

    #ifdef TESTING
    //Printing out parsed request (as proof of proper parsing)
    printf("\nRequest:\n");
    for(int i = 0; i < req; i++){
        printf("%s\n", request[i]);
    }
    #endif

    //Interprets the path extension from the 'GET' operation
    char is_valid = 0;
    int path_len = strlen(*path);
    for(int i = 0; i < req; i++){
        //Handling a GET request
        if(request[i][0] == 'G' && request[i][1] == 'E' && request[i][2] == 'T'){
            req_type = GET;
            for(int j = 4; request[i][j] != ' '; j++){
                (*path)[path_len] = request[i][j];
                path_len++;
            }
            path[path_len] = '\0';
            path_len++;
            is_valid = 1;
            break;
        }
        //Handling a HEAD request
        else if(request[i][0] == 'H' && request[i][1] == 'E' && request[i][2] == 'A' && request[i][3] == 'D'){
            req_type = HEAD;
            for(int j = 5; request[i][j] != ' '; j++){
                (*path)[path_len] = request[i][j];
                path_len++;
            }
            path[path_len] = '\0';
            path_len++;
            is_valid = 1;
            break;
        }
    }

    //Get the current time in the HTTP Date format
    char* date = getTime();

    //Parsing the content type from the path
    char* content_type = parseExtension((const char *)*path, path_len);

    //If the request wasn't valid, send a code 400 and return with 2
    if(!is_valid){
        sprintf(out_buff, "HTTP/1.1 400 Bad Request\r\nDate: %s\r\nContent-Length: 0\r\nConnection: close\r\n\r\n", date);
        #ifdef TESTING
        printf("Bad request!\n");
        #endif
        send(sockfd, out_buff, strlen(out_buff), 0);
        return 1;
    }

    //Start building response depending on the request type
    //If it's a GET or HEAD request
    if(req_type == GET || req_type == HEAD){
        //Open a file for reading
        FILE *file = fopen(*path, "r");

        //Initializing the file contents
        char *file_contents;

        //If the file could be found
        if(file){
            //Get the content size and read the file into the out_buff
            fseek(file, 0, SEEK_END);
            content_size = ftell(file);
            fseek(file, 0, SEEK_SET);

            //Allocate memory for the file
            file_contents = malloc(content_size);
            memset(file_contents, 0, content_size);
            fread(file_contents, content_size, 1, file);
        }

        //Building the base messsage
        if(content_size != -1){
            sprintf(out_buff, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nDate: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", content_type, date, content_size);
        } else {
            //If the file couldn't be found, send a code 404 and return with 1
            #ifdef TESTING
            printf("Could not open the file at %s!\n", *path);
            #endif
            sprintf(out_buff, "HTTP/1.1 404 Not Found\r\nDate: %s\r\nContent-Length: 0\r\nConnection: close\r\n\r\n", date);
            send(sockfd, out_buff, strlen(out_buff), 0);
            return 2;
        }

        message_size = strlen(out_buff);
        
        //If it's a GET request
        if(req_type == GET){
            //Manually appending the content body to the message (can't sprintf due to null bytes in images)
            for(int i = 0; i < content_size; i++){
                out_buff[i+message_size] = file_contents[i];
            }
        }
    }

    //Send the response
    #ifdef TESTING
    printf("Response:\n%s\n\n", out_buff);
    #endif
    send(sockfd, out_buff, message_size + content_size, 0);

    //If everything is successful, return 0.
    return 0;
}


//Main function
int main(int argc, char **argv) {
    //If the proper arguments weren't passed in
    if(argc != 3){
        printf("Please use the following format:\n./server <path-to-web-files> <port>\n");
        return 1;
    }

    //Params required for various steps of server initialization
    struct sockaddr_storage outAddr;
    struct addrinfo hints;
    struct addrinfo *serv_info;
    int sockfd;
    int acceptfd;
    socklen_t addrSize;

    //Store params
    char *path = (char*)malloc(MAX_PATH_LEN);
    char *port = (char*)malloc(sizeof argv[2]);

    //Store buffers (on recieving socket)
    char buff[MAX_IN_BUFF_SIZE];

    //Clear buffer
    memset(buff, 0, MAX_IN_BUFF_SIZE);

    //Storing parameters
    strcpy(path, argv[1]);
    strcpy(port, argv[2]);

    printf("Listening on port %s (path to the files is %s):\n", port, path);

    //Initialize server info
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, port, &hints, &serv_info);

    //Initialize the socket
    sockfd = socket(serv_info->ai_family, serv_info->ai_socktype, serv_info->ai_protocol);

    //Bind the socket to a port
    bind(sockfd, serv_info->ai_addr, serv_info->ai_addrlen);

    //Free the server info
    freeaddrinfo(serv_info);

    //Listen on the socket
    listen(sockfd, 10);

    //Accept 
    addrSize = sizeof outAddr;

    acceptfd = accept(sockfd, (struct sockaddr*)&outAddr, &addrSize);
    int bytes_recv = 0;

    //Listening loop
    while(1){
        bytes_recv = recv(acceptfd, (void*)buff, MAX_IN_BUFF_SIZE, 0);
        if(bytes_recv){
            if(!fork()){
                //Child process
                if(handleRequest(acceptfd, buff, bytes_recv, &path)){
                    #ifdef TESTING
                    printf("Returned an error code!\n\n");
                    #endif
                } else {
                    #ifdef TESTING
                    printf("Returned a code 200!\n\n");
                    #endif
                }

                //Clear the buffer
                memset(buff, 0, MAX_IN_BUFF_SIZE);

                //Close the connection
                close(acceptfd);
                exit(0);
            }
            //Parent process
            acceptfd = accept(sockfd, (struct sockaddr*)&outAddr, &addrSize);
        }
    }

    //Freeing stuff (Won't ever reach this stuff yet)
    free(path);
    free(port);

    return 0;
}
