#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <time.h>

#define MAXLINE 1024
#define PORTNUM 3600

struct data {
    char str[MAXLINE];
    char server_tm[MAXLINE];
};

struct thread_arg_node {
	struct data *buf;
	int client_fd;
	struct sockaddr_in client_addr;
	pthread_mutex_t *lock;
	pthread_cond_t *cond;
	struct thread_arg_node *nextNode;
};

void * producer_func(void *param)
{
	struct thread_arg_node *arg = (struct thread_arg_node *)param;

	int client_fd = arg->client_fd;
	struct data *buf = arg->buf;
	struct sockaddr_in client_addr = arg->client_addr;
	int i,readn;
	char temp;
	time_t t;
	struct tm* pTimeInfo;

	t = time(NULL);
	pTimeInfo = localtime(&t);

	pthread_mutex_lock(arg->lock);

	readn = read(client_fd, buf, sizeof(struct data));

	pthread_mutex_unlock(arg->lock);

	printf("Read Data %s (%d) : %s and %s\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port, buf->str, buf->server_tm);
	
	while(1)
	{
		pthread_mutex_lock(arg->lock);

		for(i = 0; i <= strlen(buf->str); i++)
		{
			if(i == 0)
			{
				temp = buf->str[0];
			}
			if(buf->str[i] != '\0'){
				buf->str[i-1] = buf->str[i];
			}else{
				buf->str[i-1] = temp;
			}
		}

		strcpy(buf->server_tm, asctime(pTimeInfo));

		pthread_mutex_unlock(arg->lock);
		pthread_cond_signal(arg->cond);

		sleep(1);

	}
}

void * consumer_func(void *param)
{
	struct thread_arg_node *arg = (struct thread_arg_node *)param;

    int client_fd = arg->client_fd;
    struct data *buf = arg->buf;
    struct sockaddr_in client_addr = arg->client_addr;
	
	while(1)
	{
		pthread_mutex_lock(arg->lock);
		pthread_cond_wait(arg->cond, arg->lock);

		write(client_fd, buf, sizeof(struct data));

		pthread_mutex_unlock(arg->lock);
	}
}

int main(int argc, char **argv)
{
	int listen_fd, client_fd;
	socklen_t addrlen;
	int readn;
	pthread_mutex_t *lock;
	pthread_cond_t *cond;
	struct data *buf;
	struct thread_arg_node *head = malloc(sizeof(struct thread_arg_node));
	struct thread_arg_node *curr = head -> nextNode;
	struct sockaddr_in client_addr, server_addr;
	pthread_t thread_id[2];

	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket error");
		return 1;
	}

	memset((void *)&server_addr, 0x00, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORTNUM);

	if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		perror("bind error");
		return 1;
	}

	if(listen(listen_fd, 5) == -1)
	{
		perror("listen error");
		return 1;
	}

	signal(SIGPIPE, SIG_IGN);

	while(1)
	{
		addrlen = sizeof(client_addr);

        client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);

		if(client_fd == -1)
		{
			printf("accept error\n");
			break;
		}
		while(curr != NULL)
		{
			curr = curr->nextNode;
		}
		curr = malloc(sizeof(struct thread_arg_node));
		
		buf = malloc(sizeof(struct data));
		
		lock = malloc(sizeof(pthread_mutex_t));
		cond = malloc(sizeof(pthread_cond_t));		
	
		curr->buf = buf;
		curr->client_fd = client_fd;

		curr->client_addr = client_addr;
		
		curr->lock = lock;
		curr->cond = cond;

		if(pthread_mutex_init(curr->lock, 0) != 0)
		{
			perror("Mutex Init failure");
			return 1;
		}

		if(pthread_cond_init(curr->cond, 0) != 0)
		{
			perror("Cond Init failure");
			return 1;
		}

               	if(pthread_create(&thread_id[0], NULL, producer_func, (void *)curr) != 0)
		{
			perror("pthread_create failure");
			return 1;
		}
		if(pthread_detach(thread_id[0]) != 0)
		{
			perror("pthread_detach failure");
			return 1;
		}

		if(pthread_create(&thread_id[1], NULL, consumer_func, (void *)curr) != 0)
		{
			perror("pthread_create failure");
			return 1;
		}
		if(pthread_detach(thread_id[1]) != 0)
		{
			perror("pthread_detach failure");
			return 1;
		}
	}
	return 0;
}