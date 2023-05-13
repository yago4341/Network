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
//need for chaching to looks at time stamps 
#include <sys/stat.h>
#include<time.h>
//need for sendfile
#include <fcntl.h>


#define MAX_LINE_LENGTH 500
#define BUFSIZE 8192

void error(char *msg) {
    perror(msg);
    exit(0);
}

unsigned long hash(unsigned char *str)
{
	unsigned long hash = 5381;
	int c;
	while (c = *str++){
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
	return hash;
}

int con(char *portnum, char *IP){

    int port = atoi(portnum);
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
 
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        //printf("socket creation failed...\n");
        exit(0);
    }
    else
        //printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;

    servaddr.sin_addr.s_addr = inet_addr(IP);

    servaddr.sin_port = htons(port);

 
    // connect the client socket to server socket
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))!= 0) {
        //implemnt something for that they all work
        //sockfd = 0; 
        return 0;
    }
    else{
        //printf("connected to the server..\n");
        return sockfd;
	}
    //printf("Made it to the end!\n");
    return sockfd;
}

void header(int serv, int part,char *cmd, char *direc, char *filen){
    char par[5];
    sprintf(par, "%d", part);
    long tiim = time(NULL);
    char sendfill[300];
    strcpy(sendfill, filen);
    strcat(sendfill,"+PART");
    strcat(sendfill, par);
    strcat(sendfill, "+");
    char one[1000];
    sprintf(one, "%ld", tiim);
    strcat(sendfill,one);
    strcat(sendfill, " ");
    char buf1[BUFSIZE];
    bzero(buf1, BUFSIZE);
    strcat(buf1, cmd);
    strcat(buf1, " ");
    strcat(buf1, direc);
    strcat(buf1, " ");
    strcat(buf1, sendfill);
    strcat(buf1, "\r\n\r\n");

    //printf("Header: %s\n", buf1);
    int size = strlen(buf1);

    write(serv, buf1, strlen(buf1));
}

void getheader(int serv, char *direc, char *filename){
    char toser[500];
    strcpy(toser, "get ");
    strcat(toser, direc);
    strcat(toser, " ");
    strcat(toser, filename);

    write(serv, toser, strlen(toser));
}

void getheaders(int serv, char *direc, char *filename){
    char toserv[500];
    strcpy(toserv, "Move ");
    strcat(toserv, direc);
    strcat(toserv, " ");
    strcat(toserv, filename);

    write(serv, toserv, strlen(toserv));
}

