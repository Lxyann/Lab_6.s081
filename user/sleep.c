#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(2, "ERROR: There should be a argument of time.");
        exit(1);
    }

    sleep(atoi(argv[argc - 1]));
    exit(0);
}

/*
== Test sleep, no arguments == sleep, no arguments: OK (4.6s) 
== Test sleep, returns == sleep, returns: OK (1.2s) 
== Test sleep, makes syscall == sleep, makes syscall: OK (0.9s) 
*/