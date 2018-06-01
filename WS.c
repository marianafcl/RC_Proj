#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <arpa/inet.h>
#define CSPORT 58012
#define WSPORT 59000

int fd, fdTCPserver, fdUDP;
struct hostent *hostptr, *hostptrc;
struct sockaddr_in serveraddr, serveraddrUDP, clientaddr;
struct timeval tv;
int addrlen, clientlen;
char buffer[80], servername[128];
int UDPport, TCPport;


char** readFromCentral(); 
void signalmethis(int c);
void writeToCentral(char* message, int size);

int main(int argc, char *argv[]) {
	int i, c, counter, size, count;
    char* msg;
    char* buffer;
    char* reply;
    char* buf;
    char* aux;
    char* aux1;
    char* hostname;
    char* outputFile;
    char* filename = (char*) malloc(sizeof(char*) * 25);
    char** ops = (char**) malloc(sizeof(char*) * 99);
    char** task;
    FILE* f;
    
    /* Initial argument treatment */

    UDPport = CSPORT;
    TCPport = WSPORT;
    gethostname(servername, sizeof(servername));

	if(argc == 1 || argc > 103) {
        printf("Invalid arguments. Exiting.\n");
        exit(-1);
    }
    else {
    	counter = 0;
        for(i = 1; i < argc; i++) {
        	if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "-e") == 0)
        		break;
            if(i > 99) {
                printf("Invalid arguments. Exiting.\n");
                exit(-1);
            }
            else if(strcmp(argv[i], "WCT") == 0 || strcmp(argv[i], "FLW") == 0 || strcmp(argv[i], "LOW") == 0 || strcmp(argv[i], "UPP") == 0)
                ops[counter++] = argv[i];
            else {
                printf("Invalid arguments. Exiting.\n");
                exit(-1);
            }
        }

        while(i < argc) {
        	if(strcmp(argv[i], "-p") == 0)
            	TCPport = atoi(argv[i + 1]);

        	else if(strcmp(argv[i], "-n") == 0)
            	strcpy(servername, argv[i + 1]);
      
        	else if(strcmp(argv[i], "-e") == 0)
            	UDPport = atoi(argv[i + 1]);

        	else {
            	printf("Invalid arguments. Exiting.\n");
            	exit(-1);
        	}

        	i += 2;
        }
    }

    /* Establishing UDP connection for registry */

    fdUDP = socket(AF_INET, SOCK_DGRAM, 0);

    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (setsockopt(fdUDP, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        printf("Error setting up socket timer. Exiting.\n");\
        exit(-1);
    }

    hostptr = gethostbyname(servername); 

    memset((void*)&serveraddrUDP, (int) '\0', sizeof(serveraddrUDP));
    serveraddrUDP.sin_family = AF_INET;
    serveraddrUDP.sin_addr.s_addr = ((struct in_addr *) (hostptr->h_addr_list[0]))->s_addr;
    serveraddrUDP.sin_port = htons((u_short)UDPport);

    addrlen = sizeof(serveraddrUDP);

    /* Exchange messages */

    msg = (char*) malloc(sizeof(char) * (23 + 4 * count));
    buffer = (char*) malloc(sizeof(char) * 8);

    strcpy(msg, "REG ");
    for(i = 0; i < counter; i++) {
        strcat(msg, ops[i]);
        strcat(msg, " ");
    }

    hostname = (char*) malloc(sizeof(char) * 50);
    gethostname(hostname, 50);
    hostptrc = gethostbyname(hostname);
    strcat(msg, inet_ntoa(*((struct in_addr*) hostptrc->h_addr_list[0])));

    strcat(msg, " ");
    aux = (char*) malloc(sizeof(char) * 6);
    sprintf(aux, "%d\n", TCPport);
    strcat(msg, aux);

    i = 0;
    while(strcmp(buffer, "RAK OK\n") != 0 && i < 3) {
        sendto(fdUDP, msg, strlen(msg), 0, (struct sockaddr*) &serveraddrUDP, addrlen);
        recvfrom(fdUDP, buffer, 8, 0, (struct sockaddr*) &serveraddrUDP, &addrlen);
        buffer[7] = '\0';
        i++;
    }

    if(strcmp(buffer, "RAK NOK\n") == 0) {
        printf("Failed to register with central server. Exiting.\n");
        exit(-1);
    }
    else if(strcmp(buffer, "RAK OK\n") == 0)
        printf("Successfully registered with central server.\n");
    else if(strcmp(buffer, "RAK ERR\n") == 0) {
        printf("Number of parameters incorrect. Exiting\n");
        exit(-1);
    }
    else {
        printf("ERROR: %s\n", buffer);
        exit(-1);
    }

    close(fdUDP);

    /* Signal handling for termination of working server */

    if (signal(SIGINT, signalmethis) == SIG_ERR) {
        printf("An error occurred while setting the termination signal handler.\n");
        exit(-1);
    }

    /* Establish TCP server and await connection */

    fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1) {
		printf("Failed to initialize socket.");
		exit(-1);
	}

	memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((u_short)TCPport);

	
	if(bind(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) {
	   printf("Failed to bind socket. Exiting.\n");
	   exit(-1);
	}

	listen(fd,5);

    /* Deal with request */
    
    while(1) {
        
        clientlen = sizeof(clientaddr);
        fdTCPserver = accept(fd, (struct sockaddr*)&clientaddr, &clientlen);

        if(signal(SIGINT, SIG_IGN) == SIG_ERR) {
        	printf("An error occurred while setting the termination signal handler.\n");
        	exit(-1);
        }
       
        task = readFromCentral();
        strcpy(filename, "input_files/");
        strcat(filename, task[3]);
        f = fopen(filename, "w");
        fprintf(f, "%s", task[2]);
        fclose(f);
        printf("%s %s\n", task[0], task[3]);
        if(strcmp(task[0], "WCT") == 0) {
            count = 0;
            for(i = 0; i < atoi(task[1]); i++) {
                if((task[2][i] == ' ' || task[2][i] == '\n' || task[2][i] == '\t') && (task[2][i + 1] != ' ' && task[2][i + 1] != '\n' && task[2][i + 1] != '\t'))
                    count++;
               
            }
            if((task[2][i] == ' ' || task[2][i] == '\n' || task[2][i] == '\t' || task[2][i] == '\0') && (task[2][i - 1] != ' '&& task[2][i - 1] != '\n' && task[2][i - 1] != '\t'))
                count++;
            
            aux = (char*) malloc(sizeof(char) * 10);
            sprintf(aux, "%d", count);
            
            outputFile = (char*) malloc(sizeof(char) * strlen(aux));
            strcpy(outputFile, aux);
            
            reply = (char*) malloc(sizeof(char) * (15 + strlen(aux)));
            sprintf(reply, "REP R %d %s\n", (int)strlen(aux), aux);

            printf("Number of words: %d\n", count);
        }

        else if(strcmp(task[0], "FLW") == 0) {
            count = 0;
            buf = "";
            aux = (char*) malloc(sizeof(char) * 50);
            aux1 = aux;
			for(i = 0; i < atoi(task[1]); i++) {
                if(task[2][i] == ' '){
                    aux[0] = '\0';
                    if(strlen(aux1) > count) {
                        buf = aux1;
                        count = strlen(aux1);
                    }
                    aux = (char*) malloc(sizeof(char) * 50);
                    aux1 = aux;
                }
                else {
                    aux[0] = task[2][i];
                    aux++;
                }
			}

            outputFile = (char*) malloc(sizeof(char) * strlen(buf));
            strcpy(outputFile, buf);

            reply = (char*) malloc(sizeof(char) * (15 + strlen(buf)));
            sprintf(reply, "REP R %d %s\n", (int)strlen(buf), buf);

            printf("Longest word: %s\n", buf);
        }

        else if(strcmp(task[0], "UPP") == 0) {
            for(i = 0; i < atoi(task[1]); i++)
                task[2][i] = toupper(task[2][i]);

            outputFile = (char*) malloc(sizeof(char) * strlen(task[2]));
            strcpy(outputFile, task[2]);
            
            reply = (char*) malloc(sizeof(char) * (7 + strlen(task[1]) + strlen(task[2])));
            sprintf(reply, "REP F %s %s\n", task[1], task[2]);

            printf("%s Bytes Received\n%s (%s Bytes)\n", task[1], task[3], task[1]);
        }

        else if(strcmp(task[0], "LOW") == 0) {
            for(i = 0; i < atoi(task[1]); i++)
                task[2][i] = tolower(task[2][i]);
            
            outputFile = (char*) malloc(sizeof(char) * strlen(task[2]));
            strcpy(outputFile, task[2]);

            reply = (char*) malloc(sizeof(char) * (7 + strlen(task[1]) + strlen(task[2])));
            sprintf(reply, "REP F %s %s\n", task[1], task[2]);

            printf("%s Bytes Received\n %s (%s Bytes)\n", task[1], task[3], task[1]);
        }

        else {
            size = 8;
            reply = (char*) malloc(sizeof(char) * size);
            strcpy(reply, "REP EOF\n");
        }

        f = fopen(filename, "w");
        fprintf(f, "%s", outputFile);
        fclose(f);
        size = strlen(reply);
        writeToCentral(reply, size);
        shutdown(fdTCPserver, SHUT_WR);
        close(fdTCPserver);

        signal(SIGINT, signalmethis);
    }
}

