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

pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int thread_count = 0;
static int connfd;

void *handle_input(void *arg) {
	char input[MAX_BUFSIZE];
	memset(input, 0, sizeof(input));
	while (fgets(input, MAX_BUFSIZE, stdin)) {
		if (strlen(input) > 1) {
			input[strlen(input)-1] = '\0';
			write(connfd, input, strlen(input));
			if (strcmp(input, "/quit") == 0) {
				pthread_mutex_lock(&thread_mutex);
				thread_count--;
				pthread_mutex_unlock(&thread_mutex);
				return NULL;
			}	
		}
		memset(input, 0, sizeof(input));
	}
}

int main(int argc, char *argv[]) {
	char *servIP;
	struct sockaddr_in serv_addr;
	char buffer[MAX_BUFSIZE];
	pthread_t tinput;

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

	pthread_mutex_lock(&thread_mutex);
	thread_count++;
	pthread_mutex_unlock(&thread_mutex);
	pthread_create(&tinput, NULL, &handle_input, NULL);

	memset(buffer, 0, MAX_BUFSIZE);
	while (thread_count > 0) {
		memset(buffer, 0, MAX_BUFSIZE);
		if (recv(connfd, buffer, sizeof(buffer), 0) > 0) {
			printf("Server: %s\n", buffer);
		}
	}

	close(connfd);	
}
