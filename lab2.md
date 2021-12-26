System call tracing:

1:
$ git fetch
$ git checkout syscall

2:
//MakeFile
...
$U/_wc\
$U/_zombie\
$U/_trace\
...

3:
//user.h
...
int sleep(int);
int uptime(void);

int trace(int);
...

4:
//usys.pl
...
entry("sleep");
entry("uptime");

entry("trace");
...

5:
//syscall.h
...
#define SYS_mkdir  20
#define SYS_close  21

#define SYS_trace  22
...

//syscall.c
...
extern uint64 sys_trace(void);
...

...
[SYS_trace]   sys_trace,
...

6:
//sysproc.c
uint64 sys_trace(void){
  int trace_mask;
  if(argint(0, &trace_mask) < 0)
    return -1;
  myproc()->trace_mask = trace_mask;
  return 0;
}

7:
//proc.c -> fork()
...
np->state = RUNNABLE;

//lab2
np->trace_mask = p->trace_mask;

release(&np->lock);
...

8:
//syscall.c -> syscall

static char* syscall_names[] = {
  "",
  "fork",
  "exit",
  "wait",
  "pipe",
  "read",
  "kill",
  "exec",
  "fstat",
  "chdir",
  "dup",
  "getpid",
  "sbrk",
  "sleep",
  "uptime",
  "open",
  "write",
  "mknod",
  "unlink",
  "link",
  "mkdir",
  "close",
  "trace"};

...
p->trapframe->a0 = -1;
}
  
if((p->trace_mask & (1 << num)) != 0){
    printf("%d: syscall %s -> %d\n", p->pid, syscall_names[num], p->trapframe->a0);
}
...

/*
== Test trace 32 grep == trace 32 grep: OK (1.7s) 
== Test trace all grep == trace all grep: OK (0.9s) 
== Test trace nothing == trace nothing: OK (1.1s) 
== Test trace children == trace children: OK (14.1s) 
*/




Sysinfo:

1:
The steps of adding sys_sysinfo system call is the same as <System call tracing>.
Don't forget to add the tracing of sysinfo by insert "sysinfo" to syscall.c->syscall_names.

uint64 sys_sysinfo(void){
  uint64 addr;
  struct sysinfo sinfo;

  sinfo.freemem = freemem();
  sinfo.nproc = numprocs();

  if(argaddr(0, &addr) < 0)
    return -1;
  if(copyout(myproc()->pagetable, addr, (char *)&sinfo, sizeof(sinfo)) < 0)
    return -1;
  return 0;
}

2: Add functions freemem and numprocs to defs.h, and implement them in 
kernel/kalloc.c and kernel/proc.c respectively.


uint64 freemem(){
  uint num = 0;
  struct run *r;
  r = kmem.freelist;
  while(r){
    num ++;
    r = r->next;
  }
  return num * 4096;
}

uint64 numprocs(){
  uint64 num = 0;
  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state != UNUSED){
      num ++;
    }
  }
  return num;
}

/*
== Test trace 32 grep == trace 32 grep: OK (1.6s) 
== Test trace all grep == trace all grep: OK (1.4s) 
== Test trace nothing == trace nothing: OK (1.0s) 
== Test trace children == trace children: OK (15.8s) 
== Test sysinfotest == sysinfotest: OK (2.8s) 
== Test time == 
time: OK 
Score: 35/35
*/