char** readFromCentral() {
    char* ptr = (char*) malloc(4 * sizeof(char));
    char* ptr1 = ptr;
    char** reply = (char**) malloc(4 * sizeof(char*)); 
    int nleft = 4, nread;
    while(nleft > 0) {
        nread = read(fdTCPserver, ptr, 1);
        if(nread == -1) {
            printf("Error while reading from Central Server. Exiting.\n");
            exit(-1);
        }
        else if(nread == 0)
            break;
                
        nleft -= nread;
        ptr += nread;
    }
    
	ptr = ptr1;
    nleft = 4;
    while(nleft > 0) {
       	nread = read(fdTCPserver, ptr, 1);
      	if(nread == -1) {
            printf("Error while reading from Central Server. Exiting.\n");
            exit(-1);
        }
        else if(nread == 0)
            break;
                
        nleft -= nread;
        ptr += nread;
   	}
   	
   	ptr1[3] = '\0'; 
   	reply[0] = ptr1;
    
   	ptr = (char*) malloc(13 * sizeof(char));   		
   	ptr1 = ptr;
   	

   	nleft = 13;
    while(nleft > 0) {
        nread = read(fdTCPserver, ptr, 1);
       	if(nread == -1) {
            printf("Error while reading from Central Server. Exiting.\n");
            exit(-1);
        }
        else if(nread == 0)
            break;
                
        nleft -= nread;
        if(ptr[0] != ' ') { 
			ptr += nread;
        }
        else
			break;
   	}
    
    ptr[0] = '\0';
    reply[3] = ptr1;

    ptr = (char*) malloc(10 * sizeof(char));   		
   	ptr1 = ptr;
   	
   	nleft = 10;
    while(nleft > 0) {
        nread = read(fdTCPserver, ptr, 1);
       	if(nread == -1) {
            printf("Error while reading from Central Server. Exiting.\n");
            exit(-1);
        }
        else if(nread == 0)
            break;
                
        nleft -= nread;
        if(ptr[0] != ' ') {
			ptr += nread;
        }
        else
			break;
   	}
   	
   	ptr[0] = '\0'; 
	reply[1] = ptr1;
	nleft = atoi(ptr1);
		
	ptr = (char*) malloc(sizeof(char) * (nleft + 1));
	ptr1 = ptr;
		
	while(nleft > 0) {
        nread = read(fdTCPserver, ptr, nleft);
       	if(nread == -1) {
            printf("Error while reading from Central Server. Exiting.\n");
            exit(1);
        }
        else if(nread == 0)
            break;
                
        nleft -= nread;
        ptr += nread;
   	}
		
	reply[2] = ptr1;
	reply[2][atoi(reply[1])] = '\0';
	
	return reply;
}

