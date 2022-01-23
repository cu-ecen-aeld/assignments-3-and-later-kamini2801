#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[]){
    
    openlog("assign2_log", LOG_PID, LOG_USER);
    if(argc!=3){
        syslog(LOG_ERR, "Not enough arguments\n");
        return 1;
    }
    int fd;
    fd=open(argv[1],O_CREAT|O_WRONLY|O_TRUNC, 0644);
    if(fd==-1){
        syslog(LOG_ERR, "Could not open file\n");
        return 1;
    }
    int err=write(fd, argv[2], strlen(argv[2]));
    if(err==-1){
        syslog(LOG_ERR, "Could not write to file\n");
        return 1;
    }
    else if(err!=strlen(argv[2])){
        syslog(LOG_ERR, "Partially written to file\n");
        return 1;
    }
    else{
        syslog(LOG_DEBUG, "Writing '%s' to %s where '%s' "
        "is the text string written to file and %s is the "
        "file created  by the script\n", argv[2], argv[1], argv[2],argv[1]);
    }

}