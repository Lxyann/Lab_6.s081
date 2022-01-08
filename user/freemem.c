#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void){
    int num = freemem();
    printf("%d\n", num);
    exit(0);
}