#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFSIZE 1024
#define DEFAULT_PORT 5000

typedef struct {
	struct sockaddr_in addr;	//client addr
	int fd;				//file descriptor
	int uid;			//client unique ID	
	char name[64];			//client name
} client_t;

int main(int argc, char *argv[]) {
	int connfd;
	char *servIP;
	struct sockaddr_in serv_addr;
	char buf_in[MAX_BUFSIZE];
	char buf_out[MAX_BUFSIZE];

	servIP = argv[1];
	if (strlen(servIP) < 7) 
		return -1;

	connfd = socket(AF_INET, SOCK_STREAM, 0);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(servIP);
	serv_addr.sin_port = htons(DEFAULT_PORT);	

	if (connect(connfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		close(connfd);
		perror("Error connecting to server!");
		return -2;
	}
	
	printf("Connected to server.\n");

	memset(buf_in, 0, MAX_BUFSIZE);
	memset(buf_out, 0, MAX_BUFSIZE);

	char input[MAX_BUFSIZE] = {0};
	do {
		fgets(input, MAX_BUFSIZE, stdin);
		if (strlen(input) > 1) {
			input[strlen(input)-1] = '\0';

			sprintf(buf_out, "%s", input);
			write(connfd, buf_out, strlen(buf_out));		
			if (recv(connfd, buf_in, sizeof(buf_in), 0) > 0) {
				printf("Server: %s\n", buf_in);
			}
		}
		else
			printf("Empty input!\n");
	} while (strcmp(input, "/quit") != 0);

	close(connfd);	
}
