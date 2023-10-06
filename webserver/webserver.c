#include<stdlib.h>
#include<stdio.h>
#include<sys/socket.h> 
#include<netinet/in.h> // Include "IPPROTO_TCP"
#include<arpa/inet.h> // Include htons
#include<sys/select.h>
#include<errno.h>
#include<memory.h>
#include<fcntl.h>
#include<sys/stat.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 104857600

char data_buffer[1024];



const char *get_file_extension(const char *file_name) {
    const char *dot = strrchr(file_name, '.');
    if (!dot || dot == file_name) {
        return "";
    }
    return dot + 1;
}

const char *get_mime_type(const char *file_ext) {
    if (strcasecmp(file_ext, "html") == 0 || strcasecmp(file_ext, "htm") == 0) {
        return "text/html";
    } else if (strcasecmp(file_ext, "txt") == 0) {
        return "text/plain";
    } else if (strcasecmp(file_ext, "jpg") == 0 || strcasecmp(file_ext, "jpeg") == 0) {
        return "image/jpeg";
    } else if (strcasecmp(file_ext, "png") == 0) {
        return "image/png";
    } else {
        return "application/octet-stream";
    }
}

static char *
process_GET_request(char *URL, unsigned int *response_len){
	printf("%s(%u) called with URL = %s\n", __FUNCTION__, __LINE__, URL);
	char *header = (char *)malloc(BUFFER_SIZE * sizeof(char));
	char *response = calloc(1, 1024);
	unsigned int content_len_str;

	// get file extension
        char file_ext[32];
        strcpy(file_ext, get_file_extension(URL));
	const char *mime_type = get_mime_type(file_ext);
	
	int file_fd = open(URL, O_RDONLY);
	if (file_fd == -1) {
		
		snprintf(header, BUFFER_SIZE,
                 	"HTTP/1.1 404 Not Found\r\n"
                 	"Content-Type: text/plain\r\n"
                 	"\r\n"
                 	"404 Not Found");
        	*response_len = strlen(response);
        	return header;
    	}

    	// get file size for Content-Length
    	struct stat file_stat;
    	fstat(file_fd, &file_stat);
    	off_t file_size = file_stat.st_size;

	// copy file to response buffer
    	ssize_t bytes_read;
    	while ((bytes_read = read(file_fd, response + *response_len, BUFFER_SIZE - *response_len)) > 0) {
        	*response_len += bytes_read;
    	}
	

    	/*create HTML hdr returned by server*/
    	strcpy(header, "HTTP/1.1 200 OK\n");
    	strcat(header, "Server: My Personal HTTP Server\n"    );
    	strcat(header, "Content-Length: "  );
    	strcat(header, "Connection: close\n"   );
    	//strcat(header, itoa(content_len_str));
    	strcat(header, "2048");
    	strcat(header, "\n");
    	strcat(header, "Content-Type: ");
	strcat(header, mime_type);
	strcat(header, "; charset=UTF-8\n");
    	strcat(header, "\n");

    	strcat(header, response);
    	content_len_str = strlen(header);
    	*response_len = content_len_str;
    	free(response);
    	return header;
	free(header);
}

static char *
process_POST_request(char *URL, unsigned int *response_len){
	return NULL;
}

