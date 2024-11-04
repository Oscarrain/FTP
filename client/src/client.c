#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <memory.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <stdlib.h>
#define BUFF_SIZE 8192
#define MAX_LISTEN 10
#define DEFAULT_PORT 21
#define PASV_MODE 1
#define PORT_MODE 2

// create socket
int create_socket(int port,struct sockaddr_in* addr, int mode, char* ip)
{
	int listenfd;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	int option = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	memset(addr, 0, sizeof(*addr));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);

	if (mode == PASV_MODE) {
		addr->sin_addr.s_addr = inet_addr(ip);
	} else {
		addr->sin_addr.s_addr = htonl(INADDR_ANY);

		// 绑定端口
		if (bind(listenfd, (struct sockaddr*)addr, sizeof(*addr)) == -1) {
			printf("Error bind(): %s(%d)\n", strerror(errno), errno);
			close(listenfd);
			return -1;
		}
	}
   
	return listenfd;
}

// read from server
int read_from_server(int sockfd, char* sentence)
{
    int p = 0, n;

    while ((n = read(sockfd, sentence + p, 8191 - p)) > 0) {
        p += n;
        if (sentence[p - 1] == '\n') {
            break;
        }
    }

    if (n < 0) {
        printf("Error read(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    sentence[p - 1] = '\0';
    return 0;
}

int write_to_server(int sockfd, char* sentence)
{
    int len = strlen(sentence), p = 0, n;

    while (p < len) {
        n = write(sockfd, sentence + p, len - p);
        if (n < 0) {
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
            return 1;
        }
        p += n;
    }
    return 0;
}

void parse_request(char* request, char* verb, char* info)
{
	sscanf(request, "%s %s", verb, info);
}

int get_ip_and_port(char* sentence, char* serverIP)
{
    char *start = strchr(sentence, '(');  // ip&port start
    char *end = strchr(sentence, ')');    // ip&port end

    if (start && end) {
        char addressPortStr[50], *token;
        strncpy(addressPortStr, start + 1, end - start - 1);
        addressPortStr[end - start - 1] = '\0';  // 修复越界问题
        memset(serverIP, 0, sizeof(serverIP));

        token = strtok(addressPortStr, ",");
        for (int i = 0; i < 4 && token; i++) {
            strcat(serverIP, token);
            if (i < 3) strcat(serverIP, ".");
            token = strtok(NULL, ",");
        }

        int port = token ? atoi(token) * 256 : -1;
        token = strtok(NULL, ",");
        return token ? port + atoi(token) : -1;
    }

    return -1;
}

void parse_address_port(char *token, char *ip, int *port) {
    memset(ip, 0, sizeof(char) * 256);

    // 解析IP部分
    for (int i = 0; i < 4 && token; i++) {
        strcat(ip, token);
        if (i < 3) strcat(ip, ".");
        token = strtok(NULL, ",");
    }

    // 解析Port部分
    *port = token ? atoi(token) * 256 : 0;
    token = strtok(NULL, ",");
    if (token) {
        *port += atoi(token);
    }
}

void carefully_close(int fd){
	if(fd!=-1){
		close(fd);
		fd=-1;
	}
}

int handle_port_mode(const char *sentence, int *listenfd, struct sockaddr_in *addr_PORT, char *ip, int *port) {

	// close the previous connection
	carefully_close(*listenfd);

	// parse the address and port
    char addressPortStr[50];
    strncpy(addressPortStr, sentence + 5, sizeof(addressPortStr) - 1);
    addressPortStr[sizeof(addressPortStr) - 1] = '\0';

    char *token = strtok(addressPortStr, ",");
    parse_address_port(token, ip, port);

	// create the PORT socket
    *listenfd = create_socket(*port, addr_PORT, PORT_MODE, ip);
    if (*listenfd == -1) return 1;

	// listen on the port
    if (listen(*listenfd, MAX_LISTEN) == -1) {
        printf("Error listen(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    return 0;
}

void parse_ip_and_port(int argc, char **argv, char *ip, int *port) {
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-port") == 0) {
			// 如果命令行参数中包含 -port 选项，解析端口号
			if (i + 1 < argc) {
				sscanf(argv[i + 1], "%d", port); // 从下一个参数获取端口号
			}
		} else if (strcmp(argv[i], "-ip") == 0) {
			// 如果命令行参数中包含 -ip 选项，解析
			if (i + 1 < argc) {
				strcpy(ip, argv[i + 1]);
			}
		}
	}
}

int read_and_print(int sockfd, char* sentence){
	if(read_from_server(sockfd,sentence)==1)return 1;
	printf("%s\n",sentence);
	fflush(stdout);
	return 0;
}

int to_accept(int* connfd, int listenfd){
	if ((*connfd = accept(listenfd, NULL, NULL)) == -1) {
		printf("Error accept(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	return 0;
}

int to_connect(int* connfd, struct sockaddr_in addr){
	if (connect(*connfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		printf("Error connect(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	return 0;
}

int make_connection_on_mode(int* connfd, int listenfd, int current_mode, struct sockaddr_in addr_PASV){
	if(current_mode==PORT_MODE){
		//PORT Mode
		if(to_accept(connfd, listenfd)==1) return 1;
	}
	else{
		//PASV Mode
		if(to_connect(connfd, addr_PASV)==1) return 1;
	}
	return 0;
}

int main(int argc, char **argv) {

	char sentence[8192];
	int len;
	int sockfd;
	int connfd=-1;
	int listenfd=-1;
	int current_mode=PASV_MODE;
	int connection_built_for_files=0;
	int received_flag=0;
	char ip[16]="127.0.0.1";
	int port = DEFAULT_PORT;
	char verb[5]={'\0'};
    char info[8192];
	struct sockaddr_in addr;
	struct sockaddr_in addr_PASV;
	struct sockaddr_in addr_PORT;
	FILE* fp;

	parse_ip_and_port(argc, argv, ip, &port);

	// create socket TCP
	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//set the ip and port number of the target server
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	//set the server ip 
	if (inet_pton(AF_INET, ip,&addr.sin_addr) <= 0) {// Convert IP address: Dotted Decimal to Binary
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	addr.sin_port = htons(port); // set the port number as 21

	//connect to the target server:blocking function
	to_connect(&sockfd, addr);
	
	while(1){
		// read from server
		if(!received_flag)
		{
			if(read_and_print(sockfd,sentence)==1)return 1;
		}
		received_flag=0;

		// PASV mode
		if(strncmp(sentence, "227", 3) == 0){
			// close the previous connection
			carefully_close(connfd);

			port=get_ip_and_port(sentence,ip);
			//create the PASV socket
			connfd=create_socket(port,&addr_PASV,PASV_MODE,ip);

			current_mode=PASV_MODE;

			connection_built_for_files=1;// passive
		}

		// get the command from the user
		fgets(sentence, 8192, stdin);
		// remove the newline character and replace it with the null character
		int len = strlen(sentence);
		if (sentence[len - 1] == '\n') {
			sentence[len - 1] = '\0';  // 替换 \n 为 \0
		}

		// add \r\n to meet the FTP command format
		strcat(sentence, "\r\n");
		parse_request(sentence,verb,info);

		// send command to the server	
		if(write_to_server(sockfd, sentence)==1)return 1;

		// parse the command
		if (strcmp(verb, "QUIT") == 0 || strcmp(verb, "ABRT") == 0){
			// merely close the connection
			if(read_and_print(sockfd,sentence)==1)return 1;
			break;
		} 
		else if (strcmp(verb, "PORT") == 0) {

			// set the mode to PORT
			current_mode=PORT_MODE;
			
			// handle the PORT mode
			if(handle_port_mode(sentence, &listenfd, &addr_PORT, ip, &port)==1) return 1;
			
			connection_built_for_files=1;
		}
		else if(strcmp(verb, "RETR") == 0){
			if(connection_built_for_files==1){
				// make connection first
				if(make_connection_on_mode(&connfd, listenfd, current_mode, addr_PASV)==1) return 1;

				// read the response from the server
				if(read_from_server(sockfd, sentence)==1)return 1;
				printf("%s\n",sentence);
				fflush(stdout);

				// check if more parsing is needed
				if(strncmp(sentence, "150", 3) && strncmp(sentence, "125", 3)){
					received_flag=1;
					continue;
				}

				// write the file to the client
				fp = fopen(info, "wb");
				if (fp){
					// read the file from the server
					int n;
					do {
						n = recv(connfd, sentence, BUFF_SIZE, 0);
						if (n < 0) {
							printf("Error receive file()\n");
							break;
						}
						fwrite(sentence, sizeof(char), n, fp);
					} while (n > 0);
					if(n < 0){
						printf("Error receive file()\n");
						continue;
					}
					fclose(fp);
				}
				else{
					printf("Error create file()\n");
					continue;
				}
				
				// close the connection
				connection_built_for_files=0;
				carefully_close(listenfd);
				carefully_close(connfd);
			}
		}
		else if(strcmp(verb, "STOR") == 0){
			if(connection_built_for_files==1){
				// make connection first
				if(make_connection_on_mode(&connfd, listenfd, current_mode, addr_PASV)==1) return 1;

				// read the response from the server
				if(read_and_print(sockfd,sentence)==1)return 1;
				
				// print the response
				if(strncmp(sentence, "150", 3) && strncmp(sentence, "125", 3)){
					received_flag=1;
					continue;
				}

				// write the file to client
				fp = fopen(info, "rb");
				if (fp){
					// read the file from the server
					int n;
					do {
						n = fread(sentence, sizeof(char), BUFF_SIZE, fp);
						if (n < 0) {
							printf("Error read file()\n");
							break;
						}
						send(connfd, sentence, n, 0);
					} while (n > 0);
					if(n < 0){
						printf("Error read file()\n");
						continue;
					}
					fclose(fp);
				}
				else{
					printf("File doesn't exist, please check the file");
					continue;
				}
				
				// close the connection
				connection_built_for_files=0;
				carefully_close(listenfd);
				carefully_close(connfd);
				
			}
		}
		else if(strcmp(verb, "LIST") == 0){//OVER
				if(connection_built_for_files==1){
					// make connection first
					if(make_connection_on_mode(&connfd, listenfd, current_mode, addr_PASV)==1) return 1;

					// read the response from the server
					if(read_and_print(sockfd,sentence)==1)return 1;

					// read the file from the server
					if(read_and_print(connfd,sentence)==1)return 1;
					
					// close the connection
					connection_built_for_files=0;
					carefully_close(listenfd);
					carefully_close(connfd);

				}
			}
	}

	close(sockfd);
	if (listenfd != -1) close(listenfd);

	return 0;
	
}
