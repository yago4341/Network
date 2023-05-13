#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
//Header File needes to do socket programing
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//stuff from beejs
#include <errno.h>


#define BUFSIZE 1024
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main( int argc, char ** argv ){
  int sockfd,n, portnum;
  int servlen;
  struct sockaddr_in servinfo;
  struct hostent *server;
  char *hostname;
  char buf[BUFSIZE];
  int whileloop = 0;
  char filename[20];
    char fromFile[1015];


  if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portnum = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        error("ERROR opening socket!");
    }

    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

while(1){
    bzero((char *) &servinfo, sizeof(servinfo));
    servinfo.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&servinfo.sin_addr.s_addr, server->h_length);
    servinfo.sin_port = htons(portnum);

    //add zeros to the buffer
    bzero(buf, BUFSIZE);
    printf("Please enter msg: ");
    if(fgets(buf, BUFSIZE, stdin) == NULL){
        error("bad fgets\n");
    }
    //exit
    if(strncmp(buf, "exit", 4) == 0){
        printf("You have pressed exit. Thank you and good bye\n");
        close(sockfd);
        exit(0);
    }
    if(strncmp(buf, "get", 3) == 0){
        char cmd[3];
        bzero(filename, 20);
        int assgin = sscanf(buf, "%s %s",cmd, filename);
    }
    
    //send the message to the server 
    servlen = sizeof(servinfo);
    n = sendto(sockfd, buf, strlen(buf), 0, &servinfo, servlen);
    if(n < 0){
        error("ERROR in sendto!");
    }

    if(strncmp(buf, "put", 3) == 0){
        char cmd2[3];
        bzero(filename, 20);
        int assgin2 = sscanf(buf, "%s %s",cmd2, filename);
        FILE *fil;
        fil= fopen(filename , "r" );
        if(fil == NULL){
            error("ERROR The file does not exist");
        }
        while(!feof(fil)){
            bzero(buf, BUFSIZE);
            buf[0] = 'w';
            buf[1] = ' ';
            char num[5];
            bzero(fromFile, 1015);
            int red = fread(fromFile, sizeof( char ),sizeof(fromFile), fil);
            sprintf(num, "%d", red);
            strcat(buf, num);
            strcat(buf, " ");
            strcat(buf, fromFile);
            if((n = sendto(sockfd,buf, red+4+strlen(num), 0, &servinfo, servlen)) <= 0)
            {
                error("ERROR sendto did not work!");
            }
            bzero(buf, BUFSIZE);
        }
        char fin[] ="d file was fully sent!";
        strcpy(buf, fin);
        if((n = sendto(sockfd,buf, strlen(buf), 0, &servinfo, servlen)) <= 0)
        {
            error("ERROR sendto did not work!");
        }
        fclose(fil);
    }

    bzero(buf,BUFSIZE);
    //Getting the message back 
    n = recvfrom(sockfd, buf, BUFSIZE,0 , &servinfo, &servlen);
    if(n < 0){
        error("ERROR in recvfrom client!");
    }
    //cheking mode bit to see if i need to keep listens or need to print something
    if(buf[0] == 'w'){
        FILE *fp;
        fp = fopen(filename, "wb+");
        int counter = 0;
        while(whileloop == 0){
            if(buf[0] == 'd'){
                printf("file have been fully sent\n");
                whileloop = 1;
                bzero(buf,BUFSIZE);
            }else{
                char cmd2[5];
                char sizen[5];
                int numpass;
                sscanf(buf, "%s %s",cmd2, sizen);
                numpass = atoi(sizen);
                //printf("%d\n", numpass);
                int nu=  sizeof(numpass);
                int pars = 3 + nu;

                //size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
                //printf("\nThe 'w' is a modebit use, ignore the modebit and here is the file: %s", buf);
                fwrite(buf + pars, sizeof(char), numpass, fp);
                counter += 1;
                printf("File is still sending!\n" );
                n = recvfrom(sockfd, buf, BUFSIZE,0 , &servinfo, &servlen);   
            }
            
        }
        fclose(fp);
        whileloop = 0;
    }
    else{
        printf("From the server: %s \n", buf);
    }
}
    close(sockfd);
}
