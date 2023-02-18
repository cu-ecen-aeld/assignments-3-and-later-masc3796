#define PORT "9000"
#define BACKLOG 10   // how many pending connections queue will hold
#define BUFSIZE 1000000
#define OUTPUT_FILE_PATH "/var/tmp/aesdsocketdata"


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <netdb.h>
#include <syslog.h>


int main(int argc, char *argv[])
{

	struct addrinfo hints, *res;
	int sockfd;

	//use getaddrinfo to set up memory
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
	
	int ret;
	ret = getaddrinfo(NULL, PORT, &hints, &res);
	
	if (ret != 0) // getaddrinfo failed for some reason
	{ 
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(ret));
		exit(-1);
	}

	// create a socket
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	
	if (sockfd == -1) //socket creation failed
	{
		perror("socket");
		exit(-1);
	}

	// bind it to the port we passed in to getaddrinfo():
	ret = bind(sockfd, res->ai_addr, res->ai_addrlen);
	
	if(ret == -1)
	{
		close(sockfd);
		perror("bind");
		exit(-1);
	}
	
	// make sure to free up memory
	freeaddrinfo(res);
	
	
	//listen for a connecton on the socket
	ret = listen(sockfd, BACKLOG);
	if(ret == -1)
	{
		close(sockfd);
		perror("listen");
		exit(-1);	
	}

	//accept a connection on the socket... 
    socklen_t addr_size;	
    struct sockaddr_storage their_addr;
    addr_size = sizeof their_addr;
    int new_fd;


    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);    
    
 	if (new_fd == -1) 
 	{
 		close(sockfd);
 		perror("accept");
 		exit(-1);
 	}

 	//Log the IP address of socket connected 	
 	//char s[INET6_ADDRSTRLEN];
	//inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
 	//syslog(LOG_INFO, "Accepted connection from %s", s);
 	
 	char buf[BUFSIZE];
 	int numbytes;
 	numbytes = recv(new_fd, buf, BUFSIZE-1, 0);
 	
 	if (numbytes == -1) {
 		close(new_fd);
 		perror("recv");
 		exit(-1);
 	}
    buf[numbytes] = '\0'; //Null terminate the buffer!
    
    //Put buffer data in a string
    char *writestr;   
    writestr = (char *)malloc(numbytes * sizeof(char));
    
    int i;
	for (i = 0; i <= numbytes; i++)
	{
		writestr[i] = buf[i];
	} 
    
    //Write the data to a file
	FILE *outputfile;
	outputfile = fopen(OUTPUT_FILE_PATH, "a+");
	ret = fputs(writestr, outputfile);
	
	//cleanup
	free(writestr);
	fclose(outputfile);
	
    close(new_fd); // parent doesn't need this
    //syslog(LOG_INFO, "Closed connection from %s", s);
	
    
}
