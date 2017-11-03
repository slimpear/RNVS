//
//  main_v2.c
//  RNVS HA Block 2
//
//  Created by Amir Alchikh on 01.11.17.
//  Copyright © 2017 Informatik Studium. All rights reserved.
//

/*
 ** client.c -- a stream socket client demo
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#define MAXDATASIZE 100 // max number of bytes we can get at once


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int quoteOfTheDayProtocolTCP(void) {
    int sockfd = -1, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int returnValue;
    char s[INET6_ADDRSTRLEN];
    
    // Get adress to connect to
    char adressToConnect[255] = "alpha.mike-r.com";
	printf("Please write the Websiteaddress you want to connect to:\n");
	scanf("%255s", &adressToConnect[0]);

	// Get Port
    char port = 17;
	printf("Please write the port you want to use for the connection:\n");
	scanf("%s", &port);

	// Empty memory of hints and give every value & pointer in hints 0
    memset(&hints, 0, sizeof hints);
    // AF_UNSPEC = Not specified if IPv4 or IPv6
    hints.ai_family = AF_UNSPEC;
    // Use stream connection - TCP;
    hints.ai_socktype = SOCK_STREAM;

	/* Get the adress information of every adress/Server/Website
	 that is compatible with our adressToConnect and settings.
	 Passing Server adress, port, setting for possible conntections
	 and save correct return value in serverinfo struct, if no
	 error occures*/
    if ((returnValue = getaddrinfo(adressToConnect, &port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "'getaddrinfo' returned error: %s\n", gai_strerror(returnValue));
        return 1;
    }
    
    // loop through all the results and connect to the first we can

    for(p = servinfo; p != NULL; p = p->ai_next) {
		// Test if we can build up a socket with our configuration
        if ((sockfd = socket(p->ai_family,
                             p->ai_socktype,
                             IPPROTO_TCP)) == -1) {
            perror("Couldn't initiate socket.\n");
            continue;
        }

		// Test if we can build up a connection with wanted adress/Server
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("Couldn't build up connection to Website.\n");
            continue;
        }
		// If socket could be build up and connection works -> end loop
        break;
    }

	// If we left loop bc we have no adresses anymore end Programm
    if (p == NULL) {
        fprintf(stderr, "Failed to connect.\n");
        return 2;
    }

	/* Get Server/Inet adress in human readable format. Pass adress
	 family (IPv4 or IPv6), the adress, the memoryspace where the
	 result should be stored and  the max length of this string (if
	 memory is short, but normaly the biggest IPv4 or IPv6 possible
	 needed memory)*/
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);

	// Tell user where the connection is going too.
    printf("Client: connecting to %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure

    /* If someting gets sended to us over the passed socket, save
	 it in buf, but max buf size -1, use no flags, and test how
	 much data we got. If -1 an erro occured*/
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv\n");
        exit(1);
	}

	// Set last char in buf to end of buf, (should prevent overflow)
    buf[numbytes] = '\0';
	// Print recieved message
    printf("Client: received Quote of the Day:\n'%s'\n",buf);
	// Close socket
    close(sockfd);
    return 0;
}

#define BACKLOG 10   // how many pending connections queue will hold

void sigchld_handler(int s) {
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;
	while(waitpid(-1, NULL, WNOHANG) > 0);
	errno = saved_errno;
}

int streamServer (void) {
	int sockfd = -1, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	char s[INET6_ADDRSTRLEN];
	int returnVaule;


	// Get adress for server
	char addressForServer[255] = "";
	printf("Please write the IP that should be used for the server:\n");
	scanf("%255s", &addressForServer[0]);

	// Get Port
	char port = 17;
	printf("Please write the port you want to use for the server:\n");
	scanf("%s", &port);

	// Get Path to Quotes
	char path[255] = "/Users/amiralchikh/Desktop/Programme/ruv/RNVS HA Block 2/Quotes.txt";
//	printf("Please write path to the quotes that should be used by the server:\n");
//	scanf("%255s", &path[0]);

	if (atoi(&port) < 1025) {
		printf("Warning: port is under 1024.\n");
	}

	// Clear hint's memory
	memset(&hints, 0, sizeof hints);
	// Set up connection configuration
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	// Get adress
	if ((returnVaule = getaddrinfo(addressForServer, &port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(returnVaule));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
							 p->ai_protocol)) == -1) {
			perror("Server: error while setting up socket.\n");
			continue;
		}

		// Set up options for socket
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
					   sizeof(int)) == -1) {
			perror("Server: error occured in setsockopt\n");
			exit(1);
		}

		/* Bind the socket to our port so it will be used
		 by only one socket and ever incoming call goes
		 to the right socket.*/
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("Server: error occured in bind");
			continue;
		}
		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	// If no socket could be setup correctly end programm
	if (p == NULL)  {
		fprintf(stderr, "Server: failed to bind\n");
		exit(1);
	}

	// If our socket can't get a queue for in-comming calls end programm
	if (listen(sockfd, BACKLOG) == -1) {
		perror("Server : Socket can't listen.\n");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask); // Empty all signals
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("Server: Error in sigaction occured");
		exit(1);
	}

	printf("Server IP adress: %s\n",servinfo->ai_addr->sa_data);
	
	printf("Server: waiting for connections...\n");
	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;

		/* Acceptes next call that is in our main socket queue, if no
		 one is there wait for incoming Calls. Then accept first and
		 save the incoming adress in their_addr + plus create new socket
		 for communications just with the accepted one*/
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

		if (new_fd == -1) {
			perror("Server: couldn't accept incoming connection\n");
			continue;
		}

		inet_ntop(their_addr.ss_family,
				  get_in_addr((struct sockaddr *)&their_addr),
				  s, sizeof s);
		printf("Server: got connection from %s\n", s);

		// Create Thread For every connection and send msg
		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			// Open File with quotes
			FILE* quotes = fopen(path, "r");
			// Try to send quote
			if (quotes == NULL) {
				perror("Server: couldn't open quotes\n");
			} else {
				if (send(new_fd, "Hello, world!", 13, 0) == -1) {
					perror("Server: couldn't send Quote of the Day\n");
				}
			}
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}
	return 0;
}

int main(int argc, char *argv[]) {
//    printf("Quote of the Day ended with exit code: %d\n", quoteOfTheDayProtocolTCP());
	printf("Stream Server ended with exit code: %d\n", streamServer());
	return 0;
}
