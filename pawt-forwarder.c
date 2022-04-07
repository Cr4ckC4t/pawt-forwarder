/*
 * A very basic port forwarder
 *
 * Set the SERV_PORT, TARGET_PORT, TARGET_IP and compile with:
 *
 *     gcc pawt-forwarder.c -Wall -Wpedantic -Wextra
 *
 * └─$ ./a.out
 * [•] Starting port forwarder
 * [•] SERVER:88 ➜ 127.0.0.1:80
 *
 *                                   @crackcat
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define SERV_PORT 88           // Port for incoming connections
#define TARGET_PORT 80         // Targert port to forward connections to
#define TARGET_IP "127.0.0.1"  // Target address to forward connection to

#define MAX_CLNTS 10           // Max number of concurrent connections

#define EVER ;;

/* Colors */
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define ORANGE "\033[0;33m"
#define BLUE "\033[0;34m"
#define MAGENTA "\033[0;35m"
#define CYAN "\033[0;36m"
#define END "\033[0m"


/* Print error message and kill the program */
void dieWith(const char* message);

/* Create a connection to `target` and return the socket fd */
int getSockConnection(const char* target);

/* Handle an incoming request on the assigned fd */
void handleIncoming(int in_fd);

/* Start a plain TCP server */
int main(void) {

	printf("%s[•] Starting port forwarder%s\n", CYAN, END);

	/* Create socket */
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		dieWith("socket() failed");

	/* Set REUSEADDR option */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
		dieWith("setsockopt(REUSEADDR) failed");

	/* Create server address structure */
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(SERV_PORT);

	/* Bind socket to 0.0.0.0 */
	if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr)) < 0)
		dieWith("bind() failed");
	else
		printf("%s[•] SERVER:%i ➜ %s:%i%s\n", GREEN, SERV_PORT, TARGET_IP, TARGET_PORT, END);

	/* Listen for clients */
	if (listen(sockfd, MAX_CLNTS) < 0)
		dieWith("listen() failed");

	for(EVER) {
		/* Create client structure */
		struct sockaddr_in clientaddr;
		bzero(&clientaddr, sizeof(clientaddr));

		int incoming_fd;
		unsigned int len = sizeof(clientaddr);
		if ((incoming_fd = accept(sockfd, (struct sockaddr*)&clientaddr, &len)) < 0)
			dieWith("accept() failed");

		int pid;
		// fork failed
		if ((pid=fork()) < 0)
			dieWith("fork() failed");
		// child process
		else if (pid == 0)
			handleIncoming(incoming_fd);
		// the parent continues

	}

	// never reached
	close(sockfd);

	return EXIT_SUCCESS;
}


void handleIncoming(int in_fd) {
	/* Create a set of filedescriptors to check for new readable data */
	fd_set readset;
	FD_ZERO(&readset);
	FD_SET(in_fd, &readset);

	int out_fd;
	/* Get a socket for the connection to the target */
	if ((out_fd = getSockConnection(TARGET_IP)) < 0){
		close(in_fd);
		dieWith("getSocketConnection() failed");
	}

	FD_SET(out_fd, &readset);

	/* Save the original state of both file descriptors */
	fd_set original = readset;

	/* Enter the endless pipe between the two connections */
	int maxfd = (out_fd>in_fd?out_fd:in_fd);
	for(EVER) {
		// make sure the readset is in the original state
		readset = original;
		// wait with select until one is readable
		if (select( maxfd+1, &readset, NULL, NULL, NULL) < 0)
			dieWith("select() failed");

		// when one is readable forward a datapacket
		for (int i=maxfd; i >3; i--)
			if (FD_ISSET(i, &readset)) {
				char buf[4096];
				ssize_t recvlen = read(i, buf, sizeof(buf));
				if (recvlen <= 0) // end of file or error
					dieWith("No more data (EOF)");
				ssize_t sentlen = write((in_fd == i?out_fd:in_fd), buf, recvlen);
				if (sentlen <= 0 || sentlen != recvlen) // no data written or error
					dieWith("Error while sending data");
			}
	}

	// never reached
	close(in_fd);
	close(out_fd);

	exit(EXIT_SUCCESS);
}


int getSockConnection(const char* target) {
	int out_fd;

	if ((out_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	struct sockaddr_in targetaddr;
	bzero(&targetaddr, sizeof(targetaddr));

	targetaddr.sin_family = AF_INET;
	if (!inet_aton(target, &targetaddr.sin_addr))
		return -1;

	targetaddr.sin_port = htons(TARGET_PORT);

	if (connect(out_fd, (struct sockaddr*)&targetaddr, sizeof(targetaddr)) < 0)
		return -1;

	return out_fd;
}


void dieWith(const char* message) {
	perror(message);
	exit(EXIT_FAILURE);
}
