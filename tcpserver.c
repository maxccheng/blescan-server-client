#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

#define MAX_CLI 200
#define MAX_BUFSIZE 1024
#define DEFAULT_PORT 5000

static int count_cli = 0;
static int uid = 10;

typedef struct {
	struct sockaddr_in addr;	//client addr
	int fd;				//file descriptor
	int uid;			//client unique ID	
	char name[64];			//client name
} client_t;

client_t *clients[MAX_CLI];

void add_client(client_t *c) {
	for (int i=0; i<MAX_CLI; i++) {
		if (!clients[i]) {
			clients[i] = c;
			return;
		}
	}
}

void del_client(int uid) {
	for (int i=0; i<MAX_CLI; i++) {
		if (clients[i]) {
			if (clients[i]->uid == uid) {
				clients[i] = NULL;
				return;
			}
		}
	}
}

void send_msg(char *s, int uid) {
	for (int i=0; i<MAX_CLI; i++) {
		if (clients[i]) {
			if (clients[i]->uid == uid) {
				write(clients[i]->fd, s, strlen(s));
				return;
			}
		}
	}
}

void send_msg_all(char *s) {
	for (int i=0; i<MAX_CLI; i++) {
		if (clients[i]) {
			write(clients[i]->fd, s, strlen(s));
			return;
		}
	}
}

void sleep_ms(int milliseconds) // cross-platform sleep function
{
	#ifdef WIN32
		Sleep(milliseconds);
	#elif _POSIX_C_SOURCE >= 199309L
		struct timespec ts;
		ts.tv_sec = milliseconds / 1000;
		ts.tv_nsec = (milliseconds % 1000) * 1000000;
		nanosleep(&ts, NULL);
	#else
		usleep(milliseconds * 1000);
	#endif
}

void *handle_client(void *arg) {
	char buf_in[MAX_BUFSIZE];	
	char buf_out[MAX_BUFSIZE];	
	int rlen;

	count_cli++;
	client_t *cli = (client_t *)arg;

	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(cli->addr.sin_addr), ip, INET_ADDRSTRLEN);
	printf("Accept client IP: %s uid: %d\n", ip, cli->uid);
	memset(buf_in, 0, sizeof(buf_in));
	memset(buf_out, 0, sizeof(buf_out));
	while ((rlen = read(cli->fd, buf_in, sizeof(buf_in)-1)) > 0) {
		buf_in[rlen] = '\0';
	
		printf("Client uid %d: %s\n", cli->uid, buf_in);	

		memset(buf_out, 0, sizeof(buf_out));

		//commands
		if (buf_in[0] == '/') {
			char *cmd, *param;
			cmd = strtok(buf_in, " ");
			if (strcmp(cmd, "/synctime") == 0) {
				sprintf(buf_out, "Sync time is xxx");
				send_msg(buf_out, cli->uid);
				printf("Client uid %d: %s\n", cli->uid, "reply /synctime command");	
			}
			else if (strcmp(cmd, "/quit") == 0) {
				close(cli->fd);
				printf("Closing uid: %d\n", cli->uid);
				del_client(cli->uid);
				free(cli);
				count_cli--;
				pthread_detach(pthread_self());
				return NULL;
			}
		}
		else {
			sprintf(buf_out, "ack");
			send_msg(buf_out, cli->uid);
		}
	}	
}

int main(int argc, char *argv[]) {
	int listenfd, connfd;
	struct sockaddr_in serv_addr;
	struct sockaddr_in client_addr;
	pthread_t tid;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(DEFAULT_PORT);	

	if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) > 0) {
		perror("Socket bind failed!");
		return -2;
	}

	if (listen(listenfd, 0) < 0) {
		perror("Socket listen failed!");
		return -3;
	}
	
	printf("Server started.\n");
	
	while (1) {
		socklen_t len_cli = sizeof(client_addr);
		connfd = accept(listenfd, (struct sockaddr *)&client_addr, &len_cli);
		printf("Accepted connection from client connfd = %d\n", connfd);

		if (count_cli+1 == MAX_CLI) {
			printf("Max client (%d) reached. Connection rejected.\n", MAX_CLI);
			close(connfd);
			continue;
		}

		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->addr = client_addr;
		cli->fd = connfd;
		cli->uid = uid++;
		sprintf(cli->name, "%s", "ClientName");
		
		add_client(cli);
		pthread_create(&tid, NULL, &handle_client, (void *)cli);

		sleep_ms(100);
	}
}
