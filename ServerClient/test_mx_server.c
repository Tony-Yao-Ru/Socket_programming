#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<errno.h>
#include<string.h>
#include<arpa/inet.h>

#define MAX_CLIENT_SUPPORTED 32
#define SERVER_PORT 2000


typedef struct test_struct_t{
	unsigned int a;
	unsigned int b;
} test_struct_t;

typedef struct result_struct_t{
	unsigned int c;
} result_struct_t;


test_struct_t test_struct;
result_struct_t res_struct;

char data_buffer[1024];
int monitored_fd_set[32];


static void
Initialize_monitor_fd_set(){
	for (int i=0; i<MAX_CLIENT_SUPPORTED; i++){
		monitored_fd_set[i] = -1;
	}
}

static void
add_to_monitored_fd_set(int skfd){
	for (int i=0; i<MAX_CLIENT_SUPPORTED; i++){
		if (monitored_fd_set[i] != -1) continue;
		monitored_fd_set[i] = skfd;
		break;
	}
}

static void
remove_from_monitored_fd_set(int skfd){
	for (int i=0; i<MAX_CLIENT_SUPPORTED; i++){
		if (monitored_fd_set[i] != skfd) continue;
		monitored_fd_set[i] = -1;
		break;
	}
}

static void
initialize_readfds(fd_set *ptr_readfds) {
	FD_ZERO(ptr_readfds);
	for (int  i=0; i<MAX_CLIENT_SUPPORTED; i++){
		if (monitored_fd_set[i] != -1){
			FD_SET(monitored_fd_set[i], ptr_readfds);
		}
	}
}

static int
get_max_fd(){
	int max = -1;
	for (int i=0; i<MAX_CLIENT_SUPPORTED; i++){
		if (monitored_fd_set[i] > max){
			max = monitored_fd_set[i];
		}
	}
	return max;
}

void 
setup_tcp_mx_comm(){
	
	/* (a) Initialize */
	int master_sock_tcp_fd = 0,
	    comm_sock_fd = 0,
	    sent_recv_bytes = 0;

	struct sockaddr_in server_addr,
			   client_addr;
	int addr_len = 0;
	fd_set readfds;


	Initialize_monitor_fd_set();
	 

	/* (b) TCP master socket creation */
	if ( (master_sock_tcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))==-1 ){
		printf("Socket creation failed!!!\n");
		exit(1);
	}

	/* (c) Specify the server info */
	server_addr.sin_family = AF_INET; 
	server_addr.sin_port = SERVER_PORT;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	addr_len = sizeof(struct sockaddr);
	// Bind the server
	// When a socket is created with socket(), it exists in a name space but no address.
	// Therefore, bind() assigns the addr to the socket referred to by the fd.
	if ( bind(master_sock_tcp_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1 ){
		printf("Socket bind failed!!!\n");
		return;
	}

	/* (d) Listen the incoming client connections */
	if ( listen(master_sock_tcp_fd, 5) < 0){
		printf("Listen failed!!!\n");
		return;
	}
	
	// Add master socket to the monitored set of fds
	add_to_monitored_fd_set(master_sock_tcp_fd);
	
	/* (e) Initialize and fill the readfds */
	while(1){
		// Initialize and fill the readfds
		initialize_readfds(&readfds);

		printf("Blocked on select system call...\n");

		// State machine: state 1
		// Wait for client connection
		select(get_max_fd()+1, &readfds, NULL, NULL, NULL);
		
		// State machine: state 2
		// Reurn a new temporay fd via accept()
		if (FD_ISSET(master_sock_tcp_fd, &readfds)){
			printf("New connection recieved recvd, accept the connection. Client and server complete TCP-3 way handshake!!!\n");
			comm_sock_fd = accept(master_sock_tcp_fd,
					(struct sockaddr *)&client_addr,
					&addr_len);

			if (comm_sock_fd < 0){
				printf("Accept error: errno = %d\n", errno);
				exit(0);
			}
			
			add_to_monitored_fd_set(comm_sock_fd);
			printf("Connection accepted from client: %s:%u\n", 
					inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		} else {
			comm_sock_fd = -1;
			for (int i=0; i<MAX_CLIENT_SUPPORTED; i++){
				if (FD_ISSET(monitored_fd_set[i], &readfds)){
					comm_sock_fd = monitored_fd_set[i];
					memset(data_buffer, 0, sizeof(data_buffer));
					sent_recv_bytes = recvfrom(comm_sock_fd,
							(char *)data_buffer,
							sizeof(data_buffer),
							0,
							(struct sockaddr *)&client_addr,
							&addr_len);
					printf("Server recvd %d bytes from client %s:%u\n",
							sent_recv_bytes, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

					if (sent_recv_bytes == 0){
						close(comm_sock_fd);
						remove_from_monitored_fd_set(comm_sock_fd);
						break;
					}

					test_struct_t *client_data = (test_struct_t *)data_buffer;

					if (client_data->a == 0 && client_data->b == 0){
						close(comm_sock_fd);
						remove_from_monitored_fd_set(comm_sock_fd);
						printf("Server closes connection with client: %s:%u\n",
								inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
						break;
					}

					result_struct_t result;
					result.c = client_data->a + client_data->b;

					sent_recv_bytes = sendto(comm_sock_fd, 
							(char *)&result,
							sizeof(result_struct_t),
							0,
							(struct sockaddr *)&client_addr,
							sizeof(struct sockaddr));

					printf("Server sent %d bytes in reply to client!!!\n", sent_recv_bytes);
				}
			}
		}
	}

}


int
main(int argc, char* argv[]){
	setup_tcp_mx_comm();
	return 0;
}