int main( int argc, char ** argv ){
    char dont[100];
    char care[100];
    char last[100];
    char dir1[100];
    char dir2[100];
    char dir3[100];
    char dir4[100];
    char buf[BUFSIZE];
    char buf1[BUFSIZE];
    char buf2[BUFSIZE];
    char buf3[BUFSIZE];
    char buf4[BUFSIZE];
    char cmd[5];
    char filename[100];
    int serv1, serv2,serv3, serv4;
    char SEVRINFO[4][3][BUFSIZE];

    //Getting port numbers and host from the text 
    FILE *fptr;
    fptr = fopen("dfc.conf", "r");
    char line[500];
    int x=0;
	while(fgets(line, sizeof(line), fptr)!= NULL){
        sscanf(line, "%s %s %s",dont,care,last);
        char * port = strstr(line,":");
        port = port + 1 ;
        port[strcspn(port, "\n")] = 0;
        size_t leng = strlen(last);
        last[leng-6]= '\0';
        char *hos = last;

        //To get each servers info
        if(x == 0){
            strcpy(SEVRINFO[0][2], care); 
            strcpy(SEVRINFO[0][0], port);
            strcpy(SEVRINFO[0][1], hos);
            serv1 = con(SEVRINFO[0][0],SEVRINFO[0][1]);
            if(serv1 != 0){
                close(serv1);
            }

        }
        else if(x == 1){
            strcpy(SEVRINFO[1][2], care);
            strcpy(SEVRINFO[1][0], port);
            strcpy(SEVRINFO[1][1], hos);
            serv1 = con(SEVRINFO[1][0],SEVRINFO[1][1]);
            if(serv1 != 0){
                close(serv1);
            }
        }
        else if(x == 2){
            strcpy(SEVRINFO[2][2], care);
            strcpy(SEVRINFO[2][0], port);
            strcpy(SEVRINFO[2][1], hos);
            serv1 = con(SEVRINFO[2][0],SEVRINFO[2][1]);
            if(serv1 != 0){
                close(serv1);
            }
        }
        else if(x == 3){
            strcpy(SEVRINFO[3][2], care);
            strcpy(SEVRINFO[3][0], port);
            strcpy(SEVRINFO[3][1], hos);
            serv1 = con(SEVRINFO[3][0],SEVRINFO[3][1]);
            if(serv1 != 0){
                close(serv1);
            }
        }
        x += 1;

    }

    //hash and mod
    for(int i = 2; i < argc; i++){
        int mod;
        strcpy(cmd, argv[1]);
        if(argv[2]!= NULL){
            strcpy(filename, argv[i]);
            // printf("in cmd: %s/n", cmd);
            // printf("Inn filename: %s\n", filename);F
            signed long filenam = hash(filename);
            mod= filenam % 4;

        }

        


        if(strcmp(cmd, "put") == 0){
            for(int i =0; i<4; i++){
                int serv7 = con(SEVRINFO[i][0],SEVRINFO[i][1]);
                if (serv7 == 0){
                    strcpy(cmd, "move");
                    printf("%s put failed.\n", filename);
                }else{
                    close(serv7);
                }
            }
        }


        /*Header syle COMMAND DIR FILENAME+PART#+TIMESTAMP*/
        if(strcmp(cmd, "list") == 0){
            //need to see if I have all four parts
            int uno = 0;
            int dos = 0;
            int tres = 0;
            int cuatro = 0;
            //Ask server what files they got
            //file names are on chuck so if one piece is missing send incomplete
            char tos[100];
            char frombuf[BUFSIZE];
            char tobuf[BUFSIZE];
            char toclient[BUFSIZE];
            int n = 0;
            bzero(buf,BUFSIZE);
            strcmp(buf, " ");
            for(int i =0; i < 4; i++){
                int serv8 = con(SEVRINFO[i][0],SEVRINFO[i][1]);
                if(serv8 != 0){
                    strcpy(tos, "list ");
                    strcat(tos, SEVRINFO[i][2]);
                    strcat(tos, " ");
                    strcat(tos, filename);
                    write(serv8, tos, BUFSIZE);
                    bzero(tos, BUFSIZE);

                    n = recv(serv8, frombuf, BUFSIZE, 0);
                    strcat(tobuf, frombuf);
                    close(serv8);
                }
            }

            char FILENAMES[100][500];


            //getting the file names and adding it to an array
            char tobuf2[BUFSIZE];
            strcpy(tobuf2, tobuf);
            
            char *filse = strtok(tobuf, "\n");
            int x = 0;
            int j = 0;
            while(filse != NULL){
                j = 0;
                char ccomp= '+';
                char *rest = strchr(filse, ccomp);
                if (rest != NULL){
                    int rest1 = strlen(rest);
                    int rest2 = strlen(filse);
                    if(rest1 <= rest2){
                        filse[rest2- rest1] = '\0';
                    }
                }
                if(x==0){
                    strcpy(FILENAMES[0], filse);
                    x= x + 1;
                }

                for(int i = 0; i < x; i++){
                    if(strcmp(FILENAMES[i], filse) ==0){
                        j = 1;
                        
                    }
                }
                if(j != 1){
                    strcpy(FILENAMES[x], filse);
                    x= x+1;
                    j=0;
                }
                filse = strtok(NULL, "\n");

            }

            FILE *temp;
            temp = fopen("temp.txt", "w+");
            if(temp == NULL){
                printf("Creating file eror\n");
            }
            fprintf(temp, tobuf2);
            fclose(temp);

            FILE *sortedfile;
            sortedfile = popen("sort temp.txt | uniq |  grep -v -e '^$'", "r");
            if(sortedfile == NULL){
                printf("Error opening file\n");
            }
            char line2[500];
        
            char line[MAX_LINE_LENGTH];
            char currentFileName[MAX_LINE_LENGTH] = "";
            int partCounter = 0;


            while (fgets(line2, sizeof(line2), sortedfile)!= NULL) {
                char *fileName = strtok(line2, "+");
                //printf("IN filename: %s\n", fileName);
                char *partName = strtok(NULL, "+");
                //printf("In partName:%s\n", partName);
                strtok(NULL, "+"); 

    \
                if (strcmp(currentFileName, fileName) != 0) {
                    if (currentFileName[0] != '\0') {  // If there is a previous file
                        if(partCounter != 4){
                            printf("%s incomplete\n", currentFileName);
                        }
                        else{
                            printf("%s\n", currentFileName);
                        }
                    }
                    strncpy(currentFileName, fileName, MAX_LINE_LENGTH);
                    partCounter = 1; 
                } else {
                    partCounter++;  
                }
            }

            // Process the final file
            if(partCounter != 4){
                printf("%s incomplete\n", currentFileName);
            }
            else{
                printf("%s\n", currentFileName);
            }

            pclose(sortedfile);

        }
        else if(strcmp(cmd, "put")== 0){

            FILE *fp;
            fp = fopen(filename, "r");
            if(fp == NULL){
                error("ERROR when running ls command it seem that the directory is empty!");
            }
            //Getting file size so I can see how big to have the file sizes.
            fseek(fp, 0L, SEEK_END);
            int fileSize = ftell(fp);
            //resetting the location of the file
            fseek(fp, 0L, SEEK_SET);
            fclose(fp);

            int chunck_siz = fileSize / 4;

            if(mod == 0){
                /*DFS1-(1,2), DFS2-(2,3) DFS3-(3,4) DFS4-(4,1)*/

                off_t start1 = 0;
                int end1 = chunck_siz;

                int fptt;
                /* USed https://github.com/pijewski/sendfile-example/blob/master/sendfile.c
                to to help me with send file*/
                fptt = open(filename, O_RDONLY);
                if(fptt == -1){
                    perror("Send File");
                }


                for(int i=0; i<4; i++){
                    int serv6 = con(SEVRINFO[i][0],SEVRINFO[i][1]);
                    header(serv6,i+1, cmd , SEVRINFO[i][2], filename);
                    sendfile(serv6,fptt, &start1, chunck_siz);
                    close(serv6);
                    
                }
                close(fptt);

                off_t start7 = 0;
                int fpttp;
                /* USed https://github.com/pijewski/sendfile-example/blob/master/sendfile.c
                to to help me with send file*/
                fpttp = open(filename, O_RDONLY);
                if(fpttp == -1){
                    perror("Send File");
                }
                for(int i=0; i<4; i++){
                    if(i == 0){
                        int serv6 = con(SEVRINFO[3][0],SEVRINFO[3][1]);
                        header(serv6,1, cmd , SEVRINFO[3][2], filename);
                        sendfile(serv6,fpttp, &start7, chunck_siz);
                        close(serv6);
                    }else{
                        int serv6 = con(SEVRINFO[i-1][0],SEVRINFO[i-1][1]);
                        header(serv6,i+1, cmd , SEVRINFO[i-1][2], filename);
                        sendfile(serv6,fpttp, &start7, chunck_siz);
                        close(serv6);
                    }
                    
                }
                close(fpttp);
                

            }
            else if(mod == 1){
                /*DFS1-(4,1), DFS2-(1,2) DFS3-(2,3)	DFS4-(3,4)*/
                off_t start2 = 0;
                
                int end1 = chunck_siz;

                int fptt;
                /* USed https://github.com/pijewski/sendfile-example/blob/master/sendfile.c
                to to help me with send file*/
                fptt = open(filename, O_RDONLY);
                if(fptt == -1){
                    perror("Send File");
                }

                for(int i=0; i<4; i++){
                    if(i == 0){
                        int serv6 = con(SEVRINFO[1][0],SEVRINFO[1][1]);
                        header(serv6,1, cmd , SEVRINFO[1][2], filename);
                        sendfile(serv6,fptt, &start2, chunck_siz);
                        close(serv6);
                    }else if(i == 3){
                        int serv6 = con(SEVRINFO[0][0],SEVRINFO[0][1]);
                        header(serv6,4, cmd , SEVRINFO[0][2], filename);
                        sendfile(serv6,fptt, &start2, chunck_siz);
                        close(serv6);
                    }
                    else{
                        int serv6 = con(SEVRINFO[i+1][0],SEVRINFO[i+1][1]);
                        header(serv6,i+1, cmd , SEVRINFO[i+1][2], filename);
                        sendfile(serv6,fptt, &start2, chunck_siz);
                        close(serv6);
                    }
                    
                } 
                close(fptt);

                off_t start6 = 0;
                int fptt1;
                fptt1 = open(filename, O_RDONLY);
                if(fptt1 == -1){
                    perror("Send File");
                }

                for(int i=0; i<4; i++){
                    int serv6 = con(SEVRINFO[i][0],SEVRINFO[i][1]);
                    header(serv6,i+1, cmd , SEVRINFO[i][2], filename);
                    sendfile(serv6,fptt1, &start6, chunck_siz);
                    close(serv6);
                    
                }
                close(fptt1);

            }
            else if(mod == 2){
                /*DFS1-(3,4), DFS2-(4,1) DFS3-(1,2)	DFS4-(2,3)*/
                off_t start4 = 0;
                int fptpt;
                fptpt = open(filename, O_RDONLY);
                if(fptpt == -1){
                    perror("Send File");
                }

                for(int i=0; i<4; i++){
                    if(i == 2){
                        int serv0 = con(SEVRINFO[0][0],SEVRINFO[0][1]);
                        header(serv0,3, cmd , SEVRINFO[0][2], filename);
                        sendfile(serv0,fptpt, &start4, chunck_siz);
                        close(serv0);
                    }else if(i==3){
                        int serv0 = con(SEVRINFO[1][0],SEVRINFO[1][1]);
                        header(serv0,4, cmd , SEVRINFO[1][2], filename);
                        sendfile(serv0,fptpt, &start4, chunck_siz);
                        close(serv0);
                    }
                    else{
                        int serv0 = con(SEVRINFO[i+2][0],SEVRINFO[i+2][1]);
                        header(serv0,i+1, cmd , SEVRINFO[i+2][2], filename);
                        sendfile(serv0,fptpt, &start4, chunck_siz);
                        close(serv0);
                    }
                    
                }
                close(fptpt);

                off_t start3 = 0;
                int fptp;
                fptp = open(filename, O_RDONLY);
                if(fptp == -1){
                    perror("Send File");
                }

                for(int i=0; i<4; i++){
                    if(i == 3){
                        int serv0 = con(SEVRINFO[0][0],SEVRINFO[0][1]);
                        header(serv0,4, cmd , SEVRINFO[0][2], filename);
                        sendfile(serv0,fptp, &start3, chunck_siz);
                        close(serv0);
                    }else{
                        int serv0 = con(SEVRINFO[i+1][0],SEVRINFO[i+1][1]);
                        header(serv0,i+1, cmd , SEVRINFO[i+1][2], filename);
                        sendfile(serv0,fptp, &start3, chunck_siz);
                        close(serv0);
                    }
                    
                }
                close(fptp);

            }
            else if (mod == 3){
                /*DFS1-(2,3), DFS2-(3,4) DFS3-(4,1) DFS4-(1,2)*/
                off_t start= 0;
                int end = chunck_siz;

                int fppt;
                /* USed https://github.com/pijewski/sendfile-example/blob/master/sendfile.c
                to to help me with send file*/
                fppt = open(filename, O_RDONLY);
                if(fppt == -1){
                    perror("Send File");
                }


                
                for(int i=0; i<4; i++){
                    if(i == 0){
                        int serv0 = con(SEVRINFO[3][0],SEVRINFO[3][1]);
                        header(serv0,1, cmd , SEVRINFO[3][2], filename);
                        sendfile(serv0,fppt, &start, chunck_siz);
                        close(serv0);
                    }else{
                        int serv0 = con(SEVRINFO[i-1][0],SEVRINFO[i-1][1]);
                        header(serv0,i+1, cmd , SEVRINFO[i-1][2], filename);
                        sendfile(serv0,fppt, &start, chunck_siz);
                        close(serv0);
                    }
                    
                }
                close(fppt);

                off_t start5 =0;
                int fpptt;
                fpptt = open(filename, O_RDONLY);
                if(fpptt == -1){
                    perror("Send File");
                }
                for(int i=0; i<4; i++){
                    if(i == 0){
                        int serv0 = con(SEVRINFO[2][0],SEVRINFO[2][1]);
                        header(serv0,1, cmd , SEVRINFO[2][2], filename);
                        sendfile(serv0,fpptt, &start5, chunck_siz);
                        close(serv0);
                    }else if(i==1){
                        int serv0 = con(SEVRINFO[3][0],SEVRINFO[3][1]);
                        header(serv0,2, cmd , SEVRINFO[3][2], filename);
                        sendfile(serv0,fpptt, &start5, chunck_siz);
                        close(serv0);
                    }
                    else{
                        int serv0 = con(SEVRINFO[i-2][0],SEVRINFO[i-2][1]);
                        header(serv0,i+1, cmd , SEVRINFO[i-2][2], filename);
                        sendfile(serv0,fpptt, &start5, chunck_siz);
                        close(serv0);
                    }
                    
                }

                close(fpptt);

            }
        }
        else if(strcmp(cmd, "get")== 0){

            bzero(buf, BUFSIZE);
            for(int i = 0; i < 4; i ++){
                int ser1 = con(SEVRINFO[i][0],SEVRINFO[i][1]);
                if(ser1 != 0){
                    getheader(ser1, SEVRINFO[i][2], filename);
                    recv(ser1, buf1, BUFSIZE, 0);
                    strcat(buf, buf1);
                    close(ser1);
                }
            }
            char buf0[BUFSIZE];
            bzero(buf0, BUFSIZE);
            strcpy(buf0, buf);

            //get chunkc back from server and piece it back together

            char *fils;
            int ones = 0;
            int two = 0;
            int three = 0;
            int four = 0;

            fils = strtok(buf, "\n");

            while(fils != NULL){
                char compar = '+';
                char *partss = strchr(fils, compar);
                partss[7] = '\0';
                // printf("The file name is now: %s\n", partss);

                if(strcmp(partss, "+PART1+") == 0){
                    ones = 1;
                }else if(strcmp(partss, "+PART2+") == 0){
                    two = 1;
                }else if (strcmp(partss, "+PART3+") == 0){
                    three = 1;
                }else if (strcmp(partss, "+PART4+") == 0){
                    four = 1;
                }
                fils = strtok(NULL, "\n");
            }


            char *filese;
            filese = strtok(buf0, "\n");
            char TIMESTAMPS[100][500];
            int o=0;
            int p = 0;

            while(filese != NULL){
                char comp[] = "+1";
                char *check = strstr(filese,comp);
                check = check +1;
                if(o == 0){
                    strcpy(TIMESTAMPS[0], check);
                    o = o+1;
                    p = 1;
                }
                if(p !=1){
                    strcpy(TIMESTAMPS[0], check);
                    o = o+1;
                    p = 0;
                }
                filese = strtok(NULL, "\n");
            }

            int timestam = atoi(TIMESTAMPS[0]);

            for(int t = 0; t < (o-1);t++){
                int num1 = atoi(TIMESTAMPS[t]);
                int num2 = atoi(TIMESTAMPS[t+1]);
                if(num1 < num2){
                    timestam = num2;
                }
                else{
                    timestam = num1;
                }

            }

            char reqfile[200];
            strcpy(reqfile, filename);
            strcat(reqfile, "+PART");
            int part1 = 0;
            int part2 = 0;
            int part3 = 0;
            int part4 = 0;
            if(ones == 1 && two == 1 && three == 1 && four == 1){
                FILE *filptr;
                filptr = fopen(filename, "w+");


                char compfile[BUFSIZE];
                for(int i = 0; i< 4; i++){
                    bzero(reqfile, 200);
                    strcpy(reqfile, filename);
                    strcat(reqfile, "+PART");
                    char num3[1];
                    sprintf(num3, "%d", i+1);
                    strcat(reqfile,num3);
                    strcat(reqfile, "+");
                    char reqtime[100];
                    sprintf(reqtime, "%d", timestam);
                    strcat(reqfile, reqtime);


                    if(i == 0){
                        for(int i=0; i < 4;i++){
                            if(part1 == 0){
                                int ser1 = con(SEVRINFO[i][0],SEVRINFO[i][1]);
                                if(ser1 != 0){
                                    getheaders(ser1, SEVRINFO[i][2], reqfile);
                                    bzero(buf1, BUFSIZE);
                                    recv(ser1, buf1, BUFSIZE, 0);
                                    if(strncmp(buf1, "NO1",3)!=0){
                                        fprintf(filptr,buf1);
                                        part1 = 1;
                                    }
                                    close(ser1);
                                }

                            }
                        }
                        
                    }
                    if(i == 1){
                        for(int i=0; i < 4;i++){
                            if(part2 == 0){
                                int ser1 = con(SEVRINFO[i][0],SEVRINFO[i][1]);
                                if(ser1 != 0){
                                    getheaders(ser1, SEVRINFO[i][2], reqfile);
                                    bzero(buf1, BUFSIZE);
                                    recv(ser1, buf1, BUFSIZE, 0);
                                    if(strncmp(buf1, "NO1",3)!=0){
                                        fprintf(filptr,buf1);
                                        part2 = 1;
                                    }
                                    close(ser1);
                                }

                            }
                        }
                    }

                    if(i == 2){
                            
                        for(int i=0; i < 4;i++){
                            if(part3 == 0){
                                int ser1 = con(SEVRINFO[i][0],SEVRINFO[i][1]);
                                if(ser1 != 0){
                                    getheaders(ser1, SEVRINFO[i][2], reqfile);
                                    bzero(buf1, BUFSIZE);
                                    recv(ser1, buf1, BUFSIZE, 0);
                                    if(strncmp(buf1, "NO1",3)!=0){
                                        fprintf(filptr,buf1);
                                        part3 = 1;
                                    }
                                    close(ser1);
                                }

                            }
                        }
                    }

                    if(i == 3){
                        for(int i=0; i < 4;i++){
                            if(part4 == 0){
                                int ser1 = con(SEVRINFO[i][0],SEVRINFO[i][1]);
                                if(ser1 != 0){
                                    getheaders(ser1, SEVRINFO[i][2], reqfile);
                                    bzero(buf1, BUFSIZE);
                                    recv(ser1, buf1, BUFSIZE, 0);
                                    if(strncmp(buf1, "NO1",3)!=0){
                                        fprintf(filptr,buf1);
                                        part4 = 1;
                                    }
                                    close(ser1);
                                }

                            }
                        }
                    }
                        
                }
                fclose(filptr);

            }else{
                char filemes[200];
                strcpy(filemes, filename);
                strcat(filemes, " is incomplete.");
                printf("%s\n", filemes);

            }
        }
        else{
            //send back that command is not recongized
        }
    }
    return 0;

}
