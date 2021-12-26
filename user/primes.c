#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc, char* argv[]){
    int pl[2];
    pipe(pl);
    for(int i = 2; i != 36; i++){
        write(pl[1], &i, 4);
    }
    close(pl[1]);

    char buf[4];
    while(read(pl[0], buf, 4) == 4){
        int prime = *((int*)buf);
        printf("prime %d\n", prime);
        if(fork() == 0){
            int pl1[2];
            pipe(pl1);
            while(read(pl[0], buf, 4) == 4){
                int cur_num = *((int*)buf);
                if(cur_num % prime != 0){
                    write(pl1[1], &cur_num, 4);
                }
            }
            close(pl1[1]);
            pl[0]=pl1[0];
        }
        else{
            wait((int *)0);
        }
    }
    exit(0);
}

/*
== Test primes == primes: OK (1.9s) 
*/