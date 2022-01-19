#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"
// #include "mman.h"

#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"



extern struct filmtable ftable;

static uint64
getfreeaddr(struct proc *p, uint length)
{
    struct vma *vp;
    uint64 bottomaddr = TRAPFRAME;
    int i;
    for(i = 0; i < NUMVAMS; i++){
        vp = &(p->vmas[i]);
        if(vp->used){
            if(bottomaddr > vp->addr){
                bottomaddr = vp->addr;
            }
        }
    }
    bottomaddr -= length;
    if(bottomaddr <= p->sz)
        panic("getfreeaddr: the film is too big to reach the heap area.\n");
    return bottomaddr;
}



struct vma *
getfreevma(struct proc *p)
{
    int i;
    for(i = 0; i < NUMVAMS; i++){
        if(!p->vmas[i].used){
            p->vmas[i].addr = -1; // maximum addr.
            p->vmas[i].used = 1;
            return &(p->vmas[i]);
        }
    }
    panic("mman: no free vma.\n");
}

struct vma *
getvma(struct proc *p, uint64 addr)
{
    int i;
    for(i = 0; i < NUMVAMS; i++){
        if(p->vmas[i].used && (p->vmas[i].addr <= addr && addr < p->vmas[i].addr + p->vmas[i].length)){
            return &(p->vmas[i]);
        }
    }
    printf("getvma: no such vma at address: %p\n", addr);
    return 0;
}




uint64 
mmap(uint64 addr, uint length, int prot, int flags,
           int fd, int offset)
{
    if(addr)
        panic("mmap: addr should be zero.\n");
    
    struct proc *p = myproc();
    struct file *f = p->ofile[fd];

    if(flags & MAP_SHARED){
        if(!f->readable){
        if(prot & PROT_READ){
                return -1;
            }
        }
        if(!f->writable){
            if(prot & PROT_WRITE){
                return -1;
            }
        }
    }
    

    struct vma *vp = getfreevma(p);
    
    vp->length = length;
    vp->prot = prot;
    vp->flags = flags;
    vp->offset = offset;
    vp->fp = p->ofile[fd]; // get film by fd.
    // vp->fp = filedup(vp->fp); // increase refcnt.
    acquire(&ftable.lock);
    vp->fp->ref++;
    release(&ftable.lock);

    uint64 retaddr = getfreeaddr(p, length);
    vp->addr = retaddr;
    printf("alloc addr: %p trapframe: %p\n", retaddr, TRAPFRAME);
    return retaddr;
}




uint64 
munmap(uint64 addr, uint length)
{
    // return -1;
    // printf("munmap addr: %p length: %d\n", addr, length);
    if(addr == 0)
        panic("munmap: addr should not be zero.\n");
    struct proc *p = myproc();
    struct vma *vp;
    if((vp = getvma(p, addr)) == 0){
        printf("munmap: no such vma.\n");
        exit(-1);
    }
    if(vp->addr + vp->length < addr + length){
        printf("munmap: unmap area is too large.\n");
        exit(-1);
    }
    uint64 va;
    for(va = addr; va < addr + length; va += PGSIZE){
        if(vp->flags & MAP_SHARED){
            pte_t *pte = walk(p->pagetable, va, 0);
            if(*pte & PTE_D){
                // printf("Encount dirty page.\n");
                begin_op();
                ilock(vp->fp->ip);
                writei(vp->fp->ip, 1, va, vp->offset, PGSIZE);
                iunlock(vp->fp->ip);
                end_op();
            }
        }
        // printf("uvmunmap: addr: %p\n", va);
        uvmunmap(p->pagetable, va, 1, 1);

        vp->addr += PGSIZE;
        vp->offset += PGSIZE;
        vp->length -= PGSIZE;
    }

    if(vp->length == 0){
        vp->used = 0;
        acquire(&ftable.lock);
        vp->fp->ref--;
        release(&ftable.lock);
    }
    
    return 0;
}