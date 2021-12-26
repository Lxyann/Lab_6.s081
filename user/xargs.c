#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"


char* arg(int fd, int space){
    static char buf[MAXARG+1];
    char* p = buf;
    while(read(fd, p, 1) == 1 && *p != '\n'){
        // printf("%c\n", *p);
        if(*p == '"'){
            continue;
        }else if(*p == '\\'){
            if(read(fd, ++p, 1) == 1 && *p == 'n'){
                *(p - 1) = '\0';
                return buf;
            }
        }else if(*p == ' ' && space == 1){
            *p = '\0';
            return buf;
        }else{
            p++;
        }
    }
    if(*p == '\n'){
        *p = '\0';
        p++;
    }else{
        *(p - 1) = '\0';
    }
    if(read(fd, p, 1) == 0 && p == buf){
        return 0; // null
    }else{
        return buf;
    }
}


void exe_args(int space, char** args, int i){
    char* arg1;
    while((arg1 = arg(0, 1)) != 0){
        args[i] = arg1;
        args[i + 1] = 0;
        if(fork() == 0){
            exec(args[0], args);
        }else{
            wait((int *)0);
        }
    }
}



int main(int argc, char* argv[]){


    if(strcmp(argv[1], "-n") == 0){
        char* args[argc - 3 + 1 + 1];

        int i;
        for(i = 0; i < argc - 3 + 1 + 1 - 1 - 1; i++){
            args[i] = argv[i + 3];
        }
        exe_args(1, args, i);
    }else{
        char* args[argc - 1];

        int i;
        for(i = 0; i < argc - 1; i++){
            args[i] = argv[i + 1];
        }
        exe_args(0, args, i);
    }
    exit(0);
}


/*
init: starting sh
$ sh < xargstest.sh
$ $ $ $ $ $ hello
hello
hello
$ $ 

== Test xargs == xargs: OK (1.8s) 
*/