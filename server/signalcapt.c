
#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdbool.h>


bool graceful_exit;

void signal_handler(int sig_num) {
	if ((sig_num == SIGINT) || (sig_num == SIGTERM))
	{
		graceful_exit = true;
		printf("Received Interrupt... Exiting\n");
	}
}


int main(void)
{
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	
	while(1)
	{
		if (graceful_exit) {
			printf("Graceful Exit...\n");
			exit(0);
		}

	}

}

