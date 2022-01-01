#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "elf.h"

static int loadseg(pde_t *pgdir, uint64 addr, struct inode *ip, uint offset, uint sz);
// static int kloadseg(pagetable_t pagetable, uint64 va, struct inode *ip, uint offset, uint sz);

int
exec(char *path, char **argv)
{
  // printf("----------------------Entry exec---------------------\n");
  char *s, *last;
  int i, off;
  uint64 argc, sz = 0, sp, ustack[MAXARG+1], stackbase;
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pagetable_t pagetable = 0, oldpagetable;
  // pagetable_t pagetable = 0;
  
  struct proc *p = myproc();
  pagetable_t kernel_pagetable = 0, old_kernel_pagetable;
  // pagetable_t kernel_pagetable = p->kernel_pagetable;
  //lab3: check userinit's mapped page, if there is, free it.
  // if(kwalkaddr(p->kernel_pagetable, 0) != 0){
  //   printf("userinit's page mapped.\n");
  //   kvmunmap(p->kernel_pagetable, 0, 1, 0);
  // }

  // if(p->sz > 0){
  //   printf("pid: %d proc size: %p\n", p->pid, p->sz);
  //   kvmunmap(p->kernel_pagetable, 0, PGROUNDUP(p->sz)/PGSIZE, 0);
  //   // uvmunmap(p->pagetable, 0, PGROUNDUP(p->sz)/PGSIZE, 1);
  //   proc_freepagetable(p->pagetable, p->sz);
  // }

  // kernel_pagetable = p->kernel_pagetable;

  begin_op();

  if((ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);

  // Check ELF header
  if(readi(ip, 0, (uint64)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;
  if((pagetable = proc_pagetable(p)) == 0){
    printf("proc_pagetable error\n");
    goto bad;
  }
    
  
  //lab3
  // printf("before proc\n");
  if((kernel_pagetable = proc_kernel_pagetable(p)) == 0){
    printf("proc_kernel_pagetable error\n");
    goto bad;
  }

  // printf("after proc\n");
  // map kernel stack of this process. ***

  if(mappages(kernel_pagetable, p->kstack, PGSIZE, (uint64)kvmpa(p->kstack),
              PTE_R | PTE_W) != 0){
    panic("exec map stack.");
  }

  // Load program into memory.
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, 0, (uint64)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    uint64 sz1;
    if((sz1 = uvmalloc(pagetable, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    //lab3
    // printf("Before load program\n");
    if(kuvmalloc(kernel_pagetable, pagetable, sz, ph.vaddr + ph.memsz) == 0){
      goto bad;
    }
    // printf("Load program: %p\n", kwalkaddr(kernel_pagetable, sz));
    sz = sz1;
    //-----
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loadseg(pagetable, ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;

    //lab3 filesz < memsz
    // if(kloadseg(kernel_pagetable, ph.vaddr, ip, ph.off, ph.filesz) < 0)
    //   goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;


  p = myproc();
  uint64 oldsz = p->sz;

  // Allocate two pages at the next page boundary.
  // Use the second as the user stack.
  sz = PGROUNDUP(sz);
  uint64 sz1;
  if((sz1 = uvmalloc(pagetable, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  //lab3
  if(kuvmalloc(kernel_pagetable, pagetable, sz, sz + 2*PGSIZE) == 0){
    goto bad;
  }
  // printf("Load stack: %p\n", kwalkaddr(kernel_pagetable, sz));
  sz = sz1;

  uvmclear(pagetable, sz-2*PGSIZE);
  //lab3
  kuvmclear(kernel_pagetable, sz-2*PGSIZE);
  //
  sp = sz;
  stackbase = sp - PGSIZE;

  // Push argument strings, prepare rest of stack in ustack.
  printf("into args\n");
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp -= strlen(argv[argc]) + 1;
    sp -= sp % 16; // riscv sp must be 16-byte aligned
    printf("sp < stackbase\n");
    if(sp < stackbase)
      goto bad;
    printf("cout\n");
    if(copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    //lab3
    // if(kcopyout(kernel_pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
    //   goto bad;
    ustack[argc] = sp;
  }
  ustack[argc] = 0;
  printf("mid arg\n");
  // push the array of argv[] pointers.
  sp -= (argc+1) * sizeof(uint64);
  sp -= sp % 16;
  if(sp < stackbase)  
    goto bad;
  if(copyout(pagetable, sp, (char *)ustack, (argc+1)*sizeof(uint64)) < 0)
    goto bad;
  printf("out arg\n");
  //lab3
  // if(kcopyout(kernel_pagetable, sp, (char *)ustack, (argc+1)*sizeof(uint64)) < 0)
  //   goto bad;

  // arguments to user main(argc, argv)
  // argc is returned via the system call return
  // value, which goes in a0.
  p->trapframe->a1 = sp;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(p->name, last, sizeof(p->name));
  

  

  // Commit to the user image.
  oldpagetable = p->pagetable;
  p->pagetable = pagetable;
  p->sz = sz;
  p->trapframe->epc = elf.entry;  // initial program counter = main
  p->trapframe->sp = sp; // initial stack pointer
  proc_freepagetable(oldpagetable, oldsz);

  //lab3
  old_kernel_pagetable = p->kernel_pagetable;
  p->kernel_pagetable = kernel_pagetable;
  //--switch kernal table.
  w_satp(MAKE_SATP(p->kernel_pagetable));
  sfence_vma();
  //---------
  proc_free_kernel_pagetable(old_kernel_pagetable);
  if(p->pid==1) vmprint(pagetable);
  
  
  // printf("exec1!\n");
  // if (kuvmcopy(p->pagetable, p->kernel_pagetable, p->sz) < 0)
  // {
  //   // freeproc(p);
  //   // release(&p->lock);
  //   panic("exec: copy user to kernel.");
  //   return -1;
  // }
  // printf("exec!\n");

  return argc; // this ends up in a0, the first argument to main(argc, argv)

 bad:
  printf("bed\n");
  //lab3
  if(kernel_pagetable){
    proc_free_kernel_pagetable(kernel_pagetable);
    // vmprint(kernel_pagetable);
  }
    // kvmunmap(kernel_pagetable, 0, PGROUNDUP(sz)/PGSIZE, 0);
  printf("pass kernel_pagetable\n");
  if(pagetable)
    proc_freepagetable(pagetable, sz);

  if(ip){
    iunlockput(ip);
    end_op();
  }
  printf("returned\n");
  return -1;
}

// Load a program segment into pagetable at virtual address va.
// va must be page-aligned
// and the pages from va to va+sz must already be mapped.
// Returns 0 on success, -1 on failure.
static int
loadseg(pagetable_t pagetable, uint64 va, struct inode *ip, uint offset, uint sz)
{
  uint i, n;
  uint64 pa;

  if((va % PGSIZE) != 0)
    panic("loadseg: va must be page aligned");

  for(i = 0; i < sz; i += PGSIZE){
    pa = walkaddr(pagetable, va + i);
    if(pa == 0)
      panic("loadseg: address should exist");
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if(readi(ip, 0, (uint64)pa, offset+i, n) != n)
      return -1;
  }
  
  return 0;
}


// static int
// kloadseg(pagetable_t pagetable, uint64 va, struct inode *ip, uint offset, uint sz)
// {
//   uint i, n;
//   uint64 pa;

//   if((va % PGSIZE) != 0)
//     panic("kloadseg: va must be page aligned");

//   for(i = 0; i < sz; i += PGSIZE){
//     pa = kwalkaddr(pagetable, va + i);
//     if(pa == 0)
//       panic("kloadseg: address should exist");
//     if(sz - i < PGSIZE)
//       n = sz - i;
//     else
//       n = PGSIZE;
//     if(readi(ip, 0, (uint64)pa, offset+i, n) != n)
//       return -1;
//   }
  
//   return 0;
// }