void
setup_http_server(){

	/* Initialization */
	int master_sock_fd = 0,
	    comm_sock_fd = 0,
	    sent_recv_bytes = 0,
	    opt_sock = 1;

	fd_set readfds;
	
	struct sockaddr_in server_addr,
			  client_addr;

	socklen_t addr_len = sizeof(struct sockaddr_in);

	/* tcp master socket creation */
	
	// Socket()- create an endpoint for communication and return a file descriptor that refers to that endpoint
	// int socket(int domain, int type, int protocol)
	// domain: AF_INET (IPv4 Internet protocol)
	//         AF_INET6 (IPv6 Internet protocol)
	// type: SOCK_STREAM (Sequenced, reliable, two-way, connection-based byte streams)
	// 	 SOCK_DGRAM (connectionless, unreliable msg of a fixed max length)
	// protocol: IPPROTO_TCP
	// 	     IPPROTO_UDP
	// Return value: -1 (Error)
	if ( (master_sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
		printf("Socket creation failed!!!\n");
		exit(1);
	}

	/* Set master socket to allow multipkle connections */
	
	// setsockopt()- manipulate options for the socket referred to by the file descriptor sockfd
	// int getsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
	// level: SOL_SOCKET (To manipulate options at the socket API level)
	// optname: SO_BROADCAST (To permit sending of broadcast msgs)
	// 	    SO_REUSEADDR (To specify that the rules used in validating addr supplied to bind() should allow reuse of local addr)
	// optval: 0 (The option is disabled)
	// 	   1 (The option is enabled)
	// Return value: -1 (Error)
	if (setsockopt(master_sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt_sock, sizeof(opt_sock)) < 0){
		printf("TCP socket creation failed for multuiple connections!!!\n");
		exit(EXIT_FAILURE);
	}
	
	/* Specify the server info */
	
	// Included in "netinet/in.h"
	// sockaddr_in (IPv4) vs sockaddr_in6 (IPv6)
	//
	// sockaddr_in- Describe an IPv4 Internet domain socket addr
	// struct sockaddr_in {
	//	sin_family      /* AF_INET */
	//	sin_port        /* Port number */
	//	sin_addr        /* IPv4 addr */
	// } 
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT); // htons() converts the unsigned short integer hostshort
						   // from host byte order to network byte order
	server_addr.sin_addr.s_addr = INADDR_ANY;

	/* Bind the server */
	/* Binding means, we are telling kernel that any data you recieve with dest ip = SERVER_IP and tcp port = 2000
	 * send to this process */

	// Bind()- assign the address specified by addr to the socket referred to by the file descriptor sockfd
	// int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
	// addr: server address
	// Return value: -1 (error)
	if (bind(master_sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1){
		printf("Socket binded failed!!!\n");
		return;
	}

	/* Make OS maintain the queue of max len to Queue incoming client connections */

	// listen()- listen for connections on a socket
	// int listen(int sockfd, int backlog)
	// backlog: the maximum len to which the queue of pending connections for sockfd may grow.
	// Return value: -1 (error)
	if (listen(master_sock_fd, 5) == -1) {
		printf("Listen failed!!!\n");
		return;
	} else {
		printf("Server is listening on http://%s:%u\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
	}
	
	/* Service for the clients */
	
	while(1){
		FD_ZERO(&readfds);
		FD_SET(master_sock_fd, &readfds);
		printf("blocked on select system call...\n");

		/* Wait for client connection */

		// select()- allow a program to monitor multiple file descriptors, waiting until one or more of the file descriptors become
		// 	     ready
		// int select(int nfds, fd_set readfds, fd_set writefds, fd_set exceptfds, struct timeval timeout)
		// nfds: the highest-numbered file descriptor in any of the three sets, plus 1.
		select(master_sock_fd+1, &readfds, NULL, NULL, NULL);

		if (FD_ISSET(master_sock_fd, &readfds)){
			printf("New connection recieved recvd, accept the connection. Client and server completes TCP-3 way handshake.\n");

			/* Accept the connection */
			
			// Accept()- extract first connection request on the queue of pending connections for the listening socket (sockfd),
			//           create a new connected socket, and return a new file descriptor referring to that socket
			// int accept(int sockfd, struct sockaddr addrm socklen_t addrlen)
			// Return value: -1 (error)
			comm_sock_fd = accept(master_sock_fd, (struct sockaddr *)&client_addr, &addr_len);
			if (comm_sock_fd == -1){
				printf("accept error: errono = %d\n", errno);
				exit(0);
			}

			printf("Connection accepted from client: %s:%u\n",
					inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

			while(1){
				printf("Server is ready to service client msgs.\n");
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
					break;
				}

				// Implement the HTTP request
				printf("Msg recieved : %s\n", data_buffer);
                		char *request_line = NULL;
                		char del[2] = "\n", 
                     			*method = NULL,
                     			*URL = NULL;
                		request_line = strtok(data_buffer, del); /*Extract out the request line*/
                		del[0] = ' ';
                		method = strtok(request_line, del);     /*Tokenize the request line on the basis of space, and extract the first word*/
                		URL = strtok(NULL, del);                /*Extract the URL*/
				
				char *file_name = strtok(URL, "/");

                		printf("Method = %s\n", method);
                		printf("file_name = %s\n", file_name);
                		char *response = NULL;
                		unsigned int response_length = 0 ;
				
				if (strncmp(method, "GET", strlen("GET")) == 0){
                    			response = process_GET_request(file_name, &response_length);
                		} else if (strncmp(method, "POST", strlen("POST")) == 0){
                    			response = process_POST_request(file_name, &response_length);
                		} else{
                    			printf("Unsupported URL method request\n");
                    			close(comm_sock_fd);
                    			break;
                		}
				
				/* Server replying back to client now*/
                		if(response){
                    			printf("response to be sent to client = \n%s", response);
                    			sent_recv_bytes = sendto(comm_sock_fd, 
							response, 
							response_length, 
							0,
							(struct sockaddr *)&client_addr, 
							sizeof(struct sockaddr));
                    			free(response);
                    			printf("Server sent %d bytes in reply to client\n", sent_recv_bytes);
                    			//close(comm_socket_fd);
                    			//break;
                		}
                		/*Goto state machine State 3*/

			}
		}
	}
}

int
main(int argc, char* argv[]){
	setup_http_server();
	return 0;
}
