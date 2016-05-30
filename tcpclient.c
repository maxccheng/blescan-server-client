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

	connfd = socket(AF_INET, SOCK_STREAM, 0);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(servIP);
	serv_addr.sin_port = htons(DEFAULT_PORT);	

	if (connect(connfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("Error connecting to server!");
	}
	
	printf("Connected to server.\n");

	memset(buf_in, 0, MAX_BUFSIZE);
	memset(buf_out, 0, MAX_BUFSIZE);
	recv(connfd, buf_in, sizeof(buf_in), 0);
	
	if (strlen(buf_in)) {	
		printf("Data received: %s\n", buf_in);
		char *input;
		fgets(input, MAX_BUFSIZE, stdin);
		const char *tmp = input;
		printf(buf_out, tmp);
		write(connfd, buf_out, strlen(buf_out));		
	}
	else
		printf("No data!");
		
}
