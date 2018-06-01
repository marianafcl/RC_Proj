#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#define PORT 58012
 
int fd;
struct hostent *hostptr;
struct sockaddr_in serveraddr;

char* writeAndRead(char* messageSend, int receive, int send);
 
int main(int argc, char *argv[]) {
    int i, j, port, n, size, nleft, nread, nwritten, c;
    char servername[128], buf[6];
    char* symbol = (char*) malloc(sizeof(char));
    char* filebuffer;
    char* reply;
    char* ptr;
    char* PTC = (char*) malloc(3 * sizeof(char));
    char* buffer = (char*) malloc(128 * sizeof(char));
    char* command = (char*) malloc(128 * sizeof(char));
    char* request = (char*) malloc(4 * sizeof(char));
    char* filename = (char*) malloc( 50 * sizeof(char));
    char* filenamenew = (char*) malloc( 55 * sizeof(char));
    FILE* f;
    FILE* fp;

    /* Initial argument treatment */
 
    port = PORT;
    gethostname(servername, sizeof(servername));
 
    if(argc == 5) {
        if(strcmp(argv[1], "-n") == 0) {
            strcpy(servername, argv[2]);
            if(strcmp(argv[3], "-p") == 0)
                port = atoi(argv[4]);
            else
                printf("Invalid arguments, defaults applied.\n"); 
        }
        else if(strcmp(argv[1], "-p") == 0) {
            port = atoi(argv[2]);
            if(strcmp(argv[3], "-n") == 0)
                strcpy(servername, argv[4]);
            else
                printf("Invalid arguments, defaults applied.\n");          
        }
        else
            printf("Invalid arguments, defaults applied.\n");
    }

    else if(argc == 3) {
    	if(strcmp(argv[1], "-n") == 0)
            strcpy(servername, argv[2]);
        else if(strcmp(argv[1], "-p") == 0)
            port = atoi(argv[2]);
        else
            printf("Invalid arguments, defaults applied.\n");
    }

    else if(argc != 1)
      printf("Invalid arguments, defaults applied.\n");

    /* Locate CS host */
     
    if((hostptr = gethostbyname(servername))== NULL) {
        printf("Failed to locate host.\n");
        exit(-1);
    }
 
    memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = ((struct in_addr*) (hostptr->h_addr_list[0]))->s_addr;
    serveraddr.sin_port = htons((u_short)port);
 
    while(1) {
        scanf("%s", command);
   
        if(strcmp(command, "list") == 0) {

            /* Establish TCP server and await connection */

            fd = socket(AF_INET, SOCK_STREAM, 0);
            if(fd == -1) {
                printf("Failed to initialize socket.\n");
                exit(-1);
            }
    
            if(connect(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) {
                printf("Failed to connect to server.\n");
                exit(-1);
            }   
        	
            buffer = writeAndRead("LST\n", 128 * sizeof(char), 4);

            /* Handle message */

            if (isdigit(buffer[4])) {
                n = atoi(&buffer[4]); 
			    j = 1;
		
                for(i = 6; j <= n; i+=4, j++) {
                    
                    PTC[0] = buffer[i];
		   			PTC[1] = buffer[i+1];
		    		PTC[2] = buffer[i+2];
		    
                    if(strcmp(PTC, "WCT") == 0)
                        printf("%d - %s - word count\n", j, PTC);
                    else if(strcmp(PTC, "UPP") == 0)
                        printf("%d - %s - convert to upper case\n", j, PTC);
                    else if(strcmp(PTC, "LOW") == 0)
                        printf("%d - %s - convert to lower case\n", j, PTC);
                    else if(strcmp(PTC, "FLW") == 0)
                        printf("%d - %s - find longest word\n", j, PTC);
                    else
                        printf("%d - %s\n", j, PTC);
                }
            }
            else
                printf("%s\n", buffer);
            close(fd);
        }

        else if(strcmp(command, "exit") == 0) {
            close(fd);
            exit(0);
        }
        
        else if(strcmp(command, "request") == 0) {
	        scanf("%s", request);
	        scanf("%s", filename);

            /* Establish TCP server and await connection */
            
            fd = socket(AF_INET, SOCK_STREAM, 0);

            if(fd == -1) {
                printf("Failed to initialize socket.\n");
                exit(-1);
            }
    
            if(connect(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) {
              printf("Failed to connect to server.\n");
               exit(-1);
            }

            /* Handle request */

            f = fopen(filename, "r");
            if(f == NULL) {
            	printf("Error: file doesn't exist.\n");
            	continue;
            }
            
            fseek(f, 0, SEEK_END); 
            size = ftell(f); 
            fseek(f, 0, SEEK_SET); 

            if((strcmp(request, "UPP\0")) == 0 || strcmp(request, "LOW\0") == 0)
                printf("%d Bytes to transmit\n", size);
 
            filebuffer = (char*) malloc(size + 20 * sizeof(char));
 
            strcpy(filebuffer, "REQ ");
            strcat(filebuffer, request);
	        strcat(filebuffer, " ");
            sprintf(buf,"%d", size);
            strcat(filebuffer, buf);
            strcat(filebuffer, " ");
 
            if(f != NULL) {
	            symbol[0] = fgetc(f);
                while(symbol[0] != EOF) {
                    strcat(filebuffer, symbol);
		            symbol[0] = fgetc(f);
	            }
	        }

            fclose(f);
            filebuffer = writeAndRead(filebuffer, size + 20, size + 20);
	       
            if(strcmp("FPT EOF\n", filebuffer) == 0 || strcmp("FPT ERR\n", filebuffer) == 0 || strcmp("REP ERR\n", filebuffer) == 0 || strcmp("REP EOF\n", filebuffer) == 0)
                printf("%s", filebuffer);
            else {
                i = 6, n = 0;
                ptr = (char*) malloc(sizeof(char) * 10);
                while(filebuffer[i] != ' ') {
                    ptr[n] = filebuffer[i];
                    i++;
                    n++;
                }
                n = atoi(ptr);  
                
                i++;
            
                reply = (char*)malloc(n * sizeof(char));
                c = i;

                for(j = 0; j < n; i++, j++) {
                    reply[j] = filebuffer[i];
                }
                reply[j] = '\0';
                
                if(filebuffer[4] == 'R') {
                    if(strcmp(request, "WCT") == 0)
                        printf("Number of words: %s\n", reply);
                    else if(strcmp(request, "FLW") == 0)
                        printf("Longest word: %s", reply);
                    else
                        printf("%s", reply);
                }
 
                else if(filebuffer[4] == 'F') {
                    memset(filenamenew,'\0',n);
                    for(j = 0; filename[j] != '.'; j++)
                        filenamenew[j] = filename[j];
                    filenamenew[j] = '_';
                    strcat(filenamenew, request);
                    strcat(filenamenew, ".txt");
                    fp = fopen(filenamenew,"w");
                    
                    fwrite(reply, sizeof(char), n, fp);
                    fclose(fp);

                    printf("received file %s\n%d Bytes\n", filenamenew, n);
                }
                close(fd);     
            }
        }
        else
            printf("Invalid command.\n");

    }
}

/* Function that communicates with the Central Server */

char* writeAndRead(char* messageSend, int receive, int send) {
    char* ptr = messageSend;
    char* ptr1 = (char*) malloc(receive); 
    char* b = (char*) malloc(receive);
    int nread, nleft = send, nwritten;

    while(nleft > 0) { 
        nwritten = write(fd, ptr, nleft * sizeof(char));
        if(nwritten <= 0)                     
            exit(1);
        nleft -= nwritten;            
        ptr += nwritten;
    }
    ptr1 = b;
    nleft = receive;
    while(nleft > 0) {
        nread = read(fd, b, nleft);
        if(nread == -1)
            exit(1);
        else if(nread == 0)
            break;
                
        nleft -= nread;
        b += nread;
    }
    b[0] = '\0';
    return ptr1;
 }
