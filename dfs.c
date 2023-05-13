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
//Need for filr descriptors
#include <fcntl.h>

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

void clientHandler(int new_fd){

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

    printf("From buffer: %s\n", buf);
        char cmd[5];
        char dir[500];
        char filena[500];
        sscanf(buf, "%s %s %s", cmd, dir, filena);

        if(strncmp("get", buf, 3) == 0){
            /*Using example from https://pubs.opengroup.org/onlinepubs/007904975/functions/fcntl.html*/
            char thing[200];
            char dire[100] = "ls ";
            strcat(dire, dir);
            printf("Director: %s\n", dire);
            FILE *fpp;
            fpp = popen(dire, "r");
            if(fpp == NULL){
                error("ERROR when running ls command it seem that the directory is empty!");
            }
            bzero(thing, 100);
            bzero(buf,BUFSIZE);
            while(fgets(thing,BUFSIZE, fpp) != NULL){
                int filleng = strlen(filena);
                if(strncmp(thing, filena, filleng) == 0){
                    printf("In thing: %s\n", thing);
                    strcat(buf, thing);
                }
                bzero(thing,100);
            }
            printf("%s", buf);
            write(new_fd, buf, strlen(buf));
            printf("Made it to the end of list\n");
            fclose(fpp);


            
        }
        else if(strncmp(buf, "put", 3) == 0){
            char buf3[BUFSIZE];
            int nu= read(new_fd, buf, BUFSIZE);
            strcat(dir,"/");
            strcat(dir, filena);
            printf("The direvctory is: %s\n", dir);
            char *tofil = strstr(buf, "\r\n\r\n");
            tofil = tofil + 4;
            printf("Just for text: %s\n", tofil);


            FILE * fpt;
            fpt = fopen(dir, "w");
            fwrite(tofil, 1, strlen(tofil),fpt);
            fclose(fpt);
            printf("Made it to the end of put\n");

        }
        else if(strncmp(buf, "list", 4) == 0){
            /*FILE *popen(const char *command, const char *mode)
            Used: https://pubs.opengroup.org/onlinepubs/009696799/functions/popen.html*/
            char things[100];
            char direc[100] = "ls ";
            strcat(direc, dir);
            printf("Director: %s\n", direc);
            FILE *f;
            f = popen(direc, "r");
            if(f == NULL){
                error("ERROR when running ls command it seem that the directory is empty!");
            }
            bzero(things, 100);
            bzero(buf,BUFSIZE);
            while(fgets(things,BUFSIZE, f) != NULL){
                strcat(buf,things);
                bzero(things,100);
            }
            printf("%s", buf);
            write(new_fd, buf, strlen(buf));
            printf("Made it to the edn of list\n");
            fclose(f);
        }
        else if(strncmp(buf, "move", 4)){
            strcat(dir,"/");
            strcat(dir, filena);
            printf("In dir: %s\n", dir);
            FILE *fp;
            fp = fopen(dir, "r");

            if(fp == NULL){
                char toserv[] = "NO1";
                write(new_fd, toserv, strlen(toserv));
                printf("MADE IT INO HERE!\n");
            }
            else{
                bzero(buf, BUFSIZE);
                char line1[500];
                while(fgets(line1, 500, fp) != NULL){
                    strcat(buf, line1);
                }
                write(new_fd, buf, strlen(buf));
            }
            fclose(fp);
        }
        else{
            // bzero(buf,BUFSIZE);
            // char end_result[]= "The command given was not understood!";
            // strcpy(buf, end_result);
            // if((n = sendto(sockfd,buf, strlen(buf), 0, &client_addr, addr_len)) <= 0)
            // {
            //     error("ERROR sendto did not work!");
            // }
        }

	close(new_fd);


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

    if(argc > 5 ){
        fprintf(stderr, "usages: %s <port>\n", argv[0]);
        exit(1);
    }

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[2],  &hints, &servinfo)) != 0) {
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
			//Set up timeout for socket
			close(sockfd); // child doesn't need the listener
            clientHandler(new_fd);
            
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}
    return 0;
}