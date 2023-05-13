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
//Needed from beejs guide
#include <errno.h>
#include <netdb.h>
//neede it to use a boolean for the while loop
#include <stdbool.h>

#define BUFSIZE 1024

int main(int argc, char **argv){
    int sockfd, portnum;
    char *portnm[3];
    char buf[BUFSIZE]; 
    /*Struct addrinfo
    -prep the socket address structures for subsequent use
    -used in host name lookups, and service name lookups
    -describes address information for use with TCP/IP */
    struct addrinfo hints, *servinfo, *p;
    //using cjs clients adder
    struct sockaddr_in client_addr; 
    struct hostent *hostp;
    char *hostaddrp;
    socklen_t addr_len;
    int ret;
    bool running = true;
    int n;
    //need it
    char error1[] = "Need to send filename as well!\n";
    char filename[20]= " ";
    int del = 0;
    char fromFile[1015];
    int whileloop = 0;

    //command line arguments and getting the port number
    if(argc != 2 ){
        fprintf(stderr, "usages: %s <port>\n", argv[0]);
        exit(1);
    }

    //Creating the socket
    //sockfd = socket(domain, type, protocol)
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    //returns -1 if there is an error sooooo
    if(sockfd < 0){
        error("ERROR opening socket!");
    }

    /*setsockopt: Handy debugging trick that lets us rerun the server immediately after we kill it; 
    otherwise we have to wait about 20 secs. Eliminates "ERROR on binding: Address already in use" error. 
    int setsockopt(int socket, int level, int option_name,
    const void *option_value, socklen_t option_length);*/
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    /*memset() is used to fill a block of memory with a particular value.
    void *memset(void *str(pointer to the blokc of memory), int c(value to be set), size_t n)*/
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    //gets your IP address and connects to itsefl. Fills local IP address
    hints.ai_flags = AI_PASSIVE;

    //GetaddrInfo() resolve the IP address of information
    if((ret = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return 1;
    }

    /*bind the socket to a port number and address
    int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    returns -1 if there is any errors so*/
    if(bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0){
        error("ERROR on binding!\n");

    }
    
    if(servinfo== NULL){
        error("ERROR: failed to bind socket!!");
        return 2;
    }

    /*freeaddrinfo free the linked-list 
    as in freeing up what getaddrinfo() alloscted for us */
    freeaddrinfo(servinfo);

    while(running){
        //void bzero( void *dst,ize_t n ); sets buf with zeros
        bzero(buf,BUFSIZE);
        addr_len = sizeof(client_addr);
        n = recvfrom(sockfd, buf, BUFSIZE, 0, 
         (struct sockaddr *)&client_addr, &addr_len);
        if(n<0){
            error("ERROR in recvfrom!");
        }

        hostp = gethostbyaddr((const char *)&client_addr.sin_addr.s_addr, sizeof(client_addr.sin_addr.s_addr), AF_INET);
        if (hostp == NULL){
            error("ERROR on gethostbyaddr");
        }
        hostaddrp = inet_ntoa(client_addr.sin_addr);
        if (hostaddrp == NULL){
            error("ERROR on inet_ntoa\n");
        }
        printf("server received datagram from %s (%s)\n", hostp->h_name, hostaddrp);
        printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);

        //int strncmp ( const char * str1, const char * str2, size_t num );
        bzero(filename, 20);

        if(strncmp("get", buf, 3) == 0){
            char cmd2[5];
            int red;
            int assgin1 = sscanf(buf, "%s %s",cmd2, filename);
            //getread
            if(assgin1 < 2 ){
                bzero(buf, BUFSIZE);
                strcat(buf,error1);
                if((n = sendto(sockfd,buf, strlen(buf), 0, &client_addr, addr_len)) <= 0)
                {
                    error("ERROR sendto did not work!");
                }
            }
            FILE *fil;
            fil = fopen(filename , "r" );
            if(fil == NULL){
                error("ERROR The file does not exist");
            }
            /*Getting the size of the file by using 
            https://www.geeksforgeeks.org/ftell-c-example/*/
            fseek(fil, 0L, SEEK_END);
            int fileSize = ftell(fil);
            //resetting the location of the file
            fseek(fil, 0L, SEEK_SET);
            bzero(buf, BUFSIZE);
            int fileSent = 0;
            int counter= 0;
            /*creating a mode but at the begining to see
            if the file size is too big to spilt it up
            used: https://www.geeksforgeeks.org/fread-function-in-c/ for using fread*/
            while(!feof(fil)){
                bzero(buf, BUFSIZE);
                buf[0] = 'w';
                buf[1] = ' ';
                char num[5];
                bzero(fromFile, 1015);
                /*size_t fread(void * buffer, size_t size, size_t count, FILE * stream)*/
                int red = fread(fromFile, sizeof( char ),sizeof(fromFile), fil );
                /*Need to convert an integer into a string so i will use sprintf
                https://www.educative.io/answers/how-to-use-the-sprintf-method-in-c*/
                sprintf(num, "%d", red);
                strcat(buf, num);
                strcat(buf, " ");
                strcat(buf, fromFile);
                if((n = sendto(sockfd,buf, red+4+strlen(num), 0, &client_addr, addr_len)) <= 0)
                {
                    error("ERROR sendto did not work!");
                }
                bzero(buf, BUFSIZE);
            }
            char fin[] ="d file was fully sent!";
            strcpy(buf, fin);
            if((n = sendto(sockfd,buf, strlen(buf), 0, &client_addr, addr_len)) <= 0)
            {
                error("ERROR sendto did not work!");
            }
            fclose(fil);
            /*ssize_t sendto( int sockfd, void *buff, size_t nbytes, int flags, 
            const struct sockaddr* to, socklen_t addrlen);*/
        }
        else if(strncmp(buf, "put", 3) == 0){
            char cmd[5];
            /*I am using sscanf to parse the buf into get the first two words
            https://www.educative.io/answers/how-to-read-data-using-sscanf-in-c
            it returns the number of assginments so if it returns 1 then that means 
            the file name was not sent*/
            int assgin = sscanf(buf, "%s %s",cmd, filename);
            if(assgin < 2 ){
                bzero(buf, BUFSIZE);
                strcat(buf,error1);
                if((n = sendto(sockfd,buf, strlen(buf), 0, &client_addr, addr_len)) <= 0)
                {
                    error("ERROR sendto did not work!");
                }
            }
            else{
                int filenmSz;
                filenmSz = sizeof(filename);
                filenmSz = filenmSz + 4;
                FILE *fp;
                fp = fopen (filename, "wb+");
                //fwrite(buf + filenmSz, sizeof(char), ,fp);
                n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
                if(n<0){
                    error("ERROR in recvfrom!");
                }
                while(whileloop == 0){
                    if(buf[0] == 'd'){
                        printf("%s \n", buf);
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
                        recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&client_addr, &addr_len);  
                    }
                    
                }
                char fin[] ="d file was fully sent!";
                strcpy(buf, fin);
                if((n = sendto(sockfd,buf, strlen(buf), 0, &client_addr, addr_len)) <= 0)
                {
                    error("ERROR sendto did not work!");
                }
                fclose(fp);
                whileloop = 0; 
            }
        }
        else if(strncmp(buf, "delete", 6)== 0){
            char cmd2[6];
            bzero(filename, 20);
            int assgin2 = sscanf(buf, "%s %s",cmd2, filename);
            if(assgin2 < 2){
                bzero(buf, BUFSIZE);
                strcat(buf,error1);
                if((n = sendto(sockfd,buf, strlen(buf), 0, &client_addr, addr_len)) <= 0)
                {
                    error("ERROR sendto did not work!");
                }
            }
            else{
                //int remove(const char *filename). returns 0 if it was successful
                if(remove(filename) != 0){
                    error("ERROR file was not deleted!");
                }
                else{
                    bzero(buf,BUFSIZE);
                    char deleteMes[]="File was successfully deleted!";
                    strcat(buf, deleteMes);
                   if((n = sendto(sockfd,buf, strlen(buf), 0, &client_addr, addr_len)) <= 0)
                    {
                        error("ERROR sendto did not work!");
                    }
                }
            }
        }
        else if(strncmp(buf, "ls", 2) == 0){
            /*FILE *popen(const char *command, const char *mode)
            Used: https://pubs.opengroup.org/onlinepubs/009696799/functions/popen.html*/
            char things[30];
            FILE *f;
            f = popen("ls *", "r");
            if(f == NULL){
                error("ERROR when running ls command it seem that the directory is empty!");
            }
            bzero(things, 30);
            bzero(buf,BUFSIZE);
            while (fgets(things,BUFSIZE, f) != NULL){
                strcat(buf,things);
                bzero(things,30);
            }
            printf("%s", buf);

            if((n = sendto(sockfd,buf, strlen(buf), 0, &client_addr, addr_len)) <= 0)
            {
                error("ERROR sendto did not work!");
            }
            fclose(f);

        }
        // else if(strncmp("exit", buf, 4) == 0){
        //     bzero(buf, BUFSIZE);
        //     char end[] = "You have selected exit. Thank you and goodbye!";
        //     strcpy(buf,end);
        //     n = sendto(sockfd,buf, strlen(buf), 0, (struct sockaddr *) &client_addr, addr_len);
        //     running = false;
        // }
        else{
            bzero(buf,BUFSIZE);
            char end_result[]= "The command given was not understood!";
            strcpy(buf, end_result);
            if((n = sendto(sockfd,buf, strlen(buf), 0, &client_addr, addr_len)) <= 0)
            {
                error("ERROR sendto did not work!");
            }
        }
    }

    close(sockfd);

}