void signalmethis(int c) {
    int i;
    char* msg;
    char* buffer;
    char* aux;
    /* Establishing UDP connection for deregistry */

    fdUDP = socket(AF_INET, SOCK_DGRAM, 0);

    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (setsockopt(fdUDP, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }

    hostptr = gethostbyname(servername); 

    memset((void*)&serveraddrUDP, (int) '\0', sizeof(serveraddrUDP));
    serveraddrUDP.sin_family = AF_INET;
    serveraddrUDP.sin_addr.s_addr = ((struct in_addr *) (hostptr->h_addr_list[0]))->s_addr;
    serveraddrUDP.sin_port = htons((u_short)CSPORT);

    addrlen = sizeof(serveraddrUDP);

    /* Exchange messages */

    msg = (char*) malloc(sizeof(char) * 23);
    buffer = (char*) malloc(sizeof(char) * 8);

    strcpy(msg, "UNR ");
    strcat(msg, inet_ntoa(*((struct in_addr*) hostptrc->h_addr_list[0])));
    strcat(msg, " ");
    aux = (char*) malloc(sizeof(char) * 6);
    sprintf(aux, "%d\n", TCPport);
    strcat(msg, aux);
    

    i = 0;
    while(strcmp(buffer, "UAK OK\n") != 0 && i < 3) {
        sendto(fdUDP, msg, strlen(msg), 0, (struct sockaddr*) &serveraddrUDP, addrlen);
        recvfrom(fdUDP, buffer, 8, 0, (struct sockaddr*) &serveraddrUDP, &addrlen);
        i++;
        buffer[7] = '\0';
    }

    if(strcmp(buffer, "UAK NOK\n") == 0) {
        printf("Failed to deregister with central server. Exiting.\n");
        exit(-1);
    }
    else if(strcmp(buffer, "UAK OK\n") == 0)
        printf("Successfully deregistered with central server.\n");
    else if(strcmp(buffer, "UAK ERR\n") == 0){
        printf("Number of parameters incorrect. Exiting\n");
        exit(-1);
    }
    else {
        printf("ERROR: %s\n", buffer);
        exit(-1);
    }

    close(fdUDP);
    exit(0);
}

void writeToCentral(char* message, int size) {
	int nwritten = 0, nleft = size;
	while(nleft > 0) { 
        nwritten = write(fdTCPserver, message, nleft * sizeof(char));
        if(nwritten <= 0) {
            printf("Error writing to Central Server.\n");
            exit(-1);
        }
        nleft -= nwritten;            
        message += nwritten;
    }
}
