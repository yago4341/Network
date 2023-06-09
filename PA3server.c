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
//need for chaching to looks at time stamps 
#include <sys/stat.h>
#include<time.h>
//need for file descriptors
#include <fcntl.h>
//need for curl
#include <curl/curl.h>

#define BACKLOG 64	 // how many pending connections queue will hold

#define BUFSIZE 8192

#define POR 80

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


int sendall(int s, char *buf, int *len)
{
	printf("MADE IT TO SEND ALL!\n");
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;
	//printf("IN BUFFER:%s\n",buf);
    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}


//hash function
unsigned long hash(unsigned char *str)
{
	unsigned long hash = 5381;
	int c;
	while (c = *str++){
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
	return hash;
}


//cache function
void cacheHandler(char * fnam, int new_soc, int timeoutt)
{
	char filna[100];
	double diftime;
	char buf[BUFSIZE];
	bzero(filna, 100);
	strcat(filna, "./cache/");

	signed long filn = hash(fnam);
	char filam[20];
	//convert unsigned long to string
	sprintf(filam, "%lu", filn);
	strcat(filna, filam);

	/*used to help me with file time stamps https://c-for-dummies.com/blog/?p=3004 */
	struct stat filetime;
	stat(filna, &filetime);
	//printf("Last time the file was access: %s\n", ctime(&filetime.st_atime));
	/*used to get current time to compare https://www.studytonight.com/c/programs/misc/display-current-date-and-time#:~:text=C%20Program%20to%20Display%20the,hh%3Amm%3Ass%20yyyy.*/
	time_t tim;
	time(&tim);
	//printf("Current date and time: %s\n", ctime(&tim));

	diftime = difftime(tim,filetime.st_atime);
	printf("Time Difference: %lf\n", diftime);
	FILE * fpf;
	fpf = fopen(filna, "rb");
	if(fpf == NULL){
		printf("NOT CACHED\n");
	}
	else if(difftime < timeoutt){
		printf("MADE IT IN\n");
		fseek(fpf, 0L, SEEK_END);
		int fileSize = ftell(fpf);
		//resetting the location of the file
		fseek(fpf, 0L, SEEK_SET);
		char filSize[5];
		sprintf(filSize, "%d", fileSize);
		int total = 0;
		int red, n;
		while(total < fileSize){
			bzero(buf, BUFSIZE);
			red = fread(buf, sizeof(char),BUFSIZE, fpf);
			printf("How much read: %d\n", red);
			n = send(new_soc, buf, BUFSIZE, 0);
			total = total + red;
			if(n == red){
				printf("ALL IS WELL!");
			}
			printf("Total: %d\n", total);
			if(red == n){
				perror("Sending");
			}
		}
		fclose(fpf);
	}

}

void clientHandler(int new_fd,int timo){

    char buf[BUFSIZE];
    bzero(buf,BUFSIZE);
    /*ssize_t read(int fildes, void *buf, size_t nbyte);
    reads the message from client and puts it into the buf*/
    int length_recived;
    int r;
	
    while(r = recv(new_fd, buf, BUFSIZE, 0) > 0){
        //Checking if for the double retrun line
        length_recived += r;
        if(strncmp(buf + length_recived - 4, '\r\n\r\n', 0) == 0){
            break;
        }
    }

    //printf("from client %s \n", buf);
    char method[20];
    char address[300];
    char version[20];
    char word[6];
    char hosty[300];
    int ip;
	char *IPbuffer;
    bzero(hosty, 300);
    sscanf(buf, "%s %s %s %s %s", method, address, version, word, hosty);
	if((strcmp("GET", method)) != 0){
		if (send(new_fd, "HTTP/1.1 400 Bad Request\r\n Content-Length:15 \r\n\r\n400 Bad Request",61, 0) == -1){
			perror("send");
        }
        perror("Could not parse header");
	}

	//getting host from uri: char *strstr(const char *haystack, const char *needle)
	char * url = strstr(address, "//");
	//Start the string after the //
	char host2[300];
	if(url){
		url = url + 2;
		//see whats at the end of /
		char * end = strchr(url, "/");
		
		strncpy(host2, url, end - url - 1);
		printf("Host: %s\n", host2);
	}

	if(hosty == NULL){
		printf("Host name was not given to us!\n");
		bzero(hosty, 300);
		strcpy(hosty, host2);
	}

	//Getting Ip
	struct hostent *host_entry =  gethostbyname(hosty);
	if(host_entry == NULL){
		if(send(new_fd, "HTTP/1.1 404 Not Found\r\nContent-Length:13 \r\n\r\n404 Not Found",60, 0) == -1){
			perror("send");
            exit(0);
        }
	}
	IPbuffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
	// printf("Ip adddress: %s\n",IPbuffer);

	// if((strcmp(IPbuffer,hosty))== 0){
	// 	if(send(new_fd, "HTTP/1.1 404 Not Found\r\nContent-Length:13 \r\n\r\n404 Not Found",60, 0) == -1){
	// 		perror("send");
    //         exit(0);
    //     }
	// }
	
	//checking blocklist
	FILE *fp;
	fp = fopen("blocklist", "r");
	char line[500];
	while(fgets(line, sizeof(line), fp)!= NULL){
		if((strcmp(line, hosty)) == 0|| (strcmp(IPbuffer, line))== 0){
			printf("GOT CAUGHT!");
			if(send(new_fd, "HTTP/1.1 403 Forbidden\r\nContent-Length:13 \r\n\r\n403 Forbidden",60, 0) == -1){
			    perror("send");
            }
		}
		bzero(line, 500);
	}
	fclose(fp);

	cacheHandler(address , new_fd, timo);

	int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
 
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
 
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(IPbuffer);
    servaddr.sin_port = htons(POR);
 
    // connect the client socket to server socket
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))!= 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else{
        printf("connected to the server..\n");
	}
	write(sockfd, buf, BUFSIZE);

	//Creating file name
	char filnam[100];
	bzero(filnam, 100);
	strcat(filnam, "./cache/");

	unsigned long filn = hash(address);
	char fila[20];
	//convert unsigned long to string
	sprintf(fila, "%lu", filn);
	strcat(filnam, fila);

	//opening file and writing to it
	FILE *fptr; 
	fptr= fopen(filnam, "wb");

	bzero(buf,BUFSIZE);
	int n;
	int bytes_received;
	ssize_t bytes_sent;
	ssize_t sent= 0;
    while ((bytes_received = recv(sockfd, buf, BUFSIZE - 1, 0)) > 0) {
		fwrite(buf, 1, bytes_received,fptr);
        bytes_sent = 0;
        while (bytes_sent < bytes_received) {
            bytes_sent += send(new_fd, buf + bytes_sent, bytes_received - bytes_sent, 0);
        }

    }

	
	fclose(fptr);

	printf("MADE IT TO THE END\n");

	close(new_fd);
	close(sockfd);


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

    if(argc > 3 ){
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
	int timout = argv[2];
	
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
			//Set up timeout for socket
			
			close(sockfd); // child doesn't need the listener
            clientHandler(new_fd, timout);
            
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}
    return 0;
}
