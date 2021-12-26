#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc, char* argv[]){
    int p_p2c[2]; // 0 read, 1 write.
    int p_c2p[2];

    pipe(p_p2c);
    pipe(p_c2p);
    if(fork() == 0){
        
        write(p_c2p[1], "pong", 4);
        char buf[4];
        read(p_p2c[0], buf, 4);
        printf("%d: received ",getpid());
        printf(buf);
        printf("\n");

        close(p_p2c[0]);
        close(p_c2p[1]);
    }else{

        write(p_p2c[1], "ping", 4);
        wait((int *)0); //wait for child exited.

        char buf[4];
        read(p_c2p[0], buf, 4);
        printf("%d: received ",getpid());
        printf(buf);
        printf("\n");

        close(p_p2c[1]);
        close(p_c2p[0]);
    }
    exit(0);
}



/*
init: starting sh
$ pingpong
4: received ping
3: received pong
$ 

== Test pingpong == pingpong: OK (1.7s) 
*/