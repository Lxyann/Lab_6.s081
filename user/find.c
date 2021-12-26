#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


char* name(char* path){
  static char buf[DIRSIZ+1];
  char* p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  buf[strlen(p)] = 0;
  return buf;
}


void find(char* path, char* filename){

    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
    case T_FILE:
        fprintf(2, "find: path %s should not be a file\n", path);
        break;
    case T_DIR:
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
            printf("path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf+strlen(buf);
        *p++ = '/';
        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum == 0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if(stat(buf, &st) < 0){
                printf("find: cannot stat %s\n", buf);
                continue;
            }
            if(st.type == T_FILE){
                if(strcmp(name(buf), filename) == 0){
                    printf("%s\n", buf);
                }
            }else if(st.type == T_DIR){
                if(strcmp(name(buf), ".") == 0 ||
                   strcmp(name(buf), "..") == 0){
                    continue;
                }else{
                    find(buf, filename);
                }
            }
        }
        //case
        break;
    }
}



int main(int argc, char* argv[]){
    if(argc < 3){
        fprintf(2, "find: there should be 2 arguments, path, filename.\n");
        exit(1);
    }

    find(argv[1], argv[2]);
    exit(0);
}


/*
== Test find, in current directory == find, in current directory: OK (1.8s) 
== Test find, recursive == find, recursive: OK (1.2s) 
*/