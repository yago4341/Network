#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 64	 // how many pending connections queue will hold

#define BUFSIZE 8080

/* Used: https://beej.us/guide/bgnet/pdf/bgnet_usl_c_1.pdf page 38 for connection*/

void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void clientHandler(int new_fd){
    int sent = 0;
    char buf[BUFSIZE];
    bzero(buf,BUFSIZE);
    /*ssize_t recv( int sockfd, void *buf, size_t len, int flags );
    reads the message from client and puts it into the buf*/
    char bufcheck[4];
    bzero(bufcheck, 4);
    //recv(new_fd, buf, BUFSIZE, 0);
    int length_recived;
    int r;
    while(r = recv(new_fd, buf, BUFSIZE, 0) > 0){
        //Checking if for the double retrun line
        length_recived += r;
        if(strncmp(buf + length_recived - 4, '\r\n\r\n', 0) == 0){
            break;
        }
    }
    printf("from client %s \n", buf);
    //things needed from what the client sent
    char method[5];
    char URI[50];
    char version[10];
    char fil[100] = "./www";
    bzero(version, 10);
    sscanf(buf,"%s %s %s", method,URI, version);
    //Checking the meathod
    printf("Method: %s\n", method);
    //if after checking the method and it still dont pass it was a bad request
    if(strcmp(method, "HEAD") == 0 || strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0 || strcmp(method, "DELETE") == 0 || strcmp(method, "CONNECT") == 0){
        if (send(new_fd, "HTTP/1.1 405 Method Not Allowed\r\nConent-Length:22 \r\n\r\n405 Method Not Allowed",72, 0) == -1){
			perror("send");
        }
        perror("Could not parse header");
    }
    else if(strcmp(method, "GET") != 0 ){
        if (send(new_fd, "HTTP/1.1 400 Bad Request\r\n Content-Length:15 \r\n\r\n400 Bad Request",61, 0) == -1){
			perror("send");
        }
        perror("Could not parse header");
    }
    else{
        printf("Working\n");
    }
    //Version sometimes makes my style.css ask up
    printf("Version: %s\n", version);
if((strncmp(version,"HTTP/1.1", 8) != 0 && strncmp(version, "HTTP/1.0",8))!= 0){
        if (send(new_fd, "HTTP/1.1 505 HTTP Version Not Supported\r\nConent-Length:30 \r\n\r\n505 HTTP Version Not Supported",88, 0) == -1){
            perror("send");
            exit(1);
        }
        perror("Wrong Http version");
    }
    if((strcmp(URI, "/index.htm"))== 0){
        bzero(URI, 50);
        strcpy(URI, "/index.html");
    }
    printf("URI: %s\n", URI);

    const char* luri = URI;
    /*char *strchr(const char *str, int c) searches for the first occurrence of the character c*/
    const char* ur = strrchr(luri, '/');
    if(strcmp(ur, "/")== 0){
        bzero(URI, 50);
        strcat(URI, "/index.html");
    }

    strcat(fil, URI);
    FILE *fp;
    fp = fopen(fil, "rb");
    if (fp == NULL) {
        //errno 13 is permission denied!
        if(errno == 13){
            if(send(new_fd, "HTTP/1.1 403 Forbidden\r\nContent-Length:13 \r\n\r\n403 Forbidden",60, 0) == -1){
			    perror("send");
            }
        }
        else{
            if(send(new_fd, "HTTP/1.1 404 Not Found\r\nContent-Length:13 \r\n\r\n404 Not Found",60, 0) == -1){
			    perror("send");
                exit(0);
            }
        }
        //perror("File was not found");
        exit(0);
    }
    bzero(buf,BUFSIZE);
    //for some reason it gets sent to here 
    printf("from client: %s\n", buf);
    printf("directory: %s\n", fil);


    //getting the header done
    bzero(buf,BUFSIZE);
    strcat(buf, "HTTP/1.1 200 OK\r\nContent-Type:");

    char extension[28];
    bzero(extension, 28);
    const char* furi = URI;
    /*char *strchr(const char *str, int c) searches for the first occurrence of the character c*/
    const char* exten = strrchr(furi, '.');
    if(strcmp(exten, ".html")== 0 ){
        strcpy(extension,"text/html\r\n");
    }else if(strcmp(exten, ".txt")== 0 ){
        strcpy(extension,"text/plain\r\n");
    }else if(strcmp(exten, ".png")== 0 ){
        strcpy(extension,"image/png\r\n");
    }else if(strcmp(exten, ".gif")== 0 ){
        strcpy(extension,"image/gif\r\n");
    }else if(strcmp(exten, ".jpg")== 0 ){
        strcpy(extension,"image/jpg\r\n");
    }else if(strcmp(exten, ".ico")== 0 ){
        strcpy(extension,"image/x-icon\r\n");
    }else if(strcmp(exten, ".css")== 0 ){
        strcpy(extension,"text/css\r\n");
    }else{
        strcpy(extension,"application/javascript\r\n");
    }
    printf("extension: %s",extension);
    strcat(buf, extension);
    strcat(buf, "Content-Length:");
    /*Getting the size of the file by using 
    https://www.geeksforgeeks.org/ftell-c-example/*/
    fseek(fp, 0L, SEEK_END);
    int fileSize = ftell(fp);
    //resetting the location of the file
    fseek(fp, 0L, SEEK_SET);
    char filSize[5];
    sprintf(filSize, "%d", fileSize);
    printf("File Size: %d\n", fileSize);
    strcat(buf, filSize);
    strcat(buf, "\r\n\r\n");
    printf("header: %s\n", buf);

    int headerSize = strlen(buf);
    printf("header size: %d\n",headerSize );
    /*using beejs guide for send all https://beej.us/guide/bgnet/html/#sendman */
    //send(new_fd,"working ", 10,0);

    int howMuchSent = send(new_fd, buf,headerSize, 0);
    if(howMuchSent == -1){
		perror("send");
    }
    int total = 0;
    int red, n;
    while(total < fileSize){
        bzero(buf, BUFSIZE);
        red = fread(buf, sizeof(char),BUFSIZE, fp);
        printf("How much read: %d\n", red);
        n = send(new_fd, buf, BUFSIZE, 0);
        total = total + red;
        if(n == red){
            printf("ALL IS WELL!");
        }
        printf("Total: %d\n", total);
        if(red == n){
            perror("Sending");
        }
    }

    fclose(fp);

}

int main(int argc, char **argv)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
    char buf[BUFSIZE]; 

    if(argc != 2 ){
        fprintf(stderr, "usages: %s <port>\n", argv[0]);
        exit(1);
    }

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1],  &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

    /*int listen(int socket, int backlog);*
    This shows the readinesds of the client to accept the connection*/
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;

        /*int accept(int sockfd, struct sockaddr *restrict addr,socklen_t *restrict addrlen); 
        this accepts the connection on a socket*/
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
            clientHandler(new_fd);
            
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}
    return 0;
}