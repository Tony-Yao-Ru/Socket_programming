#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<netdb.h>

#define DEST_PORT 2000
#define SERVER_IP_ADDR "192.168.1.106"


typedef struct _test_struct{
	unsigned int a;
	unsigned int b;
} test_struct_t;

typedef struct result_struct_{
	unsigned int c;
} result_struct_t;


test_struct_t client_data;
result_struct_t result;


void 
conduct_tcp_client() {
	
	/* (a) Intialize the variables */
	int sockfd = 0;
	int addr_len = 0;
	addr_len = sizeof(struct sockaddr);
	int sent_recv_bytes = 0;
	struct sockaddr_in dest;

	/* (b) Specify server info */
	dest.sin_family = AF_INET;
	dest.sin_port = DEST_PORT;
	struct hostent *host = (struct hostent *)gethostbyname(SERVER_IP_ADDR); // hostent structure is used by the functions to store info about
										// a given host.
	dest.sin_addr = *((struct in_addr *)host->h_addr);

	/* (c) Create a TCP socket */
	//sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	connect(sockfd, (struct sockaddr *)&dest, sizeof(struct sockaddr));

	/* (d) Get the data to be sent to the server */

PROMPT_USER:
	// Prompt the user to enter data
	printf("Enter a: ?\n");
	scanf("%u", &client_data.a);
	printf("Enter b: ?\n");
	scanf("%u", &client_data.b);

	// Send the data to the server
	sent_recv_bytes = sendto(sockfd,
			&client_data,
			sizeof(test_struct_t),
			0,
			(struct sockaddr *)&dest,
			sizeof(struct sockaddr));
	printf("No of bytes sent = %d\n", sent_recv_bytes);

	// Reply from the server after sending the data
	sent_recv_bytes = recvfrom(sockfd,
			(char *)&result,
			sizeof(result_struct_t),
			0,
			(struct sockaddr *)&dest,
			&addr_len);
	printf("No of bytes recvd = %d\n", sent_recv_bytes);

	printf("Result recvd = %u\n", result.c);
	goto PROMPT_USER;
}


int
main(int argc, char* argv[]) {
	
	conduct_tcp_client();
	printf("Application quits\n");
	return 0;
}
