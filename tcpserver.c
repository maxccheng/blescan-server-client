#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLI 200
#define MAX_BUFSIZE 1024
#define DEFAULT_PORT 5000

static int count_cli = 0;
static int uid = 10;
static const char fsl = '/';

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

void strip_newline(char *s){
	while(*s != '\0'){
		if(*s == '\r' || *s == '\n'){
			*s = '\0';
		}
		s++;
	}
}

void print_client_addr(struct sockaddr_in addr){
	printf("%d.%d.%d.%d",
		addr.sin_addr.s_addr & 0xFF,
		(addr.sin_addr.s_addr & 0xFF00)>>8,
		(addr.sin_addr.s_addr & 0xFF0000)>>16,
		(addr.sin_addr.s_addr & 0xFF000000)>>24);
}

void *handle_client(void *arg) {
	char buf_in[MAX_BUFSIZE];	
	char buf_out[MAX_BUFSIZE];	
	int rlen;

	count_cli++;
	client_t *cli = (client_t *)arg;

	//printf("Accept client IP:%s UID:%s\n", print_client_addr(cli->addr), cli->uid);

	while ((rlen = read(cli->fd, buf_in, sizeof(buf_in)-1)) > 0) {
		buf_in[rlen] = '\0';
		buf_out[0] = '\0';
		strip_newline(buf_in);
		
		if (!strlen(buf_in))
			continue;

		//commands
		if (buf_in[0] == fsl) {
			char *cmd, *param;
			cmd = strtok(buf_in, " ");
			if (strcmp(cmd, fsl + "synctime")) {
				sprintf(buf_out, "Sync time is xxx");
				send_msg(buf_out, cli->uid);
				//printf("Received synctime request IP:%s UID:%s\n", print_client_addr(cli->addr), cli->uid);
			}
		}
		
		close(cli->fd);
		//printf("Closing IP:%s UID:%s\n", print_client_addr(cli->addr), cli->uid);
		del_client(cli->uid);
		free(cli);
		count_cli--;
		pthread_detach(pthread_self());
		
		return NULL;	
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
	}

	if (listen(listenfd, 0) < 0) {
		perror("Socket listen failed!");
	}
	
	printf("Server started.\n");
	
	while (1) {
		socklen_t len_cli = sizeof(client_addr);
		connfd = accept(listenfd, (struct sockaddr *)&client_addr, &len_cli);

		if (count_cli+1 == MAX_CLI) {
			printf("Max client (%d) reached. Connection rejected.", MAX_CLI);
			close(connfd);
			continue;
		}

		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->addr = client_addr;
		cli->fd = connfd;
		cli->uid = uid++;
		sprintf(cli->name, "%s", "Client" + cli->uid);
		
		add_client(cli);
		pthread_create(&tid, NULL, &handle_client, (void *)cli);
		
		sleep(1);
	}
}
