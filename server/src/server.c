#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#define BUFF_SIZE 8192
#define DEFAULT_PORT 21
#define DEFAULT_DIR_PATH "/tmp"
#define MAX_LISTENS 10
#define PASV_MODE 0
#define PORT_MODE 1
#define STATE_USER 0
#define STATE_PASS 1
#define STATE_VARIFIED 2

char root[512];
char verb[BUFF_SIZE];
char info[400];
char chosenfile[BUFF_SIZE];
char response[BUFF_SIZE];



void read_from_socket(int connfd, char* sentence) {
    int p = 0, n;

    do {
        n = read(connfd, sentence + p, 8191 - p);
        if (n < 0) {
            printf("Error read(): %s(%d)\n", strerror(errno), errno);
            close(connfd);
            return;  // 读取错误后直接返回
        }
        p += n;
    } while (n > 0 && sentence[p - 1] != '\n');

    if (n > 0) {
        sentence[p - 1] = '\0';  // 确保字符串以 '\0' 结尾
        printf(">>>Client: %s\n", sentence);
    }
}

void write_to_socket(int connfd,char* sentence,int len){
	printf(">>>Server: %s",sentence);
	int p = 0;
	while (p < len) {
		int n = write(connfd, sentence + p, len + 1 - p);
		if (n < 0) {
			printf("Error write(): %s(%d)\n", strerror(errno), errno);
			return;
		} else {
			p += n;
		}			
	}

}

int is_valid_command(char* sentence) {
    // 定义所有合法的FTP命令
    const char* valid_commands[] = {
        "USER", "PASS", "RETR", "STOR", "QUIT", "SYST", "TYPE", 
        "PORT", "PASV", "MKD", "CWD", "PWD", "LIST", "RMD", 
        "ABRT"
    };
    
    int num_commands = sizeof(valid_commands) / sizeof(valid_commands[0]);

    for (int i = 0; i < num_commands; i++) {
        if (strncmp(sentence, valid_commands[i], strlen(valid_commands[i])) == 0) {
            return 1; // 命令有效
        }
    }
    return 0; // 命令无效
}

void parse_request(char* request, char* verb, char* info)
{
	sscanf(request, "%s %s", verb, info);
}

int create_socket(int port, struct sockaddr_in* addr, int mode, char* ip) {
    int listenfd;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }

    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    // 根据模式决定使用 INADDR_ANY 还是特定的 IP 地址
    if (mode == PORT_MODE && ip != NULL) {
        addr->sin_addr.s_addr = inet_addr(ip);
    } else {
        addr->sin_addr.s_addr = htonl(INADDR_ANY);
    }

    return listenfd;
}

int get_ip_and_port(char* sentence,char* ip){
	char *start = sentence+5;
	char *end = strchr(start,'\0');
	
	if (start && end) {
		// get ip and port number
		char addressPortStr[50];
		strncpy(addressPortStr, start, end-start);
		addressPortStr[end-start] = '\0';

		memset (ip, 0, BUFF_SIZE);

		char *token = strtok(addressPortStr, ",");
		
		for (int i = 0; i < 4 && token; i++) {
			strcat(ip, token);
			if (i < 3) strcat(ip, ".");
			token = strtok(NULL, ",");
		}
		int port = token ? atoi(token) * 256 : -1;
		token = strtok(NULL, ",");
		if (token) port += atoi(token);
		return port;
	}
	return -1;
}

void get_port_and_root(char** argv, int argc, int *port, char* root) {

    int i;
    *port = DEFAULT_PORT;         // 设置默认端口号
    strcpy(root, DEFAULT_DIR_PATH); // 设置默认根目录路径

    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-port") == 0) {
            // 如果命令行参数中包含 -port 选项，解析端口号
            if (i + 1 < argc)sscanf(argv[i + 1], "%d", port); // 从下一个参数获取端口号
        } 
		else if (strcmp(argv[i], "-root") == 0) {
            // 如果命令行参数中包含 -root 选项，解析根目录路径
            if (i + 1 < argc)
			{
				strcpy(root, argv[i + 1]); // 从下一个参数获取根目录路径
			}
        }
    }
}

