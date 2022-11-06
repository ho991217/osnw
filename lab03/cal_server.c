#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <limits.h>

#define PORT 3600
#define MAX_LEN 1024

struct cal_data
{
        int left_num;
        int right_num;
        char op;
        int result;
        short int error;
};

struct min_max_data
{
	int max;
	int min;
	char max_client[MAX_LEN];
	char min_client[MAX_LEN];
};

int main(int argc, char **argv)
{
        struct sockaddr_in client_addr, sock_addr;
        int listen_sockfd;
        int addr_len;
        struct cal_data rdata;
        int left_num, right_num, cal_result;
        short int cal_error;
	int client_sockfd[3];
	int max_value = INT_MIN;
	int min_value = INT_MAX;
	int i;
	struct min_max_data result;

        if( (listen_sockfd  = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
                perror("Error ");
                return 1;
        }

        memset((void *)&sock_addr, 0x00, sizeof(sock_addr));
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        sock_addr.sin_port = htons(PORT);

        if( bind(listen_sockfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1)
        {
                perror("Error ");
                return 1;
        }

        if(listen(listen_sockfd, 5) == -1)
        {
                perror("Error ");
                return 1;
        }

        for(;;)
        {
		for (i = 0; i < 3; i++) {
                	addr_len = sizeof(client_addr);
                	client_sockfd[i] = accept(listen_sockfd, (struct sockaddr *)&client_addr, &addr_len);
                	if(client_sockfd[i] == -1)
                	{
                        	perror("Error ");
                        	return 1;
                	}
			printf("New Client Connect : %s\n", inet_ntoa(client_addr.sin_addr));

	                read(client_sockfd[i], (void *)&rdata, sizeof(rdata));

	                cal_result = 0;
	                cal_error = 0;

        	        left_num = ntohl(rdata.left_num);
        	        right_num = ntohl(rdata.right_num);

	                switch(rdata.op)
        	        {
                	        case '+':
                        	        cal_result = left_num + right_num;
                        	        break;
                        	case '-':
                                	cal_result = left_num  - right_num;
                                	break;
	                        case '*':
	                                cal_result = left_num * right_num;
	                                break;
	                        case '/':
	                                if(right_num == 0)
	                                {
	                                        cal_error = 2;
	                                        break;
	                                }
	                                cal_result = left_num / right_num;
	                                break;
	                        default:
	                                cal_error = 1;

	                }

	                rdata.result = htonl(cal_result);
	                rdata.error = htons(cal_error);
			printf("%d %c %d = %d\n", left_num, rdata.op, right_num, cal_result);

			if (max_value < cal_result) {
				max_value = cal_result;
				strcpy(result.max_client, inet_ntoa(client_addr.sin_addr));
			}
			if (min_value > cal_result) {
				min_value = cal_result;
				strcpy(result.min_client, inet_ntoa(client_addr.sin_addr));
			}

			result.max = max_value;
			result.min = min_value;
		}

		for (i = 0; i < 3; i++) {
			write(client_sockfd[i], (void *)&result, sizeof(result));
	                close(client_sockfd[i]);
	        }
	}

        close(listen_sockfd);
        return 0;
}

