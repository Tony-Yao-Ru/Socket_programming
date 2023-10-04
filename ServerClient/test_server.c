#include<stdio.h>
#include<stdlib.h>
#include<memory.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<errno.h>
#include<arpa/inet.h>

#define SERVER_PORT 2000

typedef struct _test_struct{
	unsigned int a;
	unsigned int b;
} test_struct_t;

typedef struct result_struct_{
	unsigned int c;
} result_struct_t;


char data_buffer[1024];

test_struct_t test_struct;
result_struct_t res_struct;


void conduct_tcp_server() {
	
	/* (a) Intialize the variables */
	int master_socktcp_fd = 0,
	    comm_socket_fd = 0;
	struct sockaddr_in server_addr, client_addr; // Store the server and client info, which included in "netinet/in.h"
	int  addr_len = 0;

	fd_set readfds;
	int sent_recv_bytes = 0;
	test_struct_t *client_data;

	/* (b) Create master socket */
	// socket()- create an endpoint for communication 
	// int socket(int domain, int type, int protocol)
	// IPPROTO_* is designed for options of a specific network protocol, such as IPPROTO_IP, IPPROTO_TCP
	if ( ( master_socktcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) ) == -1 ) {
		printf("Socket creation failed!!!\n");
		exit(1);
	}

	/* (c) Specify server information */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = SERVER_PORT;
	server_addr.sin_addr.s_addr = INADDR_ANY; // This is an IP address that is used when we don't want to bind a socket to any specific IP.
	// server_addr.sin_addr.s_addr = inet_addr(IP) // This is normal condition that u want to bind specific IP address with socket
	addr_len = sizeof(server_addr);

	/* (d) Bind the server */
	// bind()- bind a name to a socket. When a socket is created with socket(), it exists in a name space but has no address assigned to it.
	// int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
	//
	if ( ( bind(master_socktcp_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) ) == -1 ) {
		printf("Socket bind failed!!!\n");
		return;
	}
	
	/* (e) Listen for the connections on a socket */
	// listen()- listen for connections on a socket
	// int listen(int sockfd, int backlog)
	// backlog- define the max length to which the queue of pending connections for sockfd may grow.
	//
	if (listen(master_socktcp_fd, 5)<0) {
		printf("Listen failed!!!\n");
		return;
	}
	
	/* (f) Service the client */
	while(1) {
		// Initialize and fill the readfds
		//
		FD_ZERO(&readfds); // Clear a set
		FD_SET(master_socktcp_fd, &readfds); // Add a given file descriptor from a set
		printf("Blocked on select SYSTEM CALL...\n");
		
		/* Wait for client connection */
		// State machine: state 1
		//
		// select()- Allow a program to monitor multiple file descriptors, waiting until one or more FDs become "ready"
		// int select(int nfds, fd_set readfds, fd_set writefds, fd_set exceptfds, struct timeout)
		// nfds- This argument should be set to the highest-numbered fd in any of the three sets, plus 1.
		// readfds- The fd in this set are watched to see if they are ready for reading.
		// writefds- The fd in this set are watched to see if they are ready for writing.
		select(master_socktcp_fd+1, &readfds, NULL, NULL, NULL);

	       	if (FD_ISSET(master_socktcp_fd, &readfds)) // After calling select(), FD_ISSET() macro can be used to test
							   // if fd is still present in a set.
		{
			// Data arrives on Master Socket only when new client connects with the server.
			printf("New connection recieved recvd, accept the connection.\n");
			printf("Client and server completes TCP-3 way handshake at this point\n");
			
			// Accept a new connection request
			// State machine: state 2
			//
			// accept()- Return a new temporay file descriptor
			// int accept(int sockfd, struct sockaddr addr, socklen_t addrlen, int flags)
			//
			comm_socket_fd = accept(master_socktcp_fd, (struct sockaddr *)&client_addr, &addr_len);
			if (comm_socket_fd<0){
				printf("Accept() error: errNum = %d\n", errno);
				exit(0);
			}
			
			printf("Connection accepted from client: %s:%u\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
			while (1) {
				printf("Server is ready to service client msgs.\n");
				memset(data_buffer, 0, sizeof(data_buffer));

				// Server receives the data from client. Client IP and PORT will be stored in client_addr.
				// State machine: state 3
				//
				// recvfrom()- Recieve msg from a socket
				// ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
				sent_recv_bytes = recvfrom(comm_socket_fd, 
						(char *)data_buffer, 
						sizeof(data_buffer), 
						0, 
						(struct sockaddr *)&client_addr, 
						&addr_len);

				// Process msg and optionally reply to client
				// State machine: state 4
				//
				printf("Server received %d bytes from client %s:%u\n", 
						sent_recv_bytes, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
				
				// If the server receives empty msg from client, it may close the connection and wait for fresh new connection.
				if (sent_recv_bytes == 0) {
					close(comm_socket_fd);
					break;
				} else {
					client_data = (test_struct_t *)data_buffer;
					
					if (client_data->a == 0 && client_data->b == 0) {
						close(comm_socket_fd);
						printf("Server closes the connection with client: %s:%u\n", 
								inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
						break;
					}
				}

				result_struct_t result;
				result.c = client_data->a + client_data->b;

				// Server replies back to client
				sent_recv_bytes = sendto(comm_socket_fd, 
						(char *)&result, 
						sizeof(result_struct_t), 
						0, 
						(struct sockaddr *)&client_addr, 
						sizeof(struct sockaddr));

				printf("Server sent %d bytes in reply to client\n", sent_recv_bytes);
			}

		}
	}

}


int
main(int argc, char* argv[]) {
	
	conduct_tcp_server();
	return 0;
}