int file_size(FILE *file)
{
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);
	return size;
}

int carefully_close(int* fd){
	if(*fd!=-1){
		close(*fd);
		*fd=-1;
	}
	return 0;
}

int to_accept(int* fd, int listenfd){
	if ((*fd = accept(listenfd, NULL, NULL)) == -1) {
		printf("Error accept(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	return 0;
}

int to_connect(int* filefd, struct sockaddr_in addr){
	if (connect(*filefd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		printf("Error connect(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	return 0;
}

int make_connection_on_mode(int* filefd, int listenfd, int current_mode, struct sockaddr_in addr){
	if(current_mode == PASV_MODE){
		if (to_accept(filefd, listenfd) == 1) return 1;
	}
	else if (to_connect(filefd, addr) == 1) return 1;
	else printf("connect \n");
	return 0;
}

void generate_response(char* sentence, int code, char* words, int connfd){
		snprintf(response,sizeof(response),"%s",words);
		sprintf(sentence, "%d %s\r\n", code, response);
		write_to_socket(connfd, sentence, strlen(sentence)-1);
}

int connection_failed(int ret, int connfd, int listenfd, int filefd, char* sentence){
	if(ret)
	{
		generate_response(sentence, 425, "Data Connection Failed to connect.", connfd);
		carefully_close(&listenfd);
		carefully_close(&filefd);
		return 1;
	}
	return 0;
}

int to_bind(int listenfd, struct sockaddr_in addr){
	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	return 0;
}

int to_listen(int listenfd){
	if (listen(listenfd, MAX_LISTENS) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	return 0;
}

void *thread(void *arg){
	chdir(root);
	int connfd = *((int *)arg), filefd=-1, listenfd=-1;
	char sentence[BUFF_SIZE];
	int port;
	struct sockaddr_in addr_PASV;
	struct sockaddr_in addr_PORT;
	char ip[BUFF_SIZE];
    
	int state = STATE_USER;

	int current_mode=PASV_MODE;

	int file_len = 0;
	int connection_built_for_files= 0;

	generate_response(sentence, 220, "Anonymous FTP server ready.", connfd);

	while(1){
		read_from_socket(connfd,sentence);
		
		// Process data from the client
		if (!is_valid_command(sentence)) {
			generate_response(sentence, 500, "Unknown command.", connfd);
			continue;
		}

		parse_request(sentence,verb,info);

		if ((strcmp(verb, "ABRT") == 0)||(strcmp(verb, "QUIT") == 0)){
			generate_response(sentence, 221, "Thank you for using the FTP service on ftp.ssast.org. Goodbye.", connfd);
			carefully_close(&connfd);
			carefully_close(&filefd);
			carefully_close(&listenfd);
			return 0;
		}
		else if(strcmp(verb,"USER") == 0){
			if(strcmp(info,"anonymous") == 0){
				generate_response(sentence, 331, "Guest login ok, send your complete e-mail address as password.", connfd);
				state = STATE_PASS;
			}
			else generate_response(sentence, 430, "Invalid username.", connfd);
		}
		else if(strcmp(verb,"PASS") == 0){
			if(state==STATE_USER){
				generate_response(sentence, 530, "Not logged in, password error.", connfd);
			}
			else if(state==STATE_PASS){
				// may need to check the password further
				generate_response(sentence, 230, "login success.", connfd);
				state = STATE_VARIFIED;
			}
			else{
				generate_response(sentence, 503, "Bad sequence of commands.", connfd);
			}
		}
		else if(state == STATE_VARIFIED)
		{
			if(strcmp(verb, "RETR") == 0){
				if(connection_built_for_files==1){
					int ret = make_connection_on_mode(&filefd, listenfd, current_mode, addr_PORT);
					if (connection_failed(ret, connfd, listenfd, filefd, sentence)) continue;
					
					FILE *fp = fopen(info, "rb");
					if (!fp) {
						generate_response(sentence, 551, "File not found.", connfd);
						continue;
					}
					else{
						generate_response(sentence, 150, "Opening BINARY mode data connection.", connfd);
						int read_len;
						while ((read_len = fread(sentence, sizeof(char), BUFF_SIZE, fp))) {
							if(send(filefd,sentence,read_len,0)<0){
								printf("Error write(): %s(%d)\n",strerror(errno),errno);
							}
						}
						fclose(fp);
					}
					
					
					connection_built_for_files=0;

					carefully_close(&listenfd);
					carefully_close(&filefd);
					generate_response(sentence, 226, "Transfer complete.", connfd);
				}
				else generate_response(sentence, 503, "Bad sequence of commands.", connfd);
			}
			else if(strcmp(verb, "STOR") == 0){
				if(connection_built_for_files==1){
					int ret = make_connection_on_mode(&filefd, listenfd, current_mode, addr_PORT);
					if (connection_failed(ret, connfd, listenfd, filefd, sentence)) continue;
					
					FILE *fp;
					struct timespec start, end;
					double time;

					//receive the file from client
					fp = fopen(info, "wb");
					int size;

					if (fp == NULL) {
						// sprintf(sentence,"551 File created error.\r\n");
						// write_to_socket(connfd, sentence, strlen(sentence)-1);
						generate_response(sentence, 551, "File created error.", connfd);
						continue;
					}
					else{
						// sprintf(sentence, "150 Opening BINARY mode data connection for %s\r\n", info);
						// write_to_socket(connfd, sentence, strlen(sentence)-1);
						generate_response(sentence, 150, "Opening BINARY mode data connection.", connfd);
						
						int recv_len;
						clock_gettime(CLOCK_MONOTONIC, &start);
						while ((recv_len=recv(filefd,sentence, BUFF_SIZE, 0))>0) {
							fwrite(sentence, sizeof(char), recv_len, fp);
						}
						if(recv_len<0){
							printf("Error receive file().\r\n");
							continue;
						}
						clock_gettime(CLOCK_MONOTONIC, &end);
						size = file_size(fp);
						fclose(fp);
					}
					
					time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
					
					connection_built_for_files=0;
					carefully_close(&listenfd);
					carefully_close(&filefd);
					sprintf(sentence, "226 Transfer complete. Transferred:%d Bytes. Average speed:%.2lf KB/s.\r\n", size, (double)size / time / 1024);
					write_to_socket(connfd, sentence, strlen(sentence)-1);
				}
				else generate_response(sentence, 503, "Bad sequence of commands.", connfd);
			}
			else if (strcmp(verb, "PORT") == 0){

				carefully_close(&filefd);

				port=get_ip_and_port(sentence,ip);
				if(port==-1){
					generate_response(sentence, 501, "Invalid command arguments.", connfd);
					continue;
				}
				
				filefd = create_socket(port, &addr_PORT, PORT_MODE, ip);

				current_mode = PORT_MODE;
				connection_built_for_files = 1;

				if (filefd == -1) generate_response(sentence, 425, "Data Connection Failed to connect.", connfd);
				else generate_response(sentence, 200, "PORT command successful.", connfd);
			}
			else if (strcmp(verb, "PASV") == 0){
				carefully_close(&listenfd);
				current_mode=0;

				srand((unsigned)time(0));
				port=rand() % 45535 + 20000;
				int port_index_1 = port / 256;
				int port_index_2 = port % 256;
				sprintf(ip,"127.0.0.1");

				listenfd = create_socket(port, &addr_PASV, PASV_MODE, ip);							
				if (to_bind(listenfd, addr_PASV) == 1) {
					write_to_socket(connfd, sentence, strlen(sentence)-1);
					continue;
				}
				if (to_listen(listenfd) == 1) {
					write_to_socket(connfd, sentence, strlen(sentence)-1);
					continue;
				}
				connection_built_for_files=1;
				char response[BUFF_SIZE];
				snprintf(response, sizeof(response), "Entering Passive Mode (127,0,0,1,%d,%d)", port_index_1, port_index_2);
				sprintf(sentence, "%d %s\r\n", 227, response);
				write_to_socket(connfd, sentence, strlen(sentence)-1);
			}
			else if(strcmp(verb, "SYST") == 0) generate_response(sentence, 215, "UNIX Type: L8", connfd);
			else if (strcmp(verb, "TYPE") == 0){
				if(strcmp(info,"I")==0) generate_response(sentence, 200, "Type set to I.", connfd);
				else generate_response(sentence, 501, "Type Error.", connfd);
			}
			else if (strcmp(verb, "MKD") == 0){
				if (mkdir(info, 0777) == -1){
					generate_response(sentence, 504, "Failed to create directory.", connfd);
					continue;
				}
				chdir(info);
				char now_path[50];
				getcwd(now_path, 50);
				generate_response(sentence, 257, "Directory created successfully", connfd);
			}
			else if (strcmp(verb, "CWD") == 0){
				char now_path[50], chg_path[50];
				getcwd(now_path, 50); // 记录更改前的目录
				if (chdir(info) == -1)
				{
					generate_response(sentence, 550, "CWD command failed: directory not found.", connfd);
					continue;
				}
				getcwd(chg_path, 50);					   // 更改后的目录
				if (strncmp(chg_path, root, strlen(root))) // 更改后的目录不在根目录下，改回原来的目录
				{
					chdir(now_path);
					generate_response(sentence, 504, "Invalid directory.", connfd);
					continue;
				}
				generate_response(sentence, 250, "Change directory success.", connfd);
			}
			else if (strcmp(verb, "PWD") == 0){
				char cwd[400];
				if (getcwd(cwd, sizeof(cwd)) != NULL) {
					// Send the current working directory as a response
					snprintf(sentence, sizeof(sentence), "257 \"%s\" is the current directory.\r\n", cwd);
				} else {
					sprintf(sentence, "550 Failed to get current directory.\r\n");
				}
				write_to_socket(connfd, sentence, strlen(sentence)-1);
			}
			else if (strcmp(verb, "LIST") == 0){//TO BE MODIFIED
				if(connection_built_for_files==1){
					int ret = make_connection_on_mode(&filefd, listenfd, current_mode, addr_PORT);
					if (connection_failed(ret, connfd, listenfd, filefd, sentence)) continue;
					
					FILE *fp = popen("ls -l", "r");
					if (!fp) {
						printf("Error opening fp.\r\n");
						continue;
					}
					
					generate_response(sentence, 150, "Opening data channel for directory list.", connfd);

					int read_len;
					while ((read_len = fread(sentence, sizeof(char), 8192, fp)) > 0)
					{
						write_to_socket(filefd, sentence, strlen(sentence)-1);
						if (send(filefd, sentence, file_len, 0) < 0)
						{
							printf("Error send(): %s(%d)\n", strerror(errno), errno);
							continue;
						}

					}
					pclose(fp);
					close(listenfd);
					close(filefd);

					connection_built_for_files=0;
					carefully_close(&listenfd);
					carefully_close(&filefd);
					generate_response(sentence, 226, "List complete.", connfd);
				}
				else generate_response(sentence, 503, "Bad sequence of commands.", connfd);
			}
			else if (strcmp(verb, "RMD") == 0){
				if(rmdir(info)==0) generate_response(sentence, 250, "Directory deleted successfully.", connfd);
				else{
					sprintf(sentence, "550 Delete directory failed. %s\r\n", strerror(errno));
					write_to_socket(connfd, sentence, strlen(sentence)-1);
				}
			}
		}
		else generate_response(sentence, 530, "Please login with USER and PASS first.", connfd);
	}							
}
int main(int argc, char **argv) {

	int listenfd, connfd;
	int port;
	struct sockaddr_in addr;
	// Getting port and root
    get_port_and_root(argv, argc, &port, root);

	// create socket
	if ((listenfd = create_socket(port, &addr, PASV_MODE, NULL)) == -1) return 1;

	// bind ip and port to socket and listen
	if (to_bind(listenfd, addr) == 1) return 1;
	if (to_listen(listenfd) == 1) return 1;

	//持续监听连接请求
	while (1) {
		// 接受连接请求
		if ((to_accept(&connfd, listenfd)) == -1) continue;
		pthread_t thread_id;
		// 创建新线程
		if (pthread_create(&thread_id, NULL, thread, &connfd))
			printf("thread create error\n");
		pthread_detach(thread_id);
	}

	close(listenfd);
	return 0;
}