#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#define PORT 58012

int fd, fdTCPserver, fdUDP, fdTCPclient;
struct hostent *hostptr, *hostaux;
struct sockaddr_in serveraddr, serveraddrc, serveraddrUDP, clientaddr, clientaddrUDP, servera;
struct in_addr ipv4addr;
int addrlen, clientlen;
char matrix [40][4];
FILE* counter;
FILE* f;
FILE* f1;

char** readFromUser();
void signalmethis(int c);
void sendToUser(char* message, int size);
char* connectToWS(char* message, int size, char* ipaddress, char* portnumber);

int main(int argc, char *argv[]) {
	int i, j, c, m, n, w, num, size, port = PORT, flag = 0;
	char* ipaddress = (char*) malloc(sizeof(char) * 14);
	char* portnumber = (char*) malloc(sizeof(char) * 6);
	char* PTC = (char*) malloc(sizeof(char) * 4);
	char* linePtr;
	char* filename = (char*) malloc(25 * sizeof(char));
	char* line = (char*) malloc(sizeof(char) * 25);
	char* linePtc = (char*) malloc(sizeof(char) * 25);
	char* charSize;
	char* reply;
	char* outputFile;
	char* buf = (char*) malloc(5 * sizeof(char));
	char** op;
	char** info;
	char* buffer = (char*) malloc(80 * sizeof(char));
	size_t len;

	if(argc == 3) {
        if(strcmp(argv[1], "-p") == 0)
        	/* Lê o argumento inicialmente do terminal com o número de Porto do CS*/
            port = atoi(argv[2]);
        else
            printf("Invalid arguments, defaults applied.");
    }
	else if(argc != 1)
		printf("Invalid arguments, defaults applied.");
	
	/*Limpa o fileprocessingtasks, para ficar vazio caso algum WS não se tenha se conseguido registrar com sucesso*/
	unlink("fileprocessingtasks.txt");
    
    /*Counter é um ficheiro com o número de pedidos realizados ao CS. É utilizado para guardar os ficheiros enviados pelos user*/
	counter = fopen("counter.txt", "w");
	fprintf(counter, "0");
	fclose(counter);

	/*Apanha o signal por causa dos sockets*/
    if (signal(SIGINT, signalmethis) == SIG_ERR) {
        printf("An error occurred while setting the termination signal handler.\n");
        exit(-1);
    }

	if(fork() == 0) {
		/*Separa os processos com os sockets que estarão a ouvir os users, e os WS's*/
		fdUDP = socket(AF_INET, SOCK_DGRAM, 0);
		if(fdUDP == -1) {
			printf("Failed to initialize socket.");
			exit(-1);
		}

		memset((void*)&serveraddrUDP, (int)'\0', sizeof(serveraddrUDP));
		serveraddrUDP.sin_family = AF_INET;
		serveraddrUDP.sin_addr.s_addr = htonl(INADDR_ANY);
		serveraddrUDP.sin_port = htons((u_short)port);

		bind(fdUDP, (struct sockaddr*)&serveraddrUDP, sizeof(serveraddrUDP));


		while(1) {
			addrlen = sizeof(clientaddrUDP);
			if(recvfrom(fdUDP, buffer, 80, 0, (struct sockaddr*)&clientaddrUDP, &addrlen) != -1) {
				/*Copia-se o IP e o porto*/
				for(i = 0; !isdigit(buffer[i]) && i < strlen(buffer); i++);
				/*Se o i for maior que o tamanho do buffer, significa que falta argumentos*/ 	
				if(i >= strlen(buffer))
					flag = 1;
				for(j = 0; buffer[i] != ' ' && i < strlen(buffer); i++, j++) 
					ipaddress[j] = buffer[i];
				if(i >= strlen(buffer))
					flag = 1;
				i++;
				for(j = 0; buffer[i] != '\n' && i < strlen(buffer); i++, j++)
					portnumber[j] = buffer[i];

				if(buffer[0] == 'R' && buffer[1] == 'E' && buffer[2] == 'G') { 
				/*Registar ficheiro UDP*/
					f = fopen("fileprocessingtasks.txt", "a+");
					if (flag == 1) 
						sendto(fdUDP, "RAK ERR\n", 8, 0, (struct sockaddr*)&clientaddrUDP, addrlen);
					else {
						if(isdigit(buffer[4]))
							sendto(fdUDP, "RAK ERR\n", 8, 0, (struct sockaddr*)&clientaddrUDP, addrlen);
						/*Coloca o PTC, o número de IP e o Porto no fileprocessingtasks.txt*/
						else {
							/*Fica a apontar para o primeiro PTC e vai copiar o PTC, o ipaddress e o Porto*/ 
							for(i = 4; !isdigit(buffer[i]); i++) {
								if(buffer[i] != ' ') 
									fprintf(f, "%c", buffer[i]);
								else 
									fprintf(f, " %s %s\n", ipaddress, portnumber);
							}
							fclose(f); 

							buffer += 4; /*Aponta para o próximo PTC*/
							printf("+ %s", buffer);
							sendto(fdUDP, "RAK OK\n", 7, 0, (struct sockaddr*)&clientaddrUDP, addrlen);
						}
					}
				}	
				else if(buffer[0] == 'U' && buffer[1] == 'N' && buffer[2] == 'R') {
					//Derregistar ficheiro UDP
					if(flag == 1)
						sendto(fdUDP, "UAK ERR\n", 8, 0, (struct sockaddr*)&clientaddrUDP, addrlen);
					else { 
						/*Fica a apontar para o IP*/
						buffer += 4;
						printf("- ");

						/*Cria-se um ficheiro auxiliar para aonde vamos copiar todos PTC's, IP's e o Porto's, excepto do WS que se pretende derregistar*/
        				f1 = fopen("fileprocessingtasks_aux.txt", "w");
        				f = fopen("fileprocessingtasks.txt", "r");
        				len = 25 * sizeof(char);
        				while ((getline(&line, &len, f)) != -1) {
          					linePtr = line + 4;
          					if (strcmp(buffer, linePtr) != 0)
          						fprintf(f1, "%s", line);
          					else {
          						PTC[0] = line[0];
								PTC[1] = line[1];
								PTC[2] = line[2];
                				PTC[3] = '\0';
          						printf("%s ", PTC);
          					}
        				}
        				//close both the files.
        				fclose(f);
        				fclose(f1);
        				//remove original file
        				remove("fileprocessingtasks.txt");
        				//rename the file copy.c to original name
        				rename("fileprocessingtasks_aux.txt", "fileprocessingtasks.txt");

        				f = fopen("fileprocessingtasks.txt", "a+");
        				printf("%s", buffer);
        				sendto(fdUDP, "UAK OK\n", 7, 0, (struct sockaddr*)&clientaddrUDP, addrlen);
					}
				}	
				else
					sendto(fdUDP, "ERR\n", 4, 0, (struct sockaddr*)&clientaddrUDP, addrlen);
			}
			else 
				printf("Error in UDP communication.\n");
			memset(buffer, '\0', 79);
		}
	}

	else {
		fd = socket(AF_INET, SOCK_STREAM, 0);
		if(fd == -1) {
			printf("Failed to initialize socket. Exiting.\n");
			exit(-1);
		}

		memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		serveraddr.sin_port = htons((u_short)port);

		if(bind(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) {
			printf("Failed to bind socket. Exiting.\n");
			exit(-1);
		}

		listen(fd,5);

		while(1) {
			clientlen = sizeof(clientaddr);
			fdTCPserver = accept(fd, (struct sockaddr*)&clientaddr, &clientlen);

			if(fork() == 0) {
				/*Começa o processo Filho, que sai do ciclo e percorre o código seguinte*/
				close(fd);
				break;
			}
			/*Processo Pai*/
			close(fdTCPserver);
		}
	
		op = readFromUser();
		if(op == NULL) {
			/*Código do list*/
			f = fopen("fileprocessingtasks.txt", "r");
			c = 0;
			len = sizeof(linePtc);
			hostaux = gethostbyaddr(&clientaddr.sin_addr, sizeof(clientaddr.sin_addr), AF_INET);
			printf("List request: %s %hu\n", hostaux->h_name, (int) clientaddr.sin_port);
			while(getline(&linePtc, &len, f) != -1) {
				PTC[0] = linePtc[0];
				PTC[1] = linePtc[1];
				PTC[2] = linePtc[2];
                PTC[3] = '\0';
				for (i = 0; i < c; i++) {
 					if(strcmp(PTC, matrix[i]) == 0) {
 						flag = 1;
 						break; /* Se houver um PTC igual não é preciso verificar toda a matrix*/
 					}
 				}

 				if (flag == 0) {
 					strcpy(matrix[c], PTC);
 					c++;
 				}
 				flag = 0;
        	}
			fclose(f);

        	reply = (char*) malloc(sizeof(char) * (c * 4 + 6));
        	strcpy(reply, "FPT ");
        	sprintf(buf, "%d", c);
			strcat(reply, buf);
        	for (i = 0; i < c; i++) {
				strcat(reply, " ");
        		strcat(reply, matrix[i]);
        		printf("%s ", matrix[i]);
        	}
        	printf("\n");
			strcat(reply, "\n");
			sendToUser(reply, sizeof(char) * (c * 4 + 6));
		}

		else {
			/*Código dos requests - Antes de mandar para o WS*/
			counter = fopen("counter.txt", "r");
			len = sizeof(buf);
			getline(&buf, &len, counter);
			c = atoi(buf) + 1;
			fclose(counter);
			counter = fopen("counter.txt", "w");
			fprintf(counter, "%d", c);
			fclose(counter);
			sprintf(filename, "input_files/%05d.txt", c);
			f = fopen(filename, "w");

			fprintf(f, "%s", op[2]);
			fclose(f);
			f = fopen("fileprocessingtasks.txt", "r");
			
			info = (char**) malloc(40 * sizeof(char*));
			i = 0;
			len = sizeof(linePtc);
			while(getline(&linePtc, &len, f) != -1) {
				PTC[0] = linePtc[0];
				PTC[1] = linePtc[1];
				PTC[2] = linePtc[2];
				PTC[3] = '\0'; 	
				if(strcmp(PTC, op[0]) == 0) {
					linePtc += 4;
					info[i] = (char*) malloc(sizeof(char) * strlen(linePtc));
					strcpy(info[i],linePtc);
					i++;
 				}
 				
        	}
        	
			fclose(f);
			f1 = fopen(filename, "r");
			len = atoi(op[1]);
			buf = (char*) malloc(len * sizeof(char));
			n = 0;
			/*Lê cada linha do ficheiro e manda para as WS's*/
			while(i != 0 && getline(&buf, &len, f1) != -1) {
				j = n % i;
				if(fork() == 0) {
					sprintf(filename, "%05d%03d.txt", c, n);
					reply = (char*) malloc(sizeof(char) * (strlen(buf) + 18 + strlen(filename)));
					
					strcpy(reply, "WRQ ");
					strcat(reply, op[0]);
					strcat(reply, " ");
					strcat(reply, filename);
					strcat(reply, " ");
					charSize = (char*) malloc(sizeof(char) * 10);
					sprintf(charSize, "%d", (int)strlen(buf));
					strcat(reply, charSize);
					strcat(reply, " ");
					strcat(reply, buf);
				
					w = 0;
					while(info[j][w] != ' ') {
						ipaddress[w] = info[j][w];
						w++;
					}
					ipaddress[w] = '\0';
					w++;
					c = 0;
					while(info[j][w] != '\n') {
						portnumber[c] = info[j][w];
						w++;
						c++;
					}
					portnumber[w] = '\0';
					printf("%s %s %s\n", op[0], ipaddress, portnumber);
					reply = connectToWS(reply, strlen(buf) + 18 + strlen(filename), ipaddress, portnumber);
					
					if(reply[4] == 'F' || reply[4] == 'R') {
						reply += 6;
						w = 0;
						while(reply[w] != ' ') 
							w++;
						reply += w + 1;

						buf = (char*) malloc(sizeof(char) * 21);
						strcpy(buf, "output_files/");
						strcat(buf, filename);
						f = fopen(buf, "w");
						fwrite(reply, strlen(reply), 1, f);
						fclose(f);
					}

					exit(0);
				}
				n++;
			}
			
			fclose(f1); /*Faz o close do ficheiro que se abriu antes de fazer o ciclo for e o fork*/
			num = 0;
			/*Espera que todos os WS's acabam de processar os respetivos fragmentos para concatenar*/
			while(num < n){
				wait(NULL);
				num++;
			}

			if (i == 0) {
				buf = (char*) malloc(sizeof(char) * 8);
				strcpy(buf, "REP EOF\n");
				outputFile = (char*) malloc(sizeof(char));
				strcpy(outputFile, "\0");
			}

			/*Processa respostas dos WS's*/
			else if(strcmp(op[0], "WCT") == 0) {
				m = 0;
				for(j = 0; j < n; j++) {
					sprintf(filename, "output_files/%05d%03d.txt", c, j);
					f = fopen(filename, "r");
					fseek(f, 0, SEEK_END); /*vai à procura do tamanho para poder criar um buffer só para o que está dentro do ficheiro, que neste caso vai ser o linePtr*/ 
            		size = ftell(f); 
            		fseek(f, 0, SEEK_SET);
            		linePtr = (char*) malloc(size * sizeof(char));
					fread(linePtr, sizeof(char), size, f);
					fclose(f);
					m += atoi(linePtr);
				}
				linePtr = (char*) malloc(10 * sizeof(char));
				charSize = (char*) malloc(10 * sizeof(char));
				sprintf(linePtr, "%d", m);
				w =  strlen(linePtr);
				sprintf(charSize, "%d", w);
				buf = (char*) malloc(sizeof(char) * (8 + strlen(charSize) + strlen(linePtr)));
				strcpy(buf, "REP R ");
				strcat(buf, charSize);
				strcat(buf, " ");
				strcat(buf, linePtr);
				strcat(buf, "\n");
				outputFile = (char*) malloc(strlen(linePtr) * sizeof(char));
				strcpy(outputFile, linePtr);
			}

			else if(strcmp(op[0], "FLW") == 0) {
				m = 0;
				for(j = 0; j < n; j++) {
					sprintf(filename, "output_files/%05d%03d.txt", c, j);
					f = fopen(filename, "r");
            		linePtr = (char*) malloc(50 * sizeof(char));
            		len = 50 * sizeof(char);
					getline(&linePtr, &len, f);
					fclose(f);
					if(strlen(linePtr) > m) {
						buf = linePtr;
						m = strlen(linePtr);
					}
				}
				linePtr = buf;
				charSize = (char*) malloc(10 * sizeof(char));
				sprintf(charSize, "%d", m);
				buf = (char*) malloc(sizeof(char) * (8 + strlen(charSize) + m));
				strcpy(buf, "REP R ");
				strcat(buf, charSize);
				strcat(buf, " ");
				strcat(buf, linePtr);
				strcat(buf, "\n");
				outputFile = (char*) malloc(strlen(linePtr) * sizeof(char));
				strcpy(outputFile, linePtr);
			}

			else {
				w = atoi(op[1]);
				reply = (char*) malloc(w * sizeof(char));
				strcpy(reply,"");
				for(j = 0; j < n; j++) {
					sprintf(filename, "output_files/%05d%03d.txt", c, j);
					f = fopen(filename, "r");
					fseek(f, 0, SEEK_END);  
            		size = ftell(f); 
            		fseek(f, 0, SEEK_SET);
            		linePtr = (char*) malloc(size * sizeof(char));
            		len = size * sizeof(char);
					getline(&linePtr, &len, f);
					fclose(f);
					strcat(reply, linePtr);
				}

				charSize = (char*) malloc(10 * sizeof(char));
				w = strlen(reply);
				sprintf(charSize, "%d", w);
				buf = (char*) malloc(sizeof(char) * (8 + strlen(charSize) + strlen(reply)));
				strcpy(buf, "REP F ");
				strcat(buf, charSize);
				strcat(buf, " ");
				strcat(buf, reply);
				outputFile = (char*) malloc(strlen(reply) * sizeof(char));
				strcpy(outputFile, reply);
			}
			sprintf(filename, "output_files/%05d.txt", c);
			f = fopen(filename, "w");
			fprintf(f, "%s", outputFile);
			fclose(f);

			/*Responde ao user*/
			sendToUser(buf, strlen(buf));
		}

		shutdown(fdTCPserver, SHUT_WR);
		close(fdTCPserver);
		exit(0);
	}
}

char** readFromUser() {
    char* ptr = (char*) malloc(4 * sizeof(char));
    char* ptr1 = ptr;
    char** reply = (char**) malloc(3 * sizeof(char*)); 
    int nleft = 4, nread;
    while(nleft > 0) {
        nread = read(fdTCPserver, ptr, 1);
        if(nread == -1)
            exit(1);
        else if(nread == 0)
            break;
                
        nleft -= nread;
        ptr += nread;
    }
	ptr = ptr1;

    if(strcmp(ptr1, "LST\n") == 0) {
    	return NULL;
    }

    else {
    	nleft = 4;
    	while(nleft > 0) {
        	nread = read(fdTCPserver, ptr, 1);
       	 	if(nread == -1)
            	exit(1);
        	else if(nread == 0)
            	break;
                
        	nleft -= nread;
        	ptr += nread;
   		}

   		ptr1[3] = '\0'; 
   		reply[0] = ptr1;

   		ptr = (char*) malloc(10 * sizeof(char));   		
   		ptr1 = ptr;
   		
   		nleft = 10;
    	while(nleft > 0) {
        	nread = read(fdTCPserver, ptr, 1);
       	 	if(nread == -1)
            	exit(1);
        	else if(nread == 0)
            	break;
                
        	nleft -= nread;
        	if(ptr[0] != ' ') {
				ptr += nread;
        	}
        	else
				break;
   		}
		reply[1] = ptr1;
		nleft = atoi(ptr1) * sizeof(char);
		
		ptr = (char*) malloc(sizeof(char) * (nleft + 1));
		ptr1 = ptr;
		
		while(nleft > 0) {
        	nread = read(fdTCPserver, ptr, nleft);
       	 	if(nread == -1)
            	exit(1);
        	else if(nread == 0)
            	break;
                
        	nleft -= nread;
        	ptr += nread;
   		}

		reply[2] = ptr1;
		reply[2][atoi(reply[1])] = '\0';
		
		return reply;
    }
}

void sendToUser(char* message, int size) {

	int nwritten = 0, nleft = size;
	while(nleft > 0) { 
        nwritten = write(fdTCPserver, message, nleft * sizeof(char));
        if(nwritten <= 0)                     
            exit(1);
        nleft -= nwritten;            
        message += nwritten;
    }
}



char* connectToWS(char* message, int size, char* ipaddress, char* portnumber) {
	int fptr, port = atoi(portnumber);
	int nread, nleft = size, nwritten;
	char* ptr = message;
    char* b = (char*) malloc(size);
	
    inet_aton(ipaddress, &servera.sin_addr);
  	if((hostptr = gethostbyaddr(&servera.sin_addr, sizeof(servera.sin_addr), AF_INET)) == NULL) {
        printf("Failed to locate host.\n");
        exit(-1);
    }

	memset((void*)&serveraddrc, (int)'\0', sizeof(serveraddrc));
    serveraddrc.sin_family = AF_INET;
    serveraddrc.sin_addr.s_addr = ((struct in_addr*) (hostptr->h_addr_list[0]))->s_addr;
    serveraddrc.sin_port = htons((u_short)port);

	fptr = socket(AF_INET, SOCK_STREAM, 0);
	
    if(connect(fptr, (struct sockaddr*)&serveraddrc, sizeof(serveraddrc)) == -1) {
        printf("Failed to connect to server.\n");
        exit(-1);
    }
    
    while(nleft > 0) { 
        nwritten = write(fptr, ptr, nleft * sizeof(char));
        if(nwritten <= 0)                     
            exit(1);
        nleft -= nwritten;            
        ptr += nwritten;
    }
   	
    ptr = b;
    nleft = size;
    
    while(nleft > 0) {
        nread = read(fptr, b, 1);
        if(nread == -1){
        	printf("Error\n");
            exit(1);
        }
        else if(nread == 0)
            break;
                
        nleft -= nread;
        b += nread;
    }

	close(fptr);
    return ptr;
}

void signalmethis(int c) {
	close(fd);
	close(fdUDP);
	exit(0);
}