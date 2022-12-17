#include <stdio.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include<syslog.h>

//Replaces finder.sh from assignment 1
int main(int argc, char *argv[])  {

    //Initialize logging at LOG_USER level
    openlog(NULL, 0, LOG_USER);

   //Error if number of arguments is less than 2
   if( argc < 3 ) {
      syslog(LOG_ERR, "Missing Argument(s)");
      return 1;      
   }
   
   int handle;
   handle = creat(argv[1], 0644);
   
   //Error if failed to create file
   if (handle == -1) { 
    syslog(LOG_ERR, "Failed to create file");
    return 1; 
   }
   
   int line;
   line = write(handle, argv[2], strlen(argv[2]));

   //Error if failed to write the file   
   if (line == -1) {
    syslog(LOG_ERR, "Failed to write file");
    return 1;
   }  
   
   //Log success message
   syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);
   return 0;
   
}
