#define PORT "9000"
#define BACKLOG 10   // how many pending connections queue will hold
#define BUFSIZE 1048576
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
#include <stdbool.h>
#include <linux/fs.h>
#include <fcntl.h>

bool graceful_exit;
int sockfd;	

void signal_handler(int sig_num) {
	if ((sig_num == SIGINT) || (sig_num == SIGTERM))
	{
		syslog(LOG_INFO, "Caught signal, exiting\n");	
		shutdown(sockfd, SHUT_RDWR);
		graceful_exit = true;
	}
}

//from https://beej.us/guide/bgnet/html/#inet_ntopman
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc, char *argv[])
{
	graceful_exit = false;
	struct addrinfo hints, *res;
	int ret;	
	socklen_t addr_size;	
	struct sockaddr_storage their_addr;
	int new_fd;
	char recvbuf[BUFSIZE];
	char buf[BUFSIZE];
	long numbytes;
	char *writestr;
	long i;
	FILE *outputfile;
	int yes; 
	char s[INET6_ADDRSTRLEN];
	bool rx_done;
	long rx_index;
	long rx_numbytes;
	bool daemon;
	pid_t pid;	
		
	//Bind signals to handler	
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	sleep(1);
	
	//check the args to decide if daemon mode has been specified
	daemon = false;
	if (argc >= 2 && (strcmp(argv[1], "-d") == 0)) {
		daemon = true;
	}
	
	printf("daemon mode: %d\n", daemon);

	//use getaddrinfo to set up memory	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
	

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
	
	yes = 1;
	if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
		perror("setsockopt");
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

	//Free up this struct now that its not needed 
	freeaddrinfo(res);
	
	//handle daemon setup if needed
	if(daemon) {
		/* create new process */
		pid = fork();

		if (pid == -1) {
			perror("fork");
			return(-1);
		}
		
		else if(pid != 0) {
			exit(0);
		}

		/* create new session and process group */
		ret = setsid();
		if (ret == -1) {
			perror("setsid");
			return(-1);
		}

		/* set the working directory to the root directory */
		ret = chdir ("/");
		if (ret == -1) {
			return -1;
		}

		/* close all open files--NR_OPEN is overkill, but works */
		//???
		//	for (i = 0; i < NR_OPEN; i++)
		//	close(i);

		/* redirect fd's 0,1,2 to /dev/null */

		open ("/dev/null", O_RDWR);	//stdin
		dup (0);	//stdout
		dup (0); 	//stderr		
	}
	
	

	//listen for a connecton on the socket
	ret = listen(sockfd, BACKLOG);
	if(ret == -1)
	{
		close(sockfd);
		perror("listen");
		exit(-1);	
	}
	

	while(!graceful_exit)
	{
		//accept a connection on the socket... 
		addr_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);    
		
	 	if (new_fd == -1) 
	 	{
	 		//close(sockfd);
	 		//perror("accept");
	 		//exit(-1);
	 		continue;
	 	}

	 	//Log the IP address of socket connected 		 	
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("Accepted connection from %s\n", s);
        syslog(LOG_INFO, "Accepted connection from %s\n", s);
	 	
	 	
		//This looping is needed to handle long strings >16384 chars
		rx_done = false;
		rx_index = 0;
		rx_numbytes = 0;
		while (!rx_done) {
		
			//recevie a packet
		 	numbytes = recv(new_fd, recvbuf, BUFSIZE-1, 0);


		 	//Check for Errors
			if (numbytes == -1) {
		 		close(new_fd);
				printf("Closed connection from %s\n", s);
				syslog(LOG_INFO, "Closed connection from %s\n", s);	
		 		perror("recv");
		 		exit(-1);
		 	}
		 	
		 	//this makes sure we keep the total number of bytes in the message
			rx_numbytes += numbytes;
		 	
		 	//copy recv buffer into main buffer
		 	for(i=0; i<numbytes; i++) {
				buf[rx_index + i] = recvbuf[i];
		 	}
		 	
			rx_index += numbytes;
			
			//if the packet was \n terminated
			if(recvbuf[numbytes-1] == '\n') {
				rx_done = true;
			}		 	
		}
	 	printf("rx_numbytes: %ld\n", rx_numbytes);
	 	
		//make_sure the buffer is null-terminated
	 	buf[rx_numbytes] = '\0';
	 	rx_numbytes += 1;
 		
		//Put buffer data in a string
		writestr = (char *)malloc(rx_numbytes * sizeof(char));
		for (i = 0; i < rx_numbytes; i++)
		{
			writestr[i] = buf[i];
		}

		//Write the data to a file
		outputfile = fopen(OUTPUT_FILE_PATH, "a+");
		ret = fputs(writestr, outputfile);
		
		//cleanup
		free(writestr);
		
		//now readback the whole file
        fseek(outputfile, 0, SEEK_SET); 		  // go to the beginning of the file
        memset(buf, 0, BUFSIZE * sizeof(buf[0])); // zeroize the receive buffer
        while (fgets(buf, BUFSIZE, outputfile) != NULL)
        {
        	//printf("read from file: %ld\n", strlen(buf));
	        ret = send(new_fd, buf, strlen(buf), 0);
            if (ret == -1)
            {
                perror("send");
            }
        }
		
		//cleanup
		fclose(outputfile);
		close(new_fd);
		
        printf("Closed connection from %s\n", s);
        syslog(LOG_INFO, "Closed connection from %s\n", s);
	}
	
	while(graceful_exit)
	{
		printf("Received Interrupt... Exiting\n");
		close(sockfd);
		close(new_fd);
		remove(OUTPUT_FILE_PATH);
		exit(0);
	}
		
  
